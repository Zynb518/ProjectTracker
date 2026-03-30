---
name: project-tracker-stepwise-backend
description: Incremental workflow for the Project-Tracker C++20/Drogon/PostgreSQL backend. Use when working in this repo and the user wants Chinese communication, simple code, CLion-style formatting, and exactly one small code segment or one micro-change per turn for review. Trigger on requests such as "继续写", "先看 controller", "一段一段来", "我先审查", or "每次只生成一段代码".
---

# Project Tracker Stepwise Backend

## Overview

用于当前 `Project-Tracker` 仓库的后端协作约束。目标不是一次性把功能铺满，而是按“先读上下文 -> 只推进一个小段 -> 等用户审查 -> 再继续”的节奏稳定开发。

## Read Context First

- 开始写代码前，先读相关的 `design-flow/`、`db/migrations/`、`db/seeds/`、当前源文件和 `CMakeLists.txt`。
- 不要凭记忆延续上一次回答，优先以仓库中的实际文件为准。
- 如果用户说“先看 controller / service / repository”，先只检查对应文件，再决定下一小步。
- 如果用户说“我先审查”，下一次继续时必须从已存在代码接着推进，不要重铺整套实现。

## One Segment Per Turn

- 每次只生成一段代码，或者只完成一个微小改动。
- 默认的一段范围应当小到便于人工审查：
- 一个函数声明
- 一个函数实现
- 一个 controller 局部辅助函数
- 一个 repository 查询
- 一小段 CMake
- 一小段 SQL
- 不要在同一轮里同时铺开 `controller + service + repository`，除非用户明确要求整段或整文件。
- 如果一个需求天然涉及多个文件，先完成第一个阻塞文件或第一段核心代码，然后停下等用户继续。
- 如果已经获得直接编辑授权，也只做这一小段，不要顺手扩展到下一段。

## Response Pattern

- 回答代码时优先用这个顺序：
- `Target:` 指明文件和落点
- `Why:` 只解释当前段为什么这样写
- `Next:` 明确下一步是什么，但不要提前把下一段代码也写出来
- 当前协作默认不展示 `Code:`，只保留 `Target / Why / Next`。
- 只有用户明确要求查看当前代码片段时，才额外补 `Code:`。
- 用户说“继续”时，给下一个最小可推进片段，不重复之前整段内容。
- 用户说“审查”时，先给发现，再给是否继续改。

## Project Defaults

- 使用 C++20、Drogon、PostgreSQL。
- 源文件后缀使用 `.cpp`。
- 构建目录固定为 `cmake-build-debug`。
- CMake 保持简单，优先根 `CMakeLists.txt` 加 `src/CMakeLists.txt`，不要引入不必要的复杂模块化写法。
- 当前 `Project_Tracker` 可执行目标定义在 `src/CMakeLists.txt`，所以默认产物路径是 `cmake-build-debug/src/Project_Tracker`。
- 中文注释可以保留；新增注释要简短直接，只解释职责或当前这小段逻辑。
- SQL 脚本放在 `db/migrations/` 和 `db/seeds/`，不要继续堆在 `design-flow/`。
- 头文件引用按 `src` 根来写，例如 `#include "common/error/ErrorCode.h"`。
- 组装 JSON 时优先直接赋值，不为了整数类型额外包 `Json::Int64(...)`。

## Naming And Structure

- 当前模块本地命名空间不额外起别名，优先直接写 `repository::`、`service::`、`dto::view::` 这类当前作用域下可直接访问的名字。
- 只有跨模块且全路径明显影响可读性时，才考虑保留一个必要别名。
- `common` 这类全局公共命名空间可以继续写成：
- `namespace error = project_tracker::common::error;`
- 当前项目偏好简单实现，避免为了“抽象完整”引入多余 DTO、包装器、匿名辅助层。
- 对“简单纯读接口”允许直接走 `Controller -> Repository`，不要为了形式对称强行补一层 service。
- 像 `/api/auth/me` 这种只按 session 查询单条用户信息的接口，可以直接在 controller 中访问 repository。
- 如果某个辅助函数只在一个 `.cpp` 文件里局部使用，可以放匿名命名空间；如果反而让代码跳转变多，就直接内联到主函数中。
- repository 优先直写主查询逻辑，除非重复已经明显出现。

## Data And Enum Rules

