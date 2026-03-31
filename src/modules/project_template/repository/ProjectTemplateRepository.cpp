#include "modules/project_template/repository/ProjectTemplateRepository.h"

#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_template::repository {
    namespace error = project_tracker::common::error;

    drogon::Task<std::vector<dto::view::ProjectTemplateListItemView>>
    ProjectTemplateRepository::listTemplates(const common::db::SqlExecutorPtr &executor,
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
            const auto result = co_await executor->execSqlCoro(
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

    drogon::Task<std::optional<dto::view::ProjectTemplateDetailView>>
    ProjectTemplateRepository::findTemplateDetail(const common::db::SqlExecutorPtr &executor,
                                                  std::int64_t templateId) const {
        static const std::string findTemplateDetailSql = R"SQL(
            SELECT
                pt.id,
                pt.name,
                pt.description,
                pt.status,
                ptn.id AS node_id,
                ptn.name AS node_name,
                ptn.description AS node_description,
                ptn.sequence_no
            FROM project_template pt
            LEFT JOIN project_template_node ptn ON ptn.template_id = pt.id
            WHERE pt.id = $1
            ORDER BY ptn.sequence_no ASC, ptn.id ASC
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTemplateDetailSql,
                templateId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &firstRow = result.front();
            dto::view::ProjectTemplateDetailView detail{
                .id = firstRow["id"].as<std::int64_t>(),
                .name = firstRow["name"].as<std::string>(),
                .description = firstRow["description"].as<std::string>(),
                .status = static_cast<domain::ProjectTemplateStatus>(firstRow["status"].as<int>()),
                .nodes = {}
            };
            detail.nodes.reserve(result.size());

            for (const auto &row : result) {
                if (row["node_id"].isNull()) {
                    continue;
                }

                detail.nodes.push_back(dto::view::ProjectTemplateDetailNodeView{
                    .id = row["node_id"].as<std::int64_t>(),
                    .name = row["node_name"].as<std::string>(),
                    .description = row["node_description"].as<std::string>(),
                    .sequenceNo = row["sequence_no"].as<int>()
                });
            }

            co_return detail;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_template::repository
