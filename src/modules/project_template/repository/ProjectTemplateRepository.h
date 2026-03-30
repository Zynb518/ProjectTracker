#pragma once

#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "modules/project_template/dto/view/ProjectTemplateListItemView.h"

namespace project_tracker::modules::project_template::repository {
    class ProjectTemplateRepository {
    public:
        // 查询项目模板列表
        drogon::Task<std::vector<dto::view::ProjectTemplateListItemView>>
        listTemplates(std::optional<domain::ProjectTemplateStatus> status) const;
    };
} // namespace project_tracker::modules::project_template::repository
