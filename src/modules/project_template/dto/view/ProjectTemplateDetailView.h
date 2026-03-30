#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "modules/project_template/domain/ProjectTemplateEnums.h"

namespace project_tracker::modules::project_template::dto::view {
    // 项目模板详情中的节点展示对象
    struct ProjectTemplateDetailNodeView {
        std::int64_t id;
        std::string name;
        std::string description;
        int sequenceNo;
    };

    // 项目模板详情展示对象
    struct ProjectTemplateDetailView {
        std::int64_t id;
        std::string name;
        std::string description;
        domain::ProjectTemplateStatus status;
        std::vector<ProjectTemplateDetailNodeView> nodes;
    };
} // namespace project_tracker::modules::project_template::dto::view
