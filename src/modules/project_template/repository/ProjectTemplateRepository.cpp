#include "modules/project_template/repository/ProjectTemplateRepository.h"

#include <string>

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_template::repository {
    namespace error = project_tracker::common::error;

    drogon::Task<std::vector<dto::view::ProjectTemplateListItemView>>
    ProjectTemplateRepository::listTemplates(
        std::optional<domain::ProjectTemplateStatus> status) const {
        static const std::string listTemplatesSql = R"SQL(
            SELECT
                pt.id,
                pt.name,
                pt.description,
                pt.status,
                COUNT(ptn.id) AS node_count
            FROM project_template pt
            LEFT JOIN project_template_node ptn ON ptn.template_id = pt.id
            WHERE pt.status = $1
            GROUP BY
                pt.id,
                pt.name,
                pt.description,
                pt.status
            ORDER BY pt.id ASC
        )SQL";

        const auto statusForQuery = status.value_or(domain::ProjectTemplateStatus::Enabled);

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await dbClient->execSqlCoro(
                listTemplatesSql,
                domain::toInt(statusForQuery));

            std::vector<dto::view::ProjectTemplateListItemView> list;
            list.reserve(result.size());

            for (const auto &row : result) {
                list.push_back(dto::view::ProjectTemplateListItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .description = row["description"].as<std::string>(),
                    .status = static_cast<domain::ProjectTemplateStatus>(row["status"].as<int>()),
                    .nodeCount = row["node_count"].as<std::int64_t>()
                });
            }

            co_return list;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_template::repository
