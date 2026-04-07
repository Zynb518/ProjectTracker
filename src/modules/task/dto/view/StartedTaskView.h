#pragma once

#include "modules/task/dto/view/TaskProgressRecordView.h"
#include "modules/task/dto/view/UpdatedTaskStatusView.h"

namespace project_tracker::modules::task::dto::view {
    // 手动开始子任务成功后的展示对象
    struct StartedTaskView {
        UpdatedTaskStatusView subtask;
        TaskProgressRecordView progressRecord;
    };
} // namespace project_tracker::modules::task::dto::view
