#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include "modules/health/service/HealthService.h"

namespace {
    std::string loadPostgresConnInfo() {
        const auto configPath = std::filesystem::path(PROJECT_TRACKER_SOURCE_DIR) /
                                "config" / "config.dev.json";
        std::ifstream input(configPath);
        if (!input.is_open()) {
            throw std::runtime_error("failed to open config.dev.json");
        }

        Json::Value root;
        input >> root;

        const auto &dbClients = root["db_clients"];
        if (!dbClients.isArray() || dbClients.empty()) {
            throw std::runtime_error("db_clients is missing in config.dev.json");
        }

        const auto &dbConfigJson = dbClients[0];
        return "host=" + dbConfigJson["host"].asString() +
               " port=" + std::to_string(dbConfigJson["port"].asUInt()) +
               " dbname=" + dbConfigJson["dbname"].asString() +
               " user=" + dbConfigJson["user"].asString() +
               " password=" + dbConfigJson["passwd"].asString();
    }
}

int main() {
    const auto dbClient = drogon::orm::DbClient::newPgClient(loadPostgresConnInfo(), 1);
    if (!dbClient) {
        std::cerr << "expected postgres db client to be created\n";
        return EXIT_FAILURE;
    }

    project_tracker::modules::health::service::HealthService service;
    const auto ready = drogon::sync_wait(service.isDatabaseReady(dbClient));
    if (!ready) {
        std::cerr << "expected postgres readiness probe to succeed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
