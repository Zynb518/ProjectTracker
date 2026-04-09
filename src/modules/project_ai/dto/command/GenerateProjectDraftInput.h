#pragma once

#include <string>

namespace project_tracker::modules::project_ai::dto::command {
    struct GenerateProjectDraftInput {
        std::string prompt;
    };
} // namespace project_tracker::modules::project_ai::dto::command
