#include "modules/project_ai/service/ProjectAiService.h"

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>

#include <json/reader.h>
#include <json/writer.h>

#include <sys/wait.h>
#include <unistd.h>

#include <drogon/drogon.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_ai::service {
    namespace error = project_tracker::common::error;

    namespace {
        struct LocalAiConfig {
            std::filesystem::path pythonBin;
            std::filesystem::path scriptPath;
            std::filesystem::path workerScriptPath;
            std::filesystem::path modelDir;
        };

        std::filesystem::path resolveConfiguredPath(const std::string &value) {
            const std::filesystem::path path(value);
            if (path.is_absolute()) {
                return path;
            }

            return std::filesystem::absolute(path);
        }

        std::filesystem::path readConfiguredPath(const Json::Value &config,
                                                 const char *fieldName,
                                                 bool shouldBeDirectory) {
            const Json::Value &field = config[fieldName];
            if (!field.isString() || field.asString().empty()) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    std::string("local_ai.") + fieldName + " 未配置");
            }

            const auto resolvedPath = resolveConfiguredPath(field.asString());
            if (!std::filesystem::exists(resolvedPath)) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    std::string("local_ai.") + fieldName + " 指向的路径不存在");
            }

            if (shouldBeDirectory && !std::filesystem::is_directory(resolvedPath)) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    std::string("local_ai.") + fieldName + " 必须是目录");
            }

            if (!shouldBeDirectory && !std::filesystem::is_regular_file(resolvedPath)) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    std::string("local_ai.") + fieldName + " 必须是文件");
            }

            return resolvedPath;
        }

        std::filesystem::path resolveWorkerScriptPath(const std::filesystem::path &scriptPath) {
            const auto workerScriptPath = scriptPath.parent_path() / "generate_worker.py";
            if (!std::filesystem::exists(workerScriptPath) ||
                !std::filesystem::is_regular_file(workerScriptPath)) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "local_ai.script_path 所在目录缺少 generate_worker.py");
            }

            return workerScriptPath;
        }

        LocalAiConfig readLocalAiConfig() {
            const Json::Value &localAiConfig = drogon::app().getCustomConfig()["local_ai"];
            if (!localAiConfig.isObject()) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "local_ai 未配置");
            }

            const auto scriptPath = readConfiguredPath(localAiConfig, "script_path", false);
            return LocalAiConfig{
                .pythonBin = readConfiguredPath(localAiConfig, "python_bin", false),
                .scriptPath = scriptPath,
                .workerScriptPath = resolveWorkerScriptPath(scriptPath),
                .modelDir = readConfiguredPath(localAiConfig, "model_dir", true)
            };
        }

        std::string trimAsciiWhitespace(const std::string &value) {
            std::size_t start = 0;
            while (start < value.size()
                   && std::isspace(static_cast<unsigned char>(value[start]))) {
                ++start;
            }

            std::size_t end = value.size();
            while (end > start
                   && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
                --end;
            }

            return value.substr(start, end - start);
        }

        bool isInformationalLocalAiStderr(const std::string &value) {
            return value.rfind("Device set to use ", 0) == 0
                   && value.find('\n') == std::string::npos;
        }

        void logLocalAiStderr(const std::string &stderrText) {
            const auto normalizedStderrText = trimAsciiWhitespace(stderrText);
            if (normalizedStderrText.empty()) {
                return;
            }

            if (isInformationalLocalAiStderr(normalizedStderrText)) {
                LOG_INFO << "local ai stderr: " << normalizedStderrText;
            } else {
                LOG_WARN << "local ai stderr: " << normalizedStderrText;
            }
        }

        void closeFd(int &fd) {
            if (fd == -1) {
                return;
            }

            ::close(fd);
            fd = -1;
        }

        std::string serializeJsonLine(const Json::Value &json) {
            Json::StreamWriterBuilder writerBuilder;
            writerBuilder["indentation"] = "";
            return Json::writeString(writerBuilder, json);
        }

        Json::Value parseJsonText(const std::string &rawText,
                                  const char *invalidMessage,
                                  const char *unexpectedMessage,
                                  const char *invalidLogPrefix,
                                  const char *unexpectedLogPrefix) {
            Json::CharReaderBuilder readerBuilder;
            readerBuilder["collectComments"] = false;

            Json::Value parsed(Json::objectValue);
            std::string errors;
            std::istringstream input(rawText);
            if (!Json::parseFromStream(readerBuilder, input, &parsed, &errors)) {
                LOG_ERROR << invalidLogPrefix << ": " << errors
                          << ", raw output: " << rawText;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    invalidMessage);
            }

            if (!parsed.isObject()) {
                LOG_ERROR << unexpectedLogPrefix << ": " << rawText;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    unexpectedMessage);
            }

            return parsed;
        }

        Json::Value validateGeneratedDraftJson(const Json::Value &generatedDraft,
                                               const std::string &rawOutputForLog) {
            if (!generatedDraft.isObject() ||
                !generatedDraft["project"].isObject() ||
                !generatedDraft["nodes"].isArray()) {
                LOG_ERROR << "local ai returned unexpected payload: " << rawOutputForLog;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "本地 AI 返回结构不符合预期");
            }

            return generatedDraft;
        }

        class PersistentLocalAiWorker {
        public:
            explicit PersistentLocalAiWorker(LocalAiConfig config)
                : config_(std::move(config)) {
            }

            ~PersistentLocalAiWorker() {
                std::lock_guard<std::mutex> lock(mutex_);
                stopWorker();
            }

            Json::Value requestDraft(const std::string &prompt) {
                std::lock_guard<std::mutex> lock(mutex_);
                ensureWorkerStarted();

                try {
                    Json::Value requestJson(Json::objectValue);
                    requestJson["request_id"] = std::to_string(++nextRequestId_);
                    requestJson["prompt"] = prompt;

                    std::string requestLine = serializeJsonLine(requestJson);
                    requestLine.push_back('\n');
                    writeAll(stdinFd_, requestLine);

                    const auto responseLine = readLine(stdoutFd_, "读取本地 AI worker 响应失败");
                    return parseWorkerResponse(requestJson["request_id"].asString(), responseLine);
                } catch (...) {
                    stopWorker();
                    throw;
                }
            }

        private:
            void ensureWorkerStarted() {
                if (pid_ > 0) {
                    return;
                }

                startWorker();
            }

            void startWorker() {
                int stdinPipeFds[2] = {-1, -1};
                int stdoutPipeFds[2] = {-1, -1};
                int stderrPipeFds[2] = {-1, -1};

                if (::pipe(stdinPipeFds) == -1) {
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "创建本地 AI worker 输入管道失败");
                }

                if (::pipe(stdoutPipeFds) == -1) {
                    ::close(stdinPipeFds[0]);
                    ::close(stdinPipeFds[1]);
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "创建本地 AI worker 输出管道失败");
                }

                if (::pipe(stderrPipeFds) == -1) {
                    ::close(stdinPipeFds[0]);
                    ::close(stdinPipeFds[1]);
                    ::close(stdoutPipeFds[0]);
                    ::close(stdoutPipeFds[1]);
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "创建本地 AI worker 错误管道失败");
                }

                const pid_t pid = ::fork();
                if (pid == -1) {
                    ::close(stdinPipeFds[0]);
                    ::close(stdinPipeFds[1]);
                    ::close(stdoutPipeFds[0]);
                    ::close(stdoutPipeFds[1]);
                    ::close(stderrPipeFds[0]);
                    ::close(stderrPipeFds[1]);
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "启动本地 AI worker 进程失败");
                }

                if (pid == 0) {
                    ::dup2(stdinPipeFds[0], STDIN_FILENO);
                    ::dup2(stdoutPipeFds[1], STDOUT_FILENO);
                    ::dup2(stderrPipeFds[1], STDERR_FILENO);

                    ::close(stdinPipeFds[0]);
                    ::close(stdinPipeFds[1]);
                    ::close(stdoutPipeFds[0]);
                    ::close(stdoutPipeFds[1]);
                    ::close(stderrPipeFds[0]);
                    ::close(stderrPipeFds[1]);

                    std::string pythonBin = config_.pythonBin.string();
                    std::string workerScriptPath = config_.workerScriptPath.string();
                    std::string modelDir = config_.modelDir.string();
                    char modelDirFlag[] = "--model-dir";

                    char *const argv[] = {
                        pythonBin.data(),
                        workerScriptPath.data(),
                        modelDirFlag,
                        modelDir.data(),
                        nullptr
                    };

                    ::execv(pythonBin.c_str(), argv);

                    const auto errorText = std::string("execv failed: ") + std::strerror(errno) + "\n";
                    const auto written = ::write(STDERR_FILENO, errorText.c_str(), errorText.size());
                    (void)written;
                    _exit(127);
                }

                ::close(stdinPipeFds[0]);
                ::close(stdoutPipeFds[1]);
                ::close(stderrPipeFds[1]);

                stdinFd_ = stdinPipeFds[1];
                stdoutFd_ = stdoutPipeFds[0];
                stderrFd_ = stderrPipeFds[0];
                pid_ = pid;

                const int stderrReadFd = stderrFd_;
                stderrThread_ = std::thread([this, stderrReadFd]() {
                    drainWorkerStderr(stderrReadFd);
                });
            }

            void stopWorker() {
                closeFd(stdinFd_);

                if (pid_ > 0) {
                    if (::kill(pid_, SIGTERM) == -1 && errno != ESRCH) {
                        LOG_WARN << "failed to stop local ai worker, errno=" << errno;
                    }

                    int waitStatus = 0;
                    if (::waitpid(pid_, &waitStatus, 0) == -1 && errno != ECHILD) {
                        LOG_WARN << "failed to wait local ai worker exit, errno=" << errno;
                    }

                    pid_ = -1;
                }

                if (stderrThread_.joinable()) {
                    stderrThread_.join();
                }

                closeFd(stdoutFd_);
                closeFd(stderrFd_);
            }

            void drainWorkerStderr(int fd) const {
                std::string currentLine;

                while (true) {
                    char ch = '\0';
                    const ssize_t bytesRead = ::read(fd, &ch, 1);
                    if (bytesRead == 0) {
                        break;
                    }

                    if (bytesRead < 0) {
                        if (errno == EINTR) {
                            continue;
                        }
                        break;
                    }

                    if (ch == '\n') {
                        logLocalAiStderr(currentLine);
                        currentLine.clear();
                        continue;
                    }

                    currentLine.push_back(ch);
                }

                logLocalAiStderr(currentLine);
            }

            void writeAll(int fd, const std::string &content) const {
                std::size_t written = 0;
                while (written < content.size()) {
                    const ssize_t bytesWritten = ::write(
                        fd,
                        content.data() + written,
                        content.size() - written
                    );
                    if (bytesWritten < 0) {
                        if (errno == EINTR) {
                            continue;
                        }

                        error::throwInternalError(
                            error::ErrorCode::InternalError,
                            "向本地 AI worker 发送请求失败");
                    }

                    written += static_cast<std::size_t>(bytesWritten);
                }
            }

            std::string readLine(int fd, const char *errorMessage) const {
                std::string output;

                while (true) {
                    char ch = '\0';
                    const ssize_t bytesRead = ::read(fd, &ch, 1);
                    if (bytesRead == 0) {
                        break;
                    }

                    if (bytesRead < 0) {
                        if (errno == EINTR) {
                            continue;
                        }

                        error::throwInternalError(
                            error::ErrorCode::InternalError,
                            errorMessage);
                    }

                    if (ch == '\n') {
                        break;
                    }

                    output.push_back(ch);
                }

                if (output.empty()) {
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "本地 AI worker 未返回响应");
                }

                return output;
            }

            Json::Value parseWorkerResponse(const std::string &expectedRequestId,
                                            const std::string &rawResponse) const {
                const auto response = parseJsonText(
                    rawResponse,
                    "本地 AI worker 返回了非法 JSON",
                    "本地 AI worker 返回结构不符合预期",
                    "local ai worker returned invalid json",
                    "local ai worker returned unexpected payload"
                );

                if (!response["request_id"].isString() ||
                    response["request_id"].asString() != expectedRequestId ||
                    !response["ok"].isBool()) {
                    LOG_ERROR << "local ai worker returned unexpected payload: " << rawResponse;
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "本地 AI worker 返回结构不符合预期");
                }

                if (!response["ok"].asBool()) {
                    const auto errorMessage = response["error"].isString()
                                                  ? response["error"].asString()
                                                  : "unknown worker error";
                    LOG_ERROR << "local ai worker returned error, request_id="
                              << expectedRequestId << ", error=" << errorMessage;
                    error::throwInternalError(
                        error::ErrorCode::InternalError,
                        "本地 AI 生成失败");
                }

                return validateGeneratedDraftJson(response["draft"], rawResponse);
            }

            LocalAiConfig config_;
            mutable std::mutex mutex_;
            std::thread stderrThread_;
            int stdinFd_ = -1;
            int stdoutFd_ = -1;
            int stderrFd_ = -1;
            pid_t pid_ = -1;
            std::uint64_t nextRequestId_ = 0;
        };

        PersistentLocalAiWorker &getPersistentLocalAiWorker() {
            static PersistentLocalAiWorker worker(readLocalAiConfig());
            return worker;
        }
    } // namespace

    drogon::Task<Json::Value>
    ProjectAiService::generateProjectDraft(
        const dto::command::GenerateProjectDraftInput &input) const {
        co_return getPersistentLocalAiWorker().requestDraft(input.prompt);
    }
} // namespace project_tracker::modules::project_ai::service
