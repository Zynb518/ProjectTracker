#pragma once

namespace project_tracker::common::error {
    // 业务错误码
    // 说明：
    // 1. 这里的编号先对齐 design-flow/04-api-design.md
    // 2. 后续如果要新增错误码，尽量按号段继续追加，避免打乱已有编号
    enum class ErrorCode : int {
        // 通用成功
        Success = 0,

        // 400xx：请求与权限相关错误
        InvalidParameter = 40001, // 参数错误
        LoginFailed = 40002, // 登录失败
        Unauthorized = 40003, // 未登录或登录态失效
        Forbidden = 40004, // 无权限访问

        // 404xx：资源不存在
        UserNotFound = 40401, // 用户不存在
        ProjectTemplateNotFound = 40402, // 项目模板不存在
        ProjectNotFound = 40403, // 项目不存在
        ProjectMemberNotFound = 40404, // 项目成员不存在
        PhaseNotFound = 40405, // 阶段节点不存在
        SubTaskNotFound = 40406, // 子任务不存在

        // 409xx：业务规则冲突
        UsernameAlreadyExists = 40901, // 用户名已存在
        UserDisabled = 40902, // 用户已被禁用
        ProjectMemberAlreadyExists = 40903, // 项目成员重复添加
        ProjectCompletedReadonly = 40904, // 项目已完成，禁止直接修改
        PhaseCompletedReadonly = 40905, // 阶段节点已完成，禁止直接修改
        SubTaskCompletedReadonly = 40906, // 子任务已完成，禁止直接修改
        SubTaskResponsibleUserInvalid = 40907, // 子任务负责人不属于当前项目成员
        ProjectMemberHasUnfinishedTasks = 40908, // 项目成员仍存在未完成子任务，不能移除
        InvalidDateRange = 40909, // 时间区间不合法或不在父级范围内
        StatusProgressMismatch = 40910, // 状态与进度百分比不匹配
        PhaseSequenceConflict = 40911, // 阶段顺序号冲突
        CompletedPhaseReorderForbidden = 40912, // 已完成阶段节点不允许调整顺序
        PersonalProjectMemberManageForbidden = 40913, // 不允许对个人自用项目执行成员管理
        ProjectCompleteConditionNotMet = 40914, // 项目当前不满足完成条件
        PhaseCompleteConditionNotMet = 40915, // 阶段节点当前不满足完成条件
        StartConditionNotMet = 40916, // 当前对象不满足手动开始条件
        ReopenFailed = 40917, // 撤销完成失败
        OwnerTransferTargetInvalid = 40918, // 负责人转交目标不合法
        TemplateNodeMappingInvalid = 40919, // 模板节点映射不完整或顺序不合法
        ProjectTemplateAlreadyApplied = 40920, // 项目已存在阶段节点，不能再次整套应用模板

        // 500xx：系统内部错误
        InternalError = 50000 // 系统内部错误
    };

    // 把枚举转换成 int，方便后续直接写入 JSON 返回体
    inline constexpr int toInt(ErrorCode code) {
        return static_cast<int>(code);
    }
} // namespace project_tracker::common::error
