# 项目进度跟踪软件后端架构设计

## 1. 文档说明

本文档用于记录当前阶段已确认的后端架构方案，作为后续 Drogon 项目目录设计、模块拆分、控制器实现、服务层编排与 PostgreSQL 持久化实现的统一依据。

当前架构决策以“先把单体边界做清楚”为目标，不追求过早抽象，也不引入当前版本暂时不需要的复杂模式。

---

## 2. 当前已确认的架构结论

### 2.1 系统总体形态

- 当前采用 `模块化单体` 架构
- 单体内部按业务模块划分，而不是只按全局技术层划分
- 每个模块内部保留 `controller / service / repository` 的经典目录命名
- 目录内部的类名与文件名采用更具体的职责命名，例如 `TaskCommandService`、`TaskReadService`
- 数据存储当前固定为 `PostgreSQL`
- 当前阶段不做 `Repository` 接口抽象，不优先考虑依赖倒置

### 2.2 模块内部的基本分层

模块内部仍然遵循经典分层思想，但读写链路允许非对称设计。

- `Controller`：负责 HTTP 接口接入、请求解析、参数校验、响应组织
- `CommandService`：负责命令型业务编排、规则处理、事务边界
- `ReadService`：负责复杂读场景的查询聚合、权限裁剪与读模型组织，可按需省略
- `Repository / ReadRepository`：负责 PostgreSQL 持久化、SQL 与结果映射

### 2.3 读写分离的调用链规则

当前约定如下：

- 写接口：`Controller -> <Module>CommandService -> <Module>Repository`
- 复杂读接口：`Controller -> <Module>ReadService -> <Module>ReadRepository`
- 简单纯读接口：`Controller -> <Module>ReadRepository`

模块级目录补充说明：

- 写接口包含新增、删除、修改、恢复、状态流转、绑定解绑、批量更新等所有会改变系统状态的接口
- 查询接口允许绕过 `ReadService`，但前提是该接口仅做只读查询，不包含事务、状态变更、规则编排或跨仓储协调
- 读侧和写侧不要求强行对称，优先保证职责合理，而不是形式整齐

---

## 3. 为什么选择“模块化单体 + 模块内经典分层”

该方案适合当前项目阶段，原因如下：

- 比纯全局三层目录更适合业务增长，模块边界更清晰
- 比一开始引入 Clean Architecture、Ports and Adapters 成本更低
- 与 Drogon 的控制器开发方式兼容，落地阻力小
- 固定 PostgreSQL 后，可以先聚焦业务规则与 SQL 质量，而不是过早处理多实现切换
- 允许命令与查询采用不同复杂度的链路，避免出现大量空壳 `ReadService`

当前阶段的重点不是追求“最纯粹”的架构术语，而是建立稳定、可维护、可演进的代码组织方式。

---

## 4. 项目目录建议

### 4.1 项目级目录

```text
Project-Tracker/
├─ CMakeLists.txt               # 顶层构建入口，声明项目、依赖与子目录
├─ cmake/                       # 自定义 CMake 模块、工具函数与编译选项
├─ config/                      # 配置文件目录
│  ├─ config.example.json       # 配置模板，供开发与部署参考
│  └─ local/                    # 本地环境专用配置，不建议提交敏感信息
├─ db/                          # 数据库相关资源
│  ├─ migrations/               # PostgreSQL 建表、改表、索引变更脚本
│  └─ seeds/                    # 初始化演示数据或测试数据脚本
├─ design-flow/                 # 需求、ER 图、接口、架构等设计文档
├─ docs/                        # 开发说明、部署说明、运维说明等补充文档
├─ src/                         # 后端业务源码主目录
│  ├─ bootstrap/                # 应用启动、配置装载、组件初始化、路由装配
│  ├─ filters/                  # 鉴权、Trace、限流等横切过滤器
│  ├─ common/                   # 全局通用能力，如错误码、日志、工具、基础类型
│  └─ modules/                  # 业务模块目录，每个模块独立组织自己的分层代码
│     ├─ auth/                  # 认证模块，如登录、登出、当前用户信息
│     ├─ user/                  # 用户模块，如用户管理、状态维护
│     ├─ project/               # 项目模块，如项目增删改查与状态管理
│     ├─ project_member/        # 项目成员模块，如添加成员、移除成员、成员列表
│     ├─ phase/                 # 阶段节点模块，如阶段顺序、时间区间、状态管理
│     └─ task/                  # 子任务模块，如任务分配、进度更新、查询视图
├─ tests/                       # 测试代码目录
│  ├─ unit/                     # 单元测试，主要覆盖规则与服务逻辑
│  ├─ integration/              # 集成测试，主要覆盖 DB、Repository、接口联动
│  └─ e2e/                      # 端到端测试，覆盖关键业务流程
└─ scripts/                     # 本地开发、初始化、辅助检查脚本
```

