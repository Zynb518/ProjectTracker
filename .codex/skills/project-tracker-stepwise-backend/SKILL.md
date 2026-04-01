---
name: project-tracker-stepwise-backend
description: Use when continuing backend work in this Project-Tracker C++20/Drogon/PostgreSQL repo with Chinese communication, CLion-style formatting, and one small code segment or micro-change per turn for review.
---

# Project Tracker Stepwise Backend

## Overview

用于当前 `Project-Tracker` 仓库的后端协作约束。目标不是一次性把功能铺满，而是按“先读上下文 -> 只推进一个小段 -> 等用户审查 -> 再继续”的节奏稳定开发。
如果当前对话里用户已经给出更具体的新约束，以本轮确认过的约束为准，不要死守 skill 里的旧偏好。

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

## Project Defaults

- 使用 C++20、Drogon、PostgreSQL。
- 源文件后缀使用 `.cpp`。
- 构建目录固定为 `cmake-build-debug`。
- CMake 保持简单，优先根 `CMakeLists.txt` 加 `src/CMakeLists.txt`，不要引入不必要的复杂模块化写法。
- 中文注释可以保留；新增注释要简短直接，只解释职责或当前这小段逻辑。
- SQL 脚本放在 `db/migrations/` 和 `db/seeds/`，不要继续堆在 `design-flow/`。
- 头文件引用按 `src` 根来写，例如 `#include "common/error/ErrorCode.h"`。
- 组装 JSON 时优先直接赋值，不为了整数类型额外包 `Json::Int64(...)`。
- Repository 内部的 SQL 常量优先写成 `static const std::string`，不要继续写 `constexpr std::string_view` 再在调用点临时构造 `std::string(...)`。
- 当同一模块的 `service/repository` 头文件同时依赖多个 `dto/command` 和 `dto/view` 时，允许增加一个聚合头，例如 `modules/project/dto/ProjectDto.h`，后续优先直接 include 这个聚合头。

## Naming And Structure

- 当前模块本地命名空间不额外起别名，优先直接写 `repository::`、`service::`、`dto::view::` 这类当前作用域下可直接访问的名字。
- 本地模块内部同样优先直接写 `dto::view::`、`dto::command::`，不要为了缩短名字额外起 `template_view` 这类本地别名。
- 如果目标字段本身就是 `std::optional`，优先直接把解析结果写入目标字段，例如直接写 `query.systemRole`；不要额外创建一个临时 `std::optional` 再二次赋值，除非这样做明显更清晰。
- 只有跨模块且全路径明显影响可读性时，才考虑保留一个必要别名。
- `common` 这类全局公共命名空间可以继续写成：
- `namespace error = project_tracker::common::error;`
- 像 `modules::user::domain` 这类跨模块但反复出现的命名空间，允许保留一个简短别名，例如：
- `namespace user_domain = modules::user::domain;`
- 当前项目偏好简单实现，避免为了“抽象完整”引入多余 DTO、包装器、匿名辅助层。
- 对“简单纯读接口”允许直接走 `Controller -> Repository`，不要为了形式对称强行补一层 service。
- 对带业务规则的写接口，优先走 `Controller -> Service -> Repository`；不要把跨字段校验、权限判断、状态流转规则直接压进 controller。
- 当前项目若采用事务驱动的 repository，优先让 repository 显式接收 `SqlExecutorPtr` 一类执行器；写接口由 `Service` 传事务并编排多步原子动作，简单读接口由 `Controller` 传普通 `DbClient`。
- 不要在 `Controller` 中串多个 SQL 做业务判断；涉及多步写前检查时，优先下沉到 `Service` 并放进同一个事务。
- 像 `/api/auth/me` 这种只按 session 查询单条用户信息的接口，可以直接在 controller 中访问 repository。
- 如果某个辅助函数只在一个 `.cpp` 文件里局部使用，可以放匿名命名空间；如果反而让代码跳转变多，就直接内联到主函数中。
- repository 公开写方法优先一条 SQL 对应一个原子动作；读侧 repository 可以直接承接完整查询逻辑，不必为了形式把同一个读操作强拆到上层。
- 对分页列表这类读接口，不必机械要求“一个公开 repository 方法只含一条 SQL”；如果 `count + list` 本质上属于同一个读操作，允许在一个 repository 方法里用单条 CTE SQL 统一完成，优先保证结果一致性和筛选条件只写一遍。

## Write-Side Concurrency

