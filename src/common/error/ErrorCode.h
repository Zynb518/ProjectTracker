#pragma once

namespace project_tracker::common::error {
    // 业务错误码
    // 说明：
    // 1. 这里的编号先对齐 design-flow/04-api-design.md
    // 2. 后续如果要新增错误码，尽量按号段继续追加，避免打乱已有编号
    enum class ErrorCode : int {
        // 通用成功
        kSuccess = 0,

        // 400xx：请求与权限相关错误
        kInvalidParameter = 40001, // 参数错误
        kLoginFailed = 40002, // 登录失败
        kUnauthorized = 40003, // 未登录或登录态失效
        kForbidden = 40004, // 无权限访问

        // 404xx：资源不存在
        kUserNotFound = 40401, // 用户不存在
        kProjectTemplateNotFound = 40402, // 项目模板不存在
        kProjectNotFound = 40403, // 项目不存在
        kProjectMemberNotFound = 40404, // 项目成员不存在
        kPhaseNotFound = 40405, // 阶段节点不存在
        kSubTaskNotFound = 40406, // 子任务不存在

        // 409xx：业务规则冲突
        kUsernameAlreadyExists = 40901, // 用户名已存在
        kUserDisabled = 40902, // 用户已被禁用
        kProjectMemberAlreadyExists = 40903, // 项目成员重复添加
        kProjectCompletedReadonly = 40904, // 项目已完成，禁止直接修改
        kPhaseCompletedReadonly = 40905, // 阶段节点已完成，禁止直接修改
        kSubTaskCompletedReadonly = 40906, // 子任务已完成，禁止直接修改
        kSubTaskResponsibleUserInvalid = 40907, // 子任务负责人不属于当前项目成员
        kProjectMemberHasUnfinishedTasks = 40908, // 项目成员仍存在未完成子任务，不能移除
        kInvalidDateRange = 40909, // 时间区间不合法或不在父级范围内
        kStatusProgressMismatch = 40910, // 状态与进度百分比不匹配
        kPhaseSequenceConflict = 40911, // 阶段顺序号冲突
        kCompletedPhaseReorderForbidden = 40912, // 已完成阶段节点不允许调整顺序
        kPersonalProjectMemberManageForbidden = 40913, // 不允许对个人自用项目执行成员管理
        kProjectCompleteConditionNotMet = 40914, // 项目当前不满足完成条件
        kPhaseCompleteConditionNotMet = 40915, // 阶段节点当前不满足完成条件
        kStartConditionNotMet = 40916, // 当前对象不满足手动开始条件
        kReopenFailed = 40917, // 撤销完成失败
        kOwnerTransferTargetInvalid = 40918, // 负责人转交目标不合法
        kTemplateNodeMappingInvalid = 40919, // 模板节点映射不完整或顺序不合法
        kProjectTemplateAlreadyApplied = 40920, // 项目已存在阶段节点，不能再次整套应用模板

        // 500xx：系统内部错误
        kInternalError = 50000 // 系统内部错误
    };

    // 把枚举转换成 int，方便后续直接写入 JSON 返回体
    inline constexpr int toInt(ErrorCode code) {
        return static_cast<int>(code);
    }
} // namespace project_tracker::common::error
