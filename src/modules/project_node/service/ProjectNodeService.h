#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project_node/dto/ProjectNodeDto.h"
#include "modules/project_node/repository/ProjectNodeRepository.h"

namespace project_tracker::modules::project_node::service {
    class ProjectNodeService {
    public:
        // 创建阶段节点
        drogon::Task<dto::view::CreatedProjectNodeView>
        createProjectNode(const dto::command::CreateProjectNodeInput &input) const;

        // 修改阶段节点基础信息
        drogon::Task<dto::view::UpdatedProjectNodeBasicInfoView>
        updateProjectNodeBasicInfo(
            const dto::command::UpdateProjectNodeBasicInfoInput &input) const;

        // 调整阶段节点顺序
        drogon::Task<dto::view::ReorderedProjectNodesView>
        reorderProjectNodes(const dto::command::ReorderProjectNodesInput &input) const;

    private:
        repository::ProjectNodeRepository projectNodeRepository_;
    };
} // namespace project_tracker::modules::project_node::service
