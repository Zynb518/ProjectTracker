#pragma once

#include <cstdint>
#include <string>

#include "modules/project_template/domain/ProjectTemplateEnums.h"

namespace project_tracker::modules::project_template::dto::view {
    // 项目模板列表项展示对象
    struct ProjectTemplateListItemView {
        std::int64_t id;
        std::string name;
        std::string description;
        domain::ProjectTemplateStatus status;
        std::int64_t nodeCount;
    };
} // namespace project_tracker::modules::project_template::dto::view
