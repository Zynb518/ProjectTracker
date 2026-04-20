#include <cstdlib>
#include <iostream>

#include <drogon/drogon.h>

#include "modules/health/controller/HealthController.h"

int main() {
    const auto request = drogon::HttpRequest::newHttpRequest();

    project_tracker::modules::health::controller::HealthController controller;
    const auto response = drogon::sync_wait(controller.health(request));
    if (!response) {
        std::cerr << "expected /api/health to return a response\n";
        return EXIT_FAILURE;
    }

    if (response->statusCode() != drogon::k200OK) {
        std::cerr << "expected /api/health to return HTTP 200\n";
        return EXIT_FAILURE;
    }

    const auto &json = response->getJsonObject();
    if (!json) {
        std::cerr << "expected /api/health to return JSON body\n";
        return EXIT_FAILURE;
    }

    if ((*json)["code"].asInt() != 0) {
        std::cerr << "expected /api/health code to be 0\n";
        return EXIT_FAILURE;
    }

    if ((*json)["message"].asString() != "ok") {
        std::cerr << "expected /api/health message to be ok\n";
        return EXIT_FAILURE;
    }

    if (!(*json)["data"].isObject()) {
        std::cerr << "expected /api/health data to be an object\n";
        return EXIT_FAILURE;
    }

    if ((*json)["data"]["status"].asString() != "ok") {
        std::cerr << "expected /api/health data.status to be ok\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
