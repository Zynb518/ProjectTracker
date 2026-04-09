#pragma once

#include <json/value.h>

#include <drogon/utils/coroutine.h>

#include "modules/project_ai/dto/command/GenerateProjectDraftInput.h"

namespace project_tracker::modules::project_ai::service {
    class ProjectAiService {
    public:
        drogon::Task<Json::Value>
        generateProjectDraft(const dto::command::GenerateProjectDraftInput &input) const;
    };
} // namespace project_tracker::modules::project_ai::service
