#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

#include <drogon/HttpRequest.h>

#include "http/RequestLogging.h"

namespace {
    bool isUuidFormat(const std::string &value) {
        if (value.size() != 36) {
            return false;
        }

        for (std::size_t index = 0; index < value.size(); ++index) {
            if (index == 8 || index == 13 || index == 18 || index == 23) {
                if (value[index] != '-') {
                    return false;
                }
                continue;
            }

            if (!std::isxdigit(static_cast<unsigned char>(value[index]))) {
                return false;
            }
        }

        return true;
    }
}

int main() {
    const auto request = drogon::HttpRequest::newHttpRequest();

    const auto firstRequestId = project_tracker::http::getOrCreateRequestId(request);
    if (firstRequestId.empty()) {
        std::cerr << "expected request_id to be generated\n";
        return EXIT_FAILURE;
    }

    const auto secondRequestId = project_tracker::http::getOrCreateRequestId(request);
    if (firstRequestId != secondRequestId) {
        std::cerr << "expected request_id to stay stable within one request\n";
        return EXIT_FAILURE;
    }

    if (!isUuidFormat(firstRequestId)) {
        std::cerr << "expected request_id to be a UUID string\n";
        return EXIT_FAILURE;
    }

    if (project_tracker::http::resolveRequestLogLevel(200, 999) !=
        project_tracker::http::RequestLogLevel::Info) {
        std::cerr << "expected sub-threshold success request to stay INFO\n";
        return EXIT_FAILURE;
    }

    if (project_tracker::http::resolveRequestLogLevel(200, 1000) !=
        project_tracker::http::RequestLogLevel::Warn) {
        std::cerr << "expected slow success request to become WARN\n";
        return EXIT_FAILURE;
    }

    if (project_tracker::http::resolveRequestLogLevel(404, 50) !=
        project_tracker::http::RequestLogLevel::Warn) {
        std::cerr << "expected client error request to stay WARN\n";
        return EXIT_FAILURE;
    }

    if (project_tracker::http::resolveRequestLogLevel(500, 50) !=
        project_tracker::http::RequestLogLevel::Error) {
        std::cerr << "expected server error request to stay ERROR\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
