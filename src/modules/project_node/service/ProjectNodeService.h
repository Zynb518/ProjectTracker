#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project/repository/ProjectRepository.h"
#include "modules/project_node/dto/ProjectNodeDto.h"
#include "modules/project_node/repository/ProjectNodeRepository.h"

namespace project_tracker::modules::project_node::service {
    class ProjectNodeService {
    public:
        // 基于模板批量生成阶段节点
        drogon::Task<dto::view::AppliedProjectNodeTemplateView>
        applyProjectNodeTemplate(
            const dto::command::ApplyProjectNodeTemplateInput &input) const;

        // 创建阶段节点
        drogon::Task<dto::view::CreatedProjectNodeView>
        createProjectNode(const dto::command::CreateProjectNodeInput &input) const;

        // 修改阶段节点基础信息
        drogon::Task<dto::view::UpdatedProjectNodeBasicInfoView>
        updateProjectNodeBasicInfo(
            const dto::command::UpdateProjectNodeBasicInfoInput &input) const;

        // 手动开始阶段节点
        drogon::Task<dto::view::UpdatedProjectNodeStatusView>
        startProjectNode(const dto::command::ProjectNodeStatusActionInput &input) const;

        // 手动完成阶段节点
        drogon::Task<dto::view::UpdatedProjectNodeStatusView>
        completeProjectNode(const dto::command::ProjectNodeStatusActionInput &input) const;

        // 撤销阶段节点完成
        drogon::Task<dto::view::UpdatedProjectNodeStatusView>
        reopenProjectNode(const dto::command::ProjectNodeStatusActionInput &input) const;

        // 调整阶段节点顺序
        drogon::Task<dto::view::ReorderedProjectNodesView>
        reorderProjectNodes(const dto::command::ReorderProjectNodesInput &input) const;

        // 删除阶段节点
        drogon::Task<std::int64_t>
        deleteProjectNode(const dto::command::DeleteProjectNodeInput &input) const;

    private:
        project::repository::ProjectRepository projectRepository_;
        repository::ProjectNodeRepository projectNodeRepository_;
    };
} // namespace project_tracker::modules::project_node::service