### 4.2 模块级目录模板

以 `task` 模块为例：

```text
src/modules/task/
├─ controller/                  # HTTP 接入层，处理路由、参数解析、响应返回
│  ├─ TaskController.h          # 控制器声明与 Drogon 接口定义
│  └─ TaskController.cc         # 请求校验、调用 TaskCommandService/TaskReadService/TaskReadRepository、返回统一响应
├─ service/                     # 业务编排层，存放具体职责服务，如命令服务与读服务
│  ├─ TaskCommandService.h      # 命令型服务声明，如新增、修改、删除、状态流转
│  ├─ TaskCommandService.cc     # 命令型业务规则、事务边界、仓储协作实现
│  ├─ TaskReadService.h         # 复杂读场景服务声明，可按需省略
│  └─ TaskReadService.cc        # 复杂查询聚合、权限裁剪、读模型组织实现
├─ repository/                  # PostgreSQL 持久化层，封装写入 SQL 与读取 SQL
│  ├─ TaskRepository.h          # 写仓储声明，服务于命令型接口
│  ├─ TaskRepository.cc         # 写仓储实现，处理插入、更新、删除等 SQL
│  ├─ TaskReadRepository.h      # 读仓储声明，服务于列表、详情、统计查询
│  └─ TaskReadRepository.cc     # 读仓储实现，处理分页、筛选、聚合类 SQL
├─ dto/                         # 模块内部共享的数据对象，按语义组织而非按层复制
│  ├─ request/                  # 接近 HTTP 输入的请求对象
│  │  ├─ CreateTaskRequest.h    # 创建任务接口请求体
│  │  ├─ UpdateTaskRequest.h    # 更新任务接口请求体
│  │  └─ ListTaskRequest.h      # 列表查询接口请求参数
│  ├─ command/                  # 命令型输入对象，供 TaskCommandService/TaskRepository 共享
│  │  ├─ CreateTaskCommand.h    # 创建任务命令对象
│  │  ├─ UpdateTaskCommand.h    # 更新任务命令对象
│  │  └─ ChangeTaskStatusCommand.h # 任务状态流转命令对象
│  ├─ query/                    # 查询输入对象，描述筛选条件与分页条件
│  │  ├─ ListTaskQuery.h        # 任务列表查询对象
│  │  └─ GetTaskDetailQuery.h   # 任务详情查询对象
│  └─ view/                     # 读模型/返回模型，服务于 TaskReadService、TaskReadRepository 与 Controller 响应输出
│     ├─ TaskDetailDTO.h        # 任务详情展示对象
│     ├─ TaskSummaryDTO.h       # 列表项摘要对象
│     └─ TaskPageDTO.h          # 分页结果对象
└─ domain/                      # 模块内核心业务概念与轻量规则
   ├─ Task.h                    # 任务实体定义
   └─ TaskStatus.h              # 任务状态枚举与相关约束
```

### 4.3 纯目录版

仅保留目录层级，不包含文件名，但保留目录职责注释，方便单独确认结构形态。

项目级目录：

```text
Project-Tracker/
├─ cmake/                       # 自定义 CMake 模块与构建辅助逻辑
├─ config/                      # 配置文件目录
│  └─ local/                    # 本地环境专用配置
├─ db/                          # 数据库脚本目录
│  ├─ migrations/               # 建表、改表、索引等迁移脚本
│  └─ seeds/                    # 初始化或测试数据脚本
├─ design-flow/                 # 当前项目的设计过程文档
├─ docs/                        # 补充性的开发、部署、运维文档
├─ src/                         # 后端源码主目录
│  ├─ bootstrap/                # 应用启动与初始化逻辑
│  ├─ filters/                  # 鉴权、Trace、限流等过滤器
│  ├─ common/                   # 全局通用能力与公共类型
│  └─ modules/                  # 业务模块目录
│     ├─ auth/                  # 认证相关业务
│     ├─ user/                  # 用户管理相关业务
│     ├─ project/               # 项目主体相关业务
│     ├─ project_member/        # 项目成员关系相关业务
│     ├─ phase/                 # 阶段节点相关业务
│     └─ task/                  # 子任务与进度相关业务
├─ tests/                       # 测试代码目录
│  ├─ unit/                     # 单元测试
│  ├─ integration/              # 集成测试
│  └─ e2e/                      # 端到端测试
└─ scripts/                     # 辅助脚本目录
```

模块级目录：

