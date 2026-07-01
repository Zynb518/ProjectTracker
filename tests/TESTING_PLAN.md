# Project Tracker 项目测试计划书

本项目是一个基于 Drogon C++ 框架构建的项目协作与追踪系统（包含用户、项目、成员、任务节点、AI大纲生成以及设备日志等模块）。为了保证项目平稳上线并能够进行持续的回归验证，特制定本测试计划。

---

## 1. 测试策略与分层架构

测试分为 **代码级测试（GTest）** 与 **接口业务级测试（.http）**，两层相互补充：

```
                    ┌─────────────────────────┐
                    │      E2E 业务流测试       │ ◄─── .http 文件 + httpyac 自动化断言
                    │   (全系统级集成，连 DB)    │
                    ├─────────────────────────┤
                    │    服务与数据库集成测试    │ ◄─── Google Test (Integration Targets)
                    │  (Repository / Service) │
                    ├─────────────────────────┤
                    │        单元测试          │ ◄─── Google Test (Unit Targets)
                    │ (纯业务逻辑, Mock 依赖)   │
                    └─────────────────────────┘
```

* **单元测试（Unit Tests - GTest）**：针对无状态、无外部依赖的纯代码逻辑、辅助工具类进行快速测试。
* **集成测试（Integration Tests - GTest）**：验证 Service 与 Repository 之间的交互、SQL 查询的正确性以及数据库触发规则。
* **API 与场景流测试（E2E Tests - .http）**：以黑盒视角通过 HTTP 请求验证系统的端到端契约（API Contract）和核心业务链路（如登录-建项目-指派任务-提交进度）。

---

## 2. 核心模块测试规划

依据项目现有的模块结构，测试点规划如下：

### 2.1 用户与认证模块 (User & Auth)
* **GTest 单元测试**：
  * 验证密码 Argon2 哈希计算与比对函数。
  * 密码复杂度校验规则测试（如长度限制、字符限制）。
* **GTest 集成测试**：
  * 用户注册：重名检测、注册信息合法性校验。
  * 用户基本信息更新与状态管理。
* **.http 接口与场景测试**：
  * 登录场景：验证正确用户名/密码返回 Token，错误用户名/密码返回 `401/400`。
  * 登录态维持：使用 Token 访问 `/api/auth/me` 获取当前用户信息。
  * 登出场景：调用 `/api/auth/logout` 销毁 Token，并验证之后无法再使用该 Token 访问受保护接口。

### 2.2 项目管理模块 (Project)
* **GTest 单元测试**：
  * 项目状态机流转规则测试（Draft 草稿 -> In Progress 进行中 -> Completed 已完成 -> Reopened 重新打开）。
* **GTest 集成测试**：
  * 项目创建：默认所有者指派、基本信息录入。
  * 转移所有者（Transfer Owner）：验证新老所有者的转移合法性、移交后的候选人名单更新。
  * 状态流转限制：不允许将未启动的项目直接置为完成状态。
* **.http 接口与场景测试**：
  * 完整项目生命周期链路测试：
    1. 创建项目 (Draft) ➔ 2. 开启项目 ➔ 3. 各种修改 ➔ 4. 完成项目 ➔ 5. 重新开启项目。

### 2.3 项目成员模块 (Project Member)
* **GTest 集成测试**：
  * 添加成员逻辑：防止重复添加同一个用户。
  * 权限划分验证：只有项目所有者（Owner）或管理员能够添加/删除成员。
* **.http 接口与场景测试**：
  * 团队协作链路测试：
    1. 查询成员候选人列表 ➔ 2. 添加小组成员 ➔ 3. 验证成员已被添加 ➔ 4. 移除小组成员。

### 2.4 节点与任务模块 (Node & Task)
* **GTest 单元测试**：
  * 任务进度（0% - 100%）边界值输入验证。
  * 节点状态与任务关联判定算法。
* **GTest 集成测试**：
  * 创建任务节点与应用节点模板（Node Template）。
  * 任务的添加、删除、指派逻辑。
  * 提交任务进度：校验累计的进度历史记录，确认节点与项目总进度被正确触发和级联更新。
* **.http 接口与场景测试**：
  * 项目计划及实施链路测试：
    1. 应用节点模板 ➔ 2. 指派具体 Task 并提交进度 ➔ 3. 获取项目的甘特图（Gantt Chart）数据 ➔ 4. 验证甘特图中的节点和成员列表符合预期。

### 2.5 设备日志模块 (Device Log)
* **GTest 集成测试**：
  * 录入设备使用日志，过滤查询特定日期或设备型号 of 日志列表。
* **.http 接口与场景测试**：
  * 注册设备 ➔ 创建使用日志 ➔ 列表查询 ➔ 删除设备。

---

## 3. 测试自动化流水线规划 (CI/CD)

通过 Docker 容器化方案，构建一条完整的自动化回归门禁：

```
┌──────────────┐      ┌─────────────────────────┐      ┌────────────────────────┐
│  代码库更新    │ ───► │  Docker tests 编译运行   │ ───► │   httpyac 接口回归测试  │
└──────────────┘      │   (跑 GTest 单元/集成)    │      │     (跑 .http 断言)    │
                      └─────────────────────────┘      └────────────────────────┘
```

1. **第一关：代码编译与 C++ 级测试**
   * 执行 `docker compose run --rm tests`。
   * 它会在独立的 `docker-build-debug` 中编译所有测试，并执行 `Project_Tracker_UnitTests` 与 `Project_Tracker_IntegrationTests`。