- 对写接口中的“先查再改”场景，优先把检查拆成 `...ForUpdate(...)` 一类带锁检查方法，不要继续用布尔参数在同一个方法里切换是否锁行。
- 当写接口依赖多个资源时，例如同时依赖 `project` 和 `target_user`，优先拆成多个明确的带锁检查方法，而不是继续保留一个大而全的组合检查查询。
- 这类带锁检查若只表达“资源是否存在”，优先返回 `std::optional<CheckResult>`，不要在结果结构里继续保留 `projectExists`、`targetUserExists` 这类存在性布尔字段。
- 对“自动补关系”这类附带写动作，优先提供 `ensureXxxExists(...)` 幂等写方法，并在 SQL 中使用 `ON CONFLICT ... DO NOTHING`；不要直接污染原本表达“新增语义”的 `insertXxx(...)` 方法。
- 如果写接口在 `Service` 中已经做了前置带锁检查，后续 repository 写 SQL 仍可保留必要的条件保护，例如把 `status <> 3` 这类并发窗口保护继续压在 `WHERE` 条件里；但不要再把完整业务判断重新堆回 `Controller`。
- PostgreSQL 默认隔离级别是 `READ COMMITTED`，不要把“放进同一个事务”误当成“多个 `SELECT` 天然读到同一快照”；如果两次读取之间不允许漂移，优先改成单条 SQL，或者明确使用带锁检查来定义一致性边界。
- 前置检查只适合提前拒绝“被当前锁边界稳定住的条件”，例如项目本身是否存在、操作者是否有权限、项目状态是否已完成；如果条件依赖的是未被一并锁住的子表集合或聚合结果，例如“是否还有节点”“节点是否都已完成”，就不要在 `Service` 里基于这次读取结果先返回更具体的 `409`。
- 对这类会随并发变化的集合条件，优先把最终判断压进最后那条原子 `UPDATE ... WHERE ... RETURNING`；`Service` 可以只在写返回空后统一映射成“当前不满足条件”，不要让前置读取的旧结果抢先决定响应语义。
- Drogon 的 `newTransactionCoro()` 返回事务对象后，事务会绑定在该连接上；不要假设“抛异常后一定自动按业务意图回滚”。对“先 `FOR UPDATE` 再校验，校验失败直接结束”的场景，如果后续还没有真正执行写 SQL，这次事务收尾本质只是释放锁；但一旦已经执行过写 SQL，又希望整段放弃，就优先显式 `transaction->rollback()`，不要只依赖作用域结束来隐式收尾。
- 对“多段写但语义要求整段全有或全无”的场景，不要按“第一段成功 / 前两段成功 / 前三段成功”去枚举状态；通常只需要区分“尚未写入”与“已经发生过至少一次写入”，并在第一条写 SQL 之后用一个类似 `hasWrittenBeforeFinish` 的布尔标记统一决定是否整段 `rollback()`。
- 能重排流程时，优先写成“先全部读取和校验，再连续执行多段写”；尽量不要让第一条写之后还夹着新的业务校验。只有当不同写阶段失败后需要保留前半段结果、或需要做不同补偿动作时，才考虑更细的阶段状态或 `SAVEPOINT`；当前项目默认优先整段事务回滚，而不是维护多阶段半成功状态。

## Data And Enum Rules

- 如果数据库已有 `CHECK` 约束，应用层不重复做同一层枚举合法性兜底，优先直接映射。
- 但如果请求体里的整数会先在 controller 转成 enum 再向下传递，controller 需要先做枚举范围校验，避免非法底层值一路落到数据库并被映射成 `500`。
- 对中国区固定业务时间，可以明确按 `Asia/Shanghai` 处理。
- 若返回时间字符串，优先生成接口最终需要的格式，不要保留中间态再二次加工。
- 即便正常链路里“理论上一定存在”，repository 仍可用 `std::optional` 显式表达“数据库可能查不到”这一事实，例如 session 残留或用户记录已不存在的情况。
- 分页相关的 `page`、`page_size`、`total` 优先使用 `std::int64_t`，和 PostgreSQL 的 `COUNT(*)`、`LIMIT`、`OFFSET` 保持一致。
- 如果分页读接口既要返回 `total` 又要返回当前页列表，并且希望空页时仍能拿到 `total`，优先考虑 `WITH filtered / total / paged` 再 `LEFT JOIN` 的单条 SQL 结构，而不是依赖两次查询或单纯 `COUNT(*) OVER()`。
- 如果 SQL 参数一边和 `BIGINT` 列比较，一边又和 `0` 这类整数字面量比较，要显式写成 `0::bigint` 或 `$n::bigint`，避免 PostgreSQL 把参数先推断成 `integer`，再和 Drogon 传入的 `std::int64_t` 二进制绑定格式冲突。
- 类似 `($3 = 0 OR p.owner_user_id = $3)` 这种写法在 `owner_user_id` 为 `BIGINT` 时有踩坑风险；优先写成 `($3 = 0::bigint OR p.owner_user_id = $3)`。
- 对本地模块内跨层复用的简单输入模型，优先放在 `dto/command/`，避免同一条简单输入在 service / repository 重复定义。
- 如果多个“状态动作接口”输入结构完全一致，例如项目的 `start / complete / reopen`，优先合并成一个共享输入模型，例如 `ProjectStatusActionInput`，不要继续为每个动作保留重复 DTO。
- 对按北京时间判断是否延期的 SQL，优先显式写成 `(NOW() AT TIME ZONE 'Asia/Shanghai')::date` 再和 `DATE` 列比较；必要时加一行短注释说明不是直接拿 session 时区取 `NOW()::date`。