```text
src/modules/task/
├─ controller/                  # HTTP 接入层，处理路由、参数与响应
├─ service/                     # 业务编排层，存放 TaskCommandService、TaskReadService 等具体服务
├─ repository/                  # PostgreSQL 持久化层，存放 TaskRepository、TaskReadRepository 等仓储
├─ dto/                         # 模块内部共享的数据对象目录
│  ├─ request/                  # 接近 HTTP 输入的请求对象
│  ├─ command/                  # 命令型输入对象
│  ├─ query/                    # 查询条件对象
│  └─ view/                     # 读模型与返回模型
└─ domain/                      # 模块内核心业务概念与轻量规则
```

### 4.4 `common/` 目录建议

以下目录仅作为当前阶段的克制版建议，供开发过程中参考使用。

实际开发时可以根据复用情况灵活调整：

- 如果某类对象暂时没有明确复用价值，可以先不拆目录
- 如果某目录长期只承载 1 到 2 个文件，也可以先并入相邻目录
- 只有当多个模块稳定复用时，才建议抽入 `common/`

推荐结构：

```text
src/common/
├─ api/                         # 统一响应包装、通用响应结构
├─ context/                     # 当前用户、请求上下文、Trace 等横切上下文
├─ error/                       # 错误码、业务异常、字段错误信息
├─ page/                        # 分页请求、分页结果、分页元信息
├─ types/                       # 全局基础类型、小型稳定值对象、类型别名
├─ util/                        # 无业务语义的纯工具函数
└─ validation/                  # 通用校验结果与校验辅助结构
```

说明：

- 当前不建议额外建立一个大的 `common/dto/` 目录
- 当前不建议把模块私有的业务 DTO 提前抽到 `common/`
- `common/` 应优先放“横切型、跨模块、语义稳定”的对象

模块级目录补充说明：

- `controller/` 保持 HTTP 语义
- `service/` 目录建议使用明确职责命名，如 `TaskCommandService`、`TaskReadService`
- `repository/` 建议区分 `TaskRepository` 与 `TaskReadRepository`，避免所有 SQL 长期堆进同一个类
- `dto/` 放模块内部共享的数据对象，不要求每一层都新建一份
- `domain/` 当前阶段保持轻量，只承载领域枚举、实体和局部规则

---

## 5. 各层职责边界

### 5.1 Controller

`Controller` 的职责：

- 处理路由与 HTTP 方法
- 解析 URL 参数、Query 参数、JSON Body
- 做基础参数校验与格式校验
- 将 HTTP 请求对象转换为模块内部可用的输入对象
- 调用 `CommandService`、`ReadService` 或 `ReadRepository`
- 将结果转换为统一响应结构

`Controller` 不负责：

- 持久化访问
- SQL 组织
- 跨对象业务规则编排
- 事务处理

### 5.2 CommandService

`CommandService` 的职责主要面向写操作。

实际命名建议采用模块化的具体名称，例如：

- `TaskCommandService`
- `ProjectCommandService`
- `PhaseCommandService`

其职责包括：

- 承载新增、删除、修改、状态流转等命令型用例
- 做业务规则编排
- 管理事务边界
- 协调多个 `Repository`
- 注入上下文信息，例如当前用户、默认值、审计字段

`CommandService` 不应该退化成：

- 只做无意义参数转发的形式层
- 直接暴露 HTTP 类型
- 直接向上返回数据库底层对象

### 5.3 ReadService

`ReadService` 不是强制层。

实际命名建议采用模块化的具体名称，例如：

- `TaskReadService`
- `ProjectReadService`
- `PhaseReadService`

它仅在复杂查询存在以下需求时引入：

- 需要组合多个查询源
- 需要做读权限裁剪
- 需要做缓存、聚合或降级
- 需要对查询条件做业务化转换

如果查询接口只是简单列表、详情、基础筛选，则允许直接：

`Controller -> <Module>ReadRepository`

### 5.4 Repository

`Repository` 负责：

- 处理 PostgreSQL 读写
- 组织 SQL 或调用 Drogon ORM / DbClient
- 完成数据库结果映射
- 向上返回模块级数据对象，而不是底层数据库行对象

建议尽早区分：

- `Repository`：命令侧仓储
- `ReadRepository`：查询侧仓储

原因是读写 SQL 形态通常差异很大，强行合并容易使仓储类快速膨胀。

---

## 6. DTO 与参数传递设计规则

### 6.1 核心原则

当前项目不采用“每一层都必须重新定义一份结构体”的做法，而采用以下原则：

**每一种语义只定义一个对象。**

换句话说：

- 语义相同，可以复用同一个对象
- 语义变化，才值得做对象转换

### 6.2 什么时候可以复用同一个对象

当以下条件成立时，可以让 `Controller -> <具体服务> -> <具体仓储>` 复用同一个对象：

- 对象不包含 HTTP 框架类型
- 对象只表达业务输入，不带接入层痕迹
- `<具体服务>` 不需要对其做语义转换
- `Repository` 需要的字段集合与业务输入一致