2. **第二关：API 回归与契约测试**
   * 我们可以在 `tests/` 目录下集成基于 NodeJS 的 `httpyac` CLI，对项目原有的 50+ 个 `.http` 请求执行自动化校验（校验返回状态码、JSON 返回值等）。
3. **安全审计与漏扫（Penetration Sandbox）**
   * 采用模拟工具或脚本检测 API 的未授权访问（如绕过 Token 访问受保护接口）。

---

## 4. 资源泄露与并发安全检测 (Resource Leak & Concurrency Safety)

由于本项目后端采用 C++ 开发，并且基于高并发、多线程异步的 Drogon 框架，内存管理与线程安全至关重要。因此，测试计划引入以下三类工具进行资源泄露与安全检测：

### 4.1 AddressSanitizer (ASan) & LeakSanitizer (LSan)
* **检测目标**：动态捕获内存越界（Buffer Overflow）、使用已释放内存（Use-After-Free）、内存泄漏（Memory Leaks）等常见 C++ 内存安全问题。
* **执行方式**：
  * 在编译期注入。在 CMake 中，可以通过在编译选项中加入 `-fsanitize=address` 启用（LSan 在 GCC/Clang 下作为 ASan 的一部分默认开启）。
  * 建议在调试模式（Debug Build）或专门的 CI 安全流水线中开启，例如在 [tests/CMakeLists.txt](file:///home/ubzy/CLionProjects/Project-Tracker/tests/CMakeLists.txt) 中为测试目标添加：
    ```cmake
    target_compile_options(Project_Tracker_UnitTests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(Project_Tracker_UnitTests PRIVATE -fsanitize=address)
    ```
* **特点**：速度极快（相较于 Valgrind，性能损耗通常仅为 2x 左右），非常适合与单元测试/集成测试结合在 CI 中频繁运行。

### 4.2 ThreadSanitizer (TSan)
* **检测目标**：检测多线程环境下的数据竞争（Data Race）。Drogon 使用多线程 EventLoop 模式，若在多个线程间共享临界资源（如未加锁的 std::shared_ptr、自定义缓存、Service 中的共享状态等）且未进行正确同步，TSan 会在运行时直接报错。
* **执行方式**：
  * 在编译选项中加入 `-fsanitize=thread`。
  * **注意**：ASan 与 TSan 互斥，不能在同一次编译中同时开启。需要配置独立的 TSan 编译通道，建议在本地专项排查或专属 CI Job 中运行测试。
    ```cmake
    target_compile_options(Project_Tracker_UnitTests PRIVATE -fsanitize=thread)
    target_link_options(Project_Tracker_UnitTests PRIVATE -fsanitize=thread)
    ```

### 4.3 Valgrind (Memcheck + Track-FDs)
* **检测目标**：
  * **内存泄漏 (Memcheck)**：更细粒度的内存分析，无需重新编译即可分析任何二进制程序（但推荐带 `-g` 调试信息）。
  * **描述符泄露 (Track FDs)**：高并发后端容易发生套接字（Socket）或文件描述符（FD）打开后未正确关闭（资源泄露），最终导致句柄耗尽（`EMFILE: Too many open files`）。
* **执行方式**：
  * 运行集成测试或启动服务实例时，使用 valgrind 包装：
    ```bash
    valgrind --tool=memcheck --leak-check=full --track-origins=yes --track-fds=yes ./Project_Tracker_IntegrationTests
    ```
* **特点**：由于采用指令集翻译模拟，运行速度慢（通常变慢 10x-50x），但能捕获到极其隐蔽的文件描述符泄露，是测试后期和发布前必不可少的专项检测工具。

---

## 5. 性能与并发压力测试 (Performance & Concurrency Load Testing)

由于 Drogon 框架以极高的吞吐能力著称，后端接口在面临真实高并发时，需要有量化的性能指标支撑。

### 5.1 压测工具选型
* **wrk**：基于 epoll 的极简高性能 HTTP 压测工具，适合对单个或多个固定端点进行超高并发的吞吐量压测。
* **hey**：Go 语言编写的现代命令行压测工具，支持设置请求上限、指定并发数，输出漂亮的直方图和百分位数延迟（如 p95/p99）。

### 5.2 核心压测场景
1. **静态/轻量接口 (如 Health Check)**：测试框架底层处理上限，验证网络协议栈与 Drogon EventLoop 的极致性能。
2. **核心业务读接口 (如获取项目详情/任务列表)**：结合数据库查询，测试在一定并发下（如 500/1000 并发连接）系统的吞吐量（RPS）与响应耗时变化曲线。
3. **混合读写场景 (如登录后创建任务、更新状态)**：模拟真实用户的高频写入操作，验证数据库连接池（Drogon DB Client Pool）是否存在死锁或排队延迟，以及数据库在高频并发事务下的稳定性和数据一致性。

### 5.3 关键监控指标与阈值
* **吞吐量 (Throughput)**：系统在不同并发等级下能达到的最高每秒请求数 (RPS)。
* **响应延迟 (Latency)**：核心接口平均延迟控制在 < 50ms，且 p99 延迟控制在 < 200ms 以内。
* **资源使用率 (System Resource Usage)**：CPU 使用率曲线、内存平稳性（无阶梯式上涨，验证无泄漏）、网络带宽饱和度，以及系统的 FD (File Descriptor) 计数监控。
