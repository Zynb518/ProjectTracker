#pragma once

#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "modules/project_template/dto/view/ProjectTemplateDetailView.h"
#include "modules/project_template/dto/view/ProjectTemplateListItemView.h"

namespace project_tracker::modules::project_template::repository {
    class ProjectTemplateRepository {
    public:
        // 查询项目模板列表
        drogon::Task<std::vector<dto::view::ProjectTemplateListItemView>>
        listTemplates(std::optional<domain::ProjectTemplateStatus> status) const;

        // 查询项目模板详情
        drogon::Task<std::optional<dto::view::ProjectTemplateDetailView>>
        findTemplateDetail(std::int64_t templateId) const;
    };
} // namespace project_tracker::modules::project_template::repository