## Error Handling Rules

- `BusinessException` 只用于表达明确的 HTTP 业务失败，例如 `400 / 401 / 403 / 404 / 409`。
- `Controller` 是 HTTP 边界；只要调用链中可能抛 `BusinessException`，`try/catch` 就必须覆盖整段请求处理流程，而不是只包某个 `co_await`。
- `Service` 可以抛 `BusinessException`，用于业务规则不满足、权限失败、状态冲突等场景。
- `Repository` 不用异常表达“正常查不到”；这类情况优先返回 `std::optional`。
- 但如果 `UPDATE ... WHERE` 或 `SELECT ... WHERE` 已经把“状态不允许 / 权限不满足”这类业务限制压进 SQL 条件，`std::optional` 为空可能同时包含多种原因；这种情况下要么在 `Service` 预查并细分错误语义，要么至少在代码注释里明确当前空结果的几种来源。
- 对单条写 SQL 可直接表达的唯一冲突，优先使用 PostgreSQL 原子冲突处理，例如 `ON CONFLICT ... DO NOTHING RETURNING`，再由空结果映射 `409`，不要优先依赖 Drogon 细分数据库异常类型的捕获。
- 事务用于保障多条 SQL 的原子性，不要只为了识别单条唯一约束冲突就引入事务。
- `Repository` 只在数据库异常、底层调用失败等系统错误场景，把异常转成 `InternalError`。
- `common/util` 默认不直接抛 `BusinessException`，优先返回值、`std::optional` 或显式失败态，让 `Controller / Service` 决定如何返回 HTTP 错误。
- `filters` 优先直接返回 `api::fail(...)`，不要依赖抛异常完成拦截。
- 对 query 参数这类基础输入，优先只做格式校验；如果数据库已有 `CHECK` 约束，不重复在 controller 里做同层枚举兜底。

## Validation Boundary

- `Controller` 负责 HTTP 边界校验：是否登录、请求体是否为 JSON 对象、字段是否存在、字段类型是否正确、单字段基础格式是否正确。
- `Service` 负责业务规则校验：跨字段约束、权限判断、状态流转、唯一性或冲突规则，不重复做 controller 已经完成的单字段存在性 / 基础格式校验。
- 例如创建项目时，`name` 是否非空、日期字符串是否为 `YYYY-MM-DD` 放在 `Controller`；`planned_start_date <= planned_end_date` 这种跨字段业务约束放在 `Service`。

## Filter And Test Notes

- 登录态过滤器放在 `src/filters/`，当前已有 `LoginRequiredFilter`。
- 新增受保护接口时，优先先挂 filter，再补业务实现，避免 controller 里先散落未登录判断。
- 手工联调用例放在 `tests/http/`。
- `.http` 文件按接口粒度拆分，不按整模块长期混放；例如 `listUsers.http` 这种命名优于宽泛的 `users.http`。
- JetBrains / CLion 的 `.http` 文件优先保持最简单形式，默认信任 IDE 自动保存和回放 Cookie，不手写 `Set-Cookie` 解析脚本，除非确实需要。
- `.http` 用例的顺序优先写成“正常路径在前，异常路径在后”，便于人工顺序点击验证。
- 对会直接修改数据库状态的正常路径，优先把 `.http` 用例写成可重复回放：先创建临时资源，再继续开始/完成/删除等动作；如果需要承接上一步响应里的 ID，允许使用 JetBrains HTTP client 的 `client.global.set(...)` 保存变量。
- 如果某条异常路径依赖种子数据中的固定项目或固定状态，要在 `.http` 注释里明确写出这一前提；如果能通过“先创建再操作”改造成可重复用例，优先改成可重复。

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