- 如果数据库已有 `CHECK` 约束，应用层不重复做同一层枚举合法性兜底，优先直接映射。
- 对中国区固定业务时间，可以明确按 `Asia/Shanghai` 处理。
- 若返回时间字符串，优先生成接口最终需要的格式，不要保留中间态再二次加工。
- 即便正常链路里“理论上一定存在”，repository 仍可用 `std::optional` 显式表达“数据库可能查不到”这一事实，例如 session 残留或用户记录已不存在的情况。
- 分页相关的 `page`、`page_size`、`total` 优先使用 `std::int64_t`，和 PostgreSQL 的 `COUNT(*)`、`LIMIT`、`OFFSET` 保持一致。

## Error Handling Rules

- `BusinessException` 只用于表达明确的 HTTP 业务失败，例如 `400 / 401 / 403 / 404 / 409`。
- `Controller` 是 HTTP 边界；只要调用链中可能抛 `BusinessException`，`try/catch` 就必须覆盖整段请求处理流程，而不是只包某个 `co_await`。
- `Service` 可以抛 `BusinessException`，用于业务规则不满足、权限失败、状态冲突等场景。
- `Repository` 不用异常表达“正常查不到”；这类情况优先返回 `std::optional`。
- 对单条写 SQL 可直接表达的唯一冲突，优先使用 PostgreSQL 原子冲突处理，例如 `ON CONFLICT ... DO NOTHING RETURNING`，再由空结果映射 `409`，不要优先依赖 Drogon 细分数据库异常类型的捕获。
- 事务用于保障多条 SQL 的原子性，不要只为了识别单条唯一约束冲突就引入事务。
- `Repository` 只在数据库异常、底层调用失败等系统错误场景，把异常转成 `InternalError`。
- `common/util` 默认不直接抛 `BusinessException`，优先返回值、`std::optional` 或显式失败态，让 `Controller / Service` 决定如何返回 HTTP 错误。
- `filters` 优先直接返回 `api::fail(...)`，不要依赖抛异常完成拦截。
- 对 query 参数这类基础输入，优先只做格式校验；如果数据库已有 `CHECK` 约束，不重复在 controller 里做同层枚举兜底。

## Current Auth Snapshot

- 当前已落地认证接口：
- `POST /api/auth/login`
- `POST /api/auth/logout`
- `GET /api/auth/me`
- `GET /api/auth/me` 当前通过 `filters/LoginRequiredFilter` 保护，路由约束里直接使用 `filters::LoginRequiredFilter::classTypeName()`。
- 登录成功后会把 `user_id` 和 `system_role` 写入 session，并调用 `changeSessionIdToClient()`。
- 登出时会清空 session，并调用 `changeSessionIdToClient()`。
- 认证模块当前分工：
- `findUserByUsername()` 仍返回 `AuthUserRecord`，因为登录要校验 `password_hash`
- `findUserById()` 已改为返回 `SysUserView`，用于 `/api/auth/me` 这种只读展示场景
- `/api/auth/me` 当前采用 `Controller -> Repository`，如果 session 中 `user_id` 非法或数据库已查不到该用户，则返回未登录，并清理 session。

## Filter And Test Notes

- 登录态过滤器放在 `src/filters/`，当前已有 `LoginRequiredFilter`。
- 新增受保护接口时，优先先挂 filter，再补业务实现，避免 controller 里先散落未登录判断。
- 手工联调用例放在 `tests/http/`。
- `.http` 文件按接口粒度拆分，不按整模块长期混放；例如 `listUsers.http` 这种命名优于宽泛的 `users.http`。
- JetBrains / CLion 的 `.http` 文件优先保持最简单形式，默认信任 IDE 自动保存和回放 Cookie，不手写 `Set-Cookie` 解析脚本，除非确实需要。
- `.http` 用例的顺序优先写成“正常路径在前，异常路径在后”，便于人工顺序点击验证。

## Debugging Workflow

- 出现联调或运行时 bug 时，一次只定位并修复一个问题，不并行修改多个未确认根因。
- 修复 bug 时优先做最小改动，先让当前问题收平，再决定是否继续做下一处清理。
- 每次小改后优先做本地编译验证：`cmake -S . -B cmake-build-debug` 和 `cmake --build cmake-build-debug`。
- 如果是 `.http` 联调问题，代码层编译通过后先停给用户重测，不顺手扩展到下一个功能点。

## Review Discipline

- 不要在用户尚未审查前连续推进多层实现。
- 不要顺手修改无关格式或重构用户刚整理过的代码风格。
- 每次完成一段后，停在“当前段已经自洽、便于审查”的位置。
- 如果用户提出风格偏好，以用户当前仓库中的真实风格为准，而不是模型偏好。
