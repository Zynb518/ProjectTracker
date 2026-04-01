#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::project::dto::view {
    // 转交项目负责人成功后的展示对象
    struct TransferredProjectOwnerView {
        std::int64_t projectId;
        std::int64_t previousOwnerUserId;
        std::int64_t ownerUserId;
        std::string ownerRealName;
        bool autoAddedAsMember;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project::dto::view
