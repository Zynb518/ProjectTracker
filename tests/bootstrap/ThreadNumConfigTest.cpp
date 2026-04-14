#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <json/value.h>

#include "bootstrap/ThreadNumConfig.h"

namespace {
    bool expectThrows(const Json::Value &customConfig) {
        try {
            (void)project_tracker::bootstrap::readConfiguredThreadNum(customConfig);
            return false;
        } catch (const std::runtime_error &) {
            return true;
        }
    }
}

int main() {
    {
        Json::Value customConfig(Json::objectValue);
        const auto threadNum = project_tracker::bootstrap::readConfiguredThreadNum(customConfig);
        if (threadNum.has_value()) {
            std::cerr << "expected missing threads_num to return nullopt\n";
            return EXIT_FAILURE;
        }
    }

    {
        Json::Value customConfig(Json::objectValue);
        customConfig["threads_num"] = 0;

        const auto threadNum = project_tracker::bootstrap::readConfiguredThreadNum(customConfig);
        if (!threadNum.has_value() || *threadNum != 0) {
            std::cerr << "expected threads_num=0 to be accepted\n";
            return EXIT_FAILURE;
        }
    }

    {
        Json::Value customConfig(Json::objectValue);
        customConfig["threads_num"] = 8;

        const auto threadNum = project_tracker::bootstrap::readConfiguredThreadNum(customConfig);
        if (!threadNum.has_value() || *threadNum != 8) {
            std::cerr << "expected threads_num=8 to be accepted\n";
            return EXIT_FAILURE;
        }
    }

    {
        Json::Value customConfig(Json::objectValue);
        customConfig["threads_num"] = "8";

        if (!expectThrows(customConfig)) {
            std::cerr << "expected string threads_num to throw\n";
            return EXIT_FAILURE;
        }
    }

    {
        Json::Value customConfig(Json::objectValue);
        customConfig["threads_num"] = -1;

        if (!expectThrows(customConfig)) {
            std::cerr << "expected negative threads_num to throw\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