例如：

```cpp
struct ListTaskQuery {
    int project_id;
    std::optional<int> assignee_id;
    int page;
    int page_size;
};
```

如果该查询对象对三层来说语义一致，则可以直接复用，不必机械地拆成三套 DTO。

### 6.3 什么时候必须转换

如果出现以下情况，则应该明确转换对象，而不是为了节省一次拷贝强行复用：

- `Controller` 输入仍带有 HTTP 语义
- `<具体服务>` 需要补默认值、做规则校验、做状态转换
- `Repository` 需要的是数据库视角的查询条件
- 写操作需要补审计字段、操作者 ID、版本号、时间戳等字段

例如：

- 前端传入 `keyword`
- `<具体服务>` 将其规范化为模糊查询模式
- `<具体仓储>` 实际接收 `keyword_pattern`

这属于语义变化，应显式转换。

### 6.4 返回值设计

对于返回值，当前约定如下：

- 查询接口：`ReadRepository` 返回值和 `ReadService` 返回值可以尽量保持一致
- 写接口：`CommandService` 返回值应面向用例语义设计，不必机械复用 `Repository` 返回值

示例：

- 查询接口可以直接返回 `TaskPageDTO`
- 创建接口中，`Repository` 返回主键，`CommandService` 再决定是否返回 `CreateTaskResult`

即：

- 查询侧优先复用读模型
- 写侧优先表达用例结果

---

## 7. 参数传递与拷贝控制建议

当前项目对 C++ 参数传递建议如下：

- 小对象、标量、枚举、ID：按值传递
- 只读查询对象：优先 `const T&`
- 写命令对象：优先按值接收，再向下 `std::move`

示例：

```cpp
TaskPageDTO listTasks(const ListTaskQuery& query);
CreateTaskResult createTask(CreateTaskCommand command);
```

推荐原因：

- 查询对象通常只读，不需要接管所有权
- 写命令对象常常需要补字段、调整状态，再转交下层
- 现代 C++ 的移动语义足以覆盖大部分对象传递成本

需要特别注意：

- 不要为了零拷贝滥用复杂引用链
- 不要让 `string_view` 跨异步边界长期存活
- 不要把数据库行对象或 ORM 对象直接传给上层
- 不要为了“少一次 move”破坏层间语义边界

在 HTTP 后端里，更大的性能成本通常来自：

- 网络 I/O
- JSON 解析
- PostgreSQL 查询

而不是几次合理的对象移动。

---

## 8. 什么情况下可以省略 ReadService

允许省略 `ReadService` 的场景：

- 只读接口
- 无事务
- 无状态变更
- 无权限规则编排
- 无跨仓储协调
- 无聚合和副作用

典型示例：

- 简单分页列表
- 详情查询
- 基础筛选查询

此时可以采用：

`Controller -> <Module>ReadRepository`

需要保留 `CommandService` 的场景：

- 新增、删除、修改
- 状态流转
- 多表一致性写入
- 事务控制
- 审计、通知、事件触发
- 业务规则判断
- 需要多个仓储协作

此时必须采用：

`Controller -> <Module>CommandService -> <Module>Repository`

---

## 9. 当前阶段不做的事情

为了控制复杂度，当前阶段明确不做以下设计：

- 不为 `Repository` 额外定义接口层以支持多存储切换
- 不引入完整的 Clean Architecture / Ports and Adapters
- 不强制所有查询都经过 `ReadService`
- 不要求每层都重新定义一套 DTO
- 不追求理论上的零拷贝而牺牲代码可读性

这些设计并非错误，而是当前版本暂不优先。

---

## 10. 当前推荐的落地原则

后续实现时，统一遵循以下原则：

1. 优先按业务模块组织代码，再在模块内做经典分层。
2. 模块内保留 `controller / service / repository` 目录命名，但类名采用具体职责命名。
3. 写操作固定经过 `<Module>CommandService`，查询操作按复杂度决定是否保留 `<Module>ReadService`。
4. `Repository` 建议区分写仓储和读仓储，避免仓储层膨胀。
5. DTO 按语义建模，而不是按层数机械复制。
6. 查询侧返回值尽量复用，写侧返回值优先表达用例结果。
7. 不为了省少量拷贝破坏层次边界。

---

## 11. 一句话总结

当前项目的后端架构方案是：

**采用模块化单体，在模块内部保留 `controller / service / repository` 目录命名；类名使用 `TaskCommandService`、`TaskReadService`、`TaskReadRepository` 这类更具体的职责命名；写操作固定经过命令服务，读操作按复杂度决定是否经过读服务，并通过语义一致的 DTO 复用来控制对象拷贝与层间转换成本。**
