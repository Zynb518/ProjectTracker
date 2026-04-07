#pragma once

#include <cstdint>
#include <string>

#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::task::dto::view {
    // 子任务进度记录展示对象
    struct TaskProgressRecordView {
        std::int64_t id;
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        std::string progressNote;
        int progressPercent;
        domain::TaskStatus status;
        std::string createdAt;
    };
} // namespace project_tracker::modules::task::dto::view
