#include "modules/project_ai/service/ProjectAiService.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

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
            std::filesystem::path modelDir;
        };

        struct TempRequestFile {
            std::filesystem::path path;

            ~TempRequestFile() {
                if (path.empty()) {
                    return;
                }

                std::error_code errorCode;
                std::filesystem::remove(path, errorCode);
            }
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

        LocalAiConfig readLocalAiConfig() {
            const Json::Value &localAiConfig = drogon::app().getCustomConfig()["local_ai"];
            if (!localAiConfig.isObject()) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "local_ai 未配置");
            }

            return LocalAiConfig{
                .pythonBin = readConfiguredPath(localAiConfig, "python_bin", false),
                .scriptPath = readConfiguredPath(localAiConfig, "script_path", false),
                .modelDir = readConfiguredPath(localAiConfig, "model_dir", true)
            };
        }

        TempRequestFile createRequestFile(const std::string &prompt) {
            char tempFileTemplate[] = "/tmp/project_tracker_ai_request_XXXXXX";
            const int fd = ::mkstemp(tempFileTemplate);
            if (fd == -1) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "创建本地 AI 请求文件失败");
            }
            ::close(fd);

            Json::Value requestJson(Json::objectValue);
            requestJson["prompt"] = prompt;

            Json::StreamWriterBuilder writerBuilder;
            writerBuilder["indentation"] = "";

            std::ofstream output(tempFileTemplate, std::ios::out | std::ios::trunc);
            if (!output.is_open()) {
                std::filesystem::remove(tempFileTemplate);
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "写入本地 AI 请求文件失败");
            }

            output << Json::writeString(writerBuilder, requestJson);
            output.close();

            return TempRequestFile{
                .path = tempFileTemplate
            };
        }

        std::string runLocalAiProcess(const LocalAiConfig &config,
                                      const std::filesystem::path &inputFilePath) {
            int pipeFds[2];
            if (::pipe(pipeFds) == -1) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "创建本地 AI 输出管道失败");
            }

            const pid_t pid = ::fork();
            if (pid == -1) {
                ::close(pipeFds[0]);
                ::close(pipeFds[1]);
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "启动本地 AI 进程失败");
            }

            if (pid == 0) {
                ::dup2(pipeFds[1], STDOUT_FILENO);
                ::dup2(pipeFds[1], STDERR_FILENO);
                ::close(pipeFds[0]);
                ::close(pipeFds[1]);

                std::string pythonBin = config.pythonBin.string();
                std::string scriptPath = config.scriptPath.string();
                std::string modelDir = config.modelDir.string();
                std::string inputFile = inputFilePath.string();
                char modelDirFlag[] = "--model-dir";
                char inputFileFlag[] = "--input-file";

                char *const argv[] = {
                    pythonBin.data(),
                    scriptPath.data(),
                    modelDirFlag,
                    modelDir.data(),
                    inputFileFlag,
                    inputFile.data(),
                    nullptr
                };

                ::execv(pythonBin.c_str(), argv);
                _exit(127);
            }

            ::close(pipeFds[1]);

            std::string output;
            std::array<char, 4096> buffer{};
            while (true) {
                const ssize_t bytesRead = ::read(pipeFds[0], buffer.data(), buffer.size());
                if (bytesRead <= 0) {
                    break;
                }

                output.append(buffer.data(), static_cast<std::size_t>(bytesRead));
            }
            ::close(pipeFds[0]);

            int waitStatus = 0;
            if (::waitpid(pid, &waitStatus, 0) == -1) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "等待本地 AI 进程结束失败");
            }

            if (!WIFEXITED(waitStatus) || WEXITSTATUS(waitStatus) != 0) {
                LOG_ERROR << "local ai invocation failed: " << output;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "本地 AI 生成失败");
            }

            return output;
        }

        Json::Value parseGeneratedDraftJson(const std::string &rawOutput) {
            Json::CharReaderBuilder readerBuilder;
            readerBuilder["collectComments"] = false;

            Json::Value generatedDraft(Json::objectValue);
            std::string errors;
            std::istringstream input(rawOutput);
            if (!Json::parseFromStream(readerBuilder, input, &generatedDraft, &errors)) {
                LOG_ERROR << "local ai returned invalid json: " << errors
                          << ", raw output: " << rawOutput;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "本地 AI 返回了非法 JSON");
            }

            if (!generatedDraft.isObject() ||
                !generatedDraft["project"].isObject() ||
                !generatedDraft["nodes"].isArray()) {
                LOG_ERROR << "local ai returned unexpected payload: " << rawOutput;
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "本地 AI 返回结构不符合预期");
            }

            return generatedDraft;
        }
    } // namespace

    drogon::Task<Json::Value>
    ProjectAiService::generateProjectDraft(
        const dto::command::GenerateProjectDraftInput &input) const {
        const auto config = readLocalAiConfig();
        const auto requestFile = createRequestFile(input.prompt);
        const auto rawOutput = runLocalAiProcess(config, requestFile.path);
        co_return parseGeneratedDraftJson(rawOutput);
    }
} // namespace project_tracker::modules::project_ai::service
