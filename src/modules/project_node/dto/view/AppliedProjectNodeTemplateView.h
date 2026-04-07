#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace project_tracker::modules::project_node::dto::view {
    // 套用模板成功后，单个生成节点的返回对象
    struct AppliedProjectNodeTemplateItemView {
        std::int64_t id;
        std::string name;
        int sequenceNo;
        std::string plannedStartDate;
        std::string plannedEndDate;
    };

    // 套用模板成功后的返回对象
    struct AppliedProjectNodeTemplateView {
        std::int64_t projectId;
        std::int64_t templateId;
        std::vector<AppliedProjectNodeTemplateItemView> generatedNodes;
    };
} // namespace project_tracker::modules::project_node::dto::view
