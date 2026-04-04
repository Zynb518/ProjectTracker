# 项目进度跟踪软件接口设计（V2）

## 1. 文档说明

本文档用于定义项目进度跟踪软件第一版的后端接口设计，作为后续 Drogon 控制器实现、前端页面联调和 Service 层规则落地的依据。

本文档的设计基准如下：

- 业务边界以 `01-requirements-boundary.md` 为准
- 数据结构以 `db/migrations/001_init_schema.sql` 为准
- `03-api-design.md` 仅保留为旧版参考，不再作为现行设计依据

本文档延续“按模块划分接口”的方式组织内容，但会严格遵守当前已经确定的数据库结构和业务规则，不再沿用旧版中已经失效的字段、角色模型和状态处理方式。

## 2. 设计原则与实现前提

### 2.1 设计原则

- 接口以业务动作划分，而不是简单按数据表做一一映射
- 项目、阶段节点、子任务的“基础信息修改”与“状态动作”分离设计
- 已完成对象默认冻结，必须先执行“撤销完成”，再进行编辑
- 项目负责人只有一个，统一由 `project.owner_user_id` 表达
- `project_member` 只表达项目成员关系，不再表达项目内角色
- 子任务进度历史统一通过 `sub_task_progress` 维护
- 第一版默认使用 `Session/Cookie` 维持登录态
- 第一版默认面向企业内部 PC 端网页使用

### 2.2 当前 schema 下的接口约束

以下约束会直接影响接口设计：

- `02` 中没有 `project_type` 字段，因此“个人自用项目 / 团队项目”的差异不对外暴露为独立字段，而由 Service 层按业务规则校验
- `02` 中没有项目级、阶段级进度百分比字段，因此本文档不定义项目和阶段的聚合 `progress_percent` 返回口径
- `02` 中没有项目和阶段的专用历史表，因此项目和阶段的“手动开始信号”需要由应用层业务实现可靠保存；接口层仅定义动作与结果，不限制具体实现手段
- 复杂业务规则不下沉到数据库触发器中，统一由应用层 / Service 层实现

### 2.3 路径与命名约定

- 所有接口统一使用 `/api` 作为前缀
- 资源路径统一使用复数名词，如 `/projects`、`/users`、`/subtasks`
- 字段命名统一对齐 `02`，使用 `snake_case`
- 时间戳字段统一使用 ISO 8601 字符串，示例：`2026-03-27T10:30:00+08:00`
- 日期字段统一使用 `YYYY-MM-DD`，示例：`2026-04-01`
- 所有基于 `DATE` 字段的业务状态判定统一按北京时间（`Asia/Shanghai`）计算

## 3. 统一接口约定

### 3.1 请求格式

- `Content-Type: application/json`
- `GET` 请求不使用请求体
- `DELETE` 请求第一版默认不使用请求体

### 3.2 成功返回结构

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

字段说明：

- `code`：业务状态码，`0` 表示成功
- `message`：结果描述
- `data`：业务数据；无数据时返回 `null`

### 3.3 失败返回结构

```json
{
  "code": 40001,
  "message": "参数错误",
  "data": null
}
```

### 3.4 分页返回结构

分页列表接口统一返回：

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "list": [],
    "total": 0,
    "page": 1,
    "page_size": 10
  }
}
```

### 3.5 常用枚举约定

#### 系统角色 `system_role`

- `1`：管理员
- `2`：项目经理
- `3`：普通员工

#### 用户状态 `status`

- `1`：启用
- `2`：禁用

#### 项目 / 阶段 / 子任务状态 `status`

- `1`：未开始
- `2`：进行中
- `3`：已完成
- `4`：已延期

#### 子任务优先级 `priority`

- `1`：低
- `2`：中
- `3`：高

### 3.6 排序约定

- 用户管理列表默认按 `id DESC`
- 项目列表默认按 `updated_at DESC`
- 阶段节点列表默认按 `sequence_no ASC`
- 子任务进度记录默认按 `created_at DESC, id DESC`

## 4. 权限与状态动作原则

### 4.1 查看权限

- 管理员可查看全部项目、用户、节点、子任务和模板
- 项目负责人可查看并管理自己负责项目中的全部业务数据
- 项目成员可查看自己参与项目中的阶段节点、子任务和相关甘特图数据
- 普通成员只能更新自己负责的子任务进度

### 4.2 写权限

- 用户管理接口默认仅管理员可调用
- 管理员可对团队项目直接执行项目内操作，但仍需遵守统一业务规则
- 对普通员工创建的个人自用项目，管理员仅保留查看和删除权限，不允许执行项目内部修改动作
- 非管理员场景下，只有具备管理员或项目经理身份的项目负责人，才能添加或移除项目成员
- 普通员工创建的个人自用项目不允许新增其他成员，也不允许转交负责人

### 4.3 完成冻结原则

- 项目、阶段节点、子任务进入 `已完成` 后默认完全冻结
- 已完成项目不允许直接修改项目基础信息，也不允许增删改阶段节点、子任务和成员
- 已完成阶段节点不允许直接修改节点基础信息，也不允许增删改其下子任务
- 已完成子任务不允许继续提交进度或修改基础信息
- 如需继续修改，必须先执行“撤销完成”

### 4.4 状态动作分离原则

以下动作不并入普通编辑接口，而使用独立动作接口：

- 项目开始、完成、撤销完成
- 阶段节点开始、完成、撤销完成
- 子任务开始、撤销完成
- 项目负责人转交
- 基于模板批量生成阶段节点

## 5. 常用业务错误码建议

| code | 含义 |
| --- | --- |
| 0 | 成功 |
| 40001 | 参数错误 |
| 40002 | 登录失败 |
| 40003 | 未登录或登录态失效 |
| 40004 | 无权限访问 |
| 40401 | 用户不存在 |
| 40402 | 项目模板不存在 |
| 40403 | 项目不存在 |
| 40404 | 项目成员不存在 |
| 40405 | 阶段节点不存在 |
| 40406 | 子任务不存在 |
| 40901 | 用户名已存在 |
| 40902 | 用户已被禁用 |
| 40903 | 项目成员重复添加 |
| 40904 | 项目已完成，禁止直接修改 |
| 40905 | 阶段节点已完成，禁止直接修改 |
| 40906 | 子任务已完成，禁止直接修改 |
| 40907 | 子任务负责人不属于当前项目成员 |
| 40908 | 项目成员仍存在未完成子任务，不能移除 |
| 40909 | 时间区间不合法或不在父级范围内 |
| 40910 | 状态与进度百分比不匹配 |
| 40911 | 阶段顺序号冲突 |
| 40912 | 已完成阶段节点不允许调整顺序 |
| 40913 | 不允许对个人自用项目执行成员管理 |
| 40914 | 项目当前不满足“完成”条件 |
| 40915 | 阶段节点当前不满足“完成”条件 |
| 40916 | 当前对象不满足“手动开始”条件 |
| 40917 | 撤销完成失败，缺少可回退的业务状态 |
| 40918 | 负责人转交目标不合法 |
| 40919 | 模板节点映射不完整或顺序不合法 |
| 40920 | 项目已存在阶段节点，不能再次整套应用模板 |
| 50000 | 系统内部错误 |

## 6. 接口模块总览

第一版接口按以下 9 个模块组织：

1. 认证接口
2. 用户接口
3. 项目模板接口
4. 项目接口
5. 项目成员接口
6. 阶段节点接口
7. 子任务接口
8. 子任务进度接口
9. 甘特图接口

---

## 7. 认证接口

### 7.1 登录

**Method**

`POST`

**Path**

`/api/auth/login`

**Request Body**

```json
{
  "username": "zhangsan",
  "password": "123456"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `username` | 是 | 登录用户名 |
| `password` | 是 | 登录密码 |

**Success `data`**

```json
{
  "user": {
    "id": 1,
    "username": "zhangsan",
    "real_name": "张三",
    "system_role": 2,
    "email": "zhangsan@example.com",
    "phone": "13800000000",
    "status": 1
  }
}
```

### 7.2 登出

**Method**

`POST`

**Path**

`/api/auth/logout`

**Request Body**

```json
{}
```

**Success `data`**

```json
null
```

### 7.3 获取当前登录用户信息

**Method**

`GET`

**Path**

`/api/auth/me`

**Success `data`**

```json
{
  "id": 1,
  "username": "zhangsan",
  "real_name": "张三",
  "system_role": 2,
  "email": "zhangsan@example.com",
  "phone": "13800000000",
  "status": 1,
  "created_at": "2026-03-27T09:00:00+08:00",
  "updated_at": "2026-03-27T09:00:00+08:00"
}
```

---

## 8. 用户接口

说明：

- 本章接口属于系统级用户管理接口，第一版默认仅管理员可调用
- 项目负责人添加项目成员时，不直接复用管理员管理接口，而使用项目上下文中的候选列表接口

### 8.1 获取用户列表

**Method**

`GET`

**Path**

`/api/users`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `keyword` | 否 | 按 `username` 或 `real_name` 模糊搜索 |
| `system_role` | 否 | 按系统角色筛选 |
| `status` | 否 | 按账号状态筛选 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `10` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 12,
      "username": "lisi",
      "real_name": "李四",
      "system_role": 2,
      "email": "lisi@example.com",
      "phone": "13800000000",
      "status": 1,
      "created_at": "2026-03-27T10:00:00+08:00",
      "updated_at": "2026-03-27T10:00:00+08:00"
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 10
}
```

### 8.2 获取用户详情

**Method**

`GET`

**Path**

`/api/users/{user_id}`

**Success `data`**

```json
{
  "id": 12,
  "username": "lisi",
  "real_name": "李四",
  "system_role": 2,
  "email": "lisi@example.com",
  "phone": "13800000000",
  "status": 1,
  "created_at": "2026-03-27T10:00:00+08:00",
  "updated_at": "2026-03-27T10:00:00+08:00"
}
```

### 8.3 创建用户

**Method**

`POST`

**Path**

`/api/users`

**Request Body**

```json
{
  "username": "lisi",
  "password": "123456",
  "real_name": "李四",
  "system_role": 2,
  "email": "lisi@example.com",
  "phone": "13800000000"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `username` | 是 | 登录用户名，唯一 |
| `password` | 是 | 明文密码，仅用于创建时输入 |
| `real_name` | 是 | 真实姓名 |
| `system_role` | 否 | 默认 `3` |
| `email` | 否 | 默认空字符串 |
| `phone` | 否 | 默认空字符串 |

**Success `data`**

```json
{
  "id": 12,
  "username": "lisi",
  "real_name": "李四",
  "system_role": 2,
  "email": "lisi@example.com",
  "phone": "13800000000",
  "status": 1,
  "created_at": "2026-03-27T10:00:00+08:00",
  "updated_at": "2026-03-27T10:00:00+08:00"
}
```

### 8.4 修改用户基础信息

**Method**

`PATCH`

**Path**

`/api/users/{user_id}`

**Request Body**

```json
{
  "real_name": "李四",
  "system_role": 2,
  "email": "lisi@example.com",
  "phone": "13800000001"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `real_name` | 否 | 真实姓名 |
| `system_role` | 否 | 系统角色 |
| `email` | 否 | 邮箱 |
| `phone` | 否 | 手机号 |

**Success `data`**

```json
{
  "id": 12,
  "username": "lisi",
  "real_name": "李四",
  "system_role": 2,
  "email": "lisi@example.com",
  "phone": "13800000001",
  "status": 1,
  "updated_at": "2026-03-27T10:30:00+08:00"
}
```

### 8.5 修改用户状态

**Method**

`PATCH`

**Path**

`/api/users/{user_id}/status`

**Request Body**

```json
{
  "status": 2
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `status` | 是 | `1=启用`，`2=禁用` |

**Success `data`**

```json
{
  "id": 12,
  "status": 2,
  "updated_at": "2026-03-27T11:00:00+08:00"
}
```

---

## 9. 项目模板接口

说明：

- 第一版仅提供系统预置模板查询能力
- 不提供用户自定义模板管理接口
- 模板只描述阶段节点名称、说明和顺序，不直接承载具体日期

### 9.1 获取项目模板列表

**Method**

`GET`

**Path**

`/api/project-templates`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `status` | 否 | 默认仅返回启用模板，可传 `1` 或 `2` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 1,
      "name": "标准研发项目模板",
      "description": "适用于常规内部研发项目",
      "status": 1,
      "node_count": 4
    }
  ]
}
```

### 9.2 获取项目模板详情

**Method**

`GET`

**Path**

`/api/project-templates/{template_id}`

**Success `data`**

```json
{
  "id": 1,
  "name": "标准研发项目模板",
  "description": "适用于常规内部研发项目",
  "status": 1,
  "nodes": [
    {
      "id": 11,
      "name": "需求分析",
      "description": "梳理需求边界",
      "sequence_no": 1
    },
    {
      "id": 12,
      "name": "开发实现",
      "description": "完成核心功能开发",
      "sequence_no": 2
    }
  ]
}
```

---

## 10. 项目接口

说明：

- 项目接口负责项目的基础信息、生命周期动作和负责人转交
- 项目状态不通过普通编辑接口直接修改
- 项目创建成功后，创建者自动成为项目负责人，并自动写入项目成员关系

### 10.1 获取项目列表

**Method**

`GET`

**Path**

`/api/projects`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `keyword` | 否 | 按项目名称模糊搜索 |
| `status` | 否 | 按项目状态筛选 |
| `owner_user_id` | 否 | 仅管理员可用，按负责人筛选 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `10` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 1001,
      "name": "内部进度平台",
      "description": "用于内部项目进度跟踪",
      "owner_user_id": 1,
      "owner_real_name": "张三",
      "status": 2,
      "planned_start_date": "2026-03-20",
      "planned_end_date": "2026-06-30",
      "completed_at": null,
      "member_count": 5,
      "node_count": 3,
      "sub_task_count": 12,
      "created_at": "2026-03-19T11:00:00+08:00",
      "updated_at": "2026-03-27T09:30:00+08:00",
      "is_owner": true
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 10
}
```

接口说明：

- 管理员返回全部项目
- 非管理员仅返回当前用户可见项目
- 第一版不单独返回 `project_type`

### 10.2 创建项目

**Method**

`POST`

**Path**

`/api/projects`

**Request Body**

```json
{
  "name": "内部进度平台",
  "description": "用于内部项目进度跟踪",
  "planned_start_date": "2026-03-20",
  "planned_end_date": "2026-06-30"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 是 | 项目名称 |
| `description` | 否 | 项目描述 |
| `planned_start_date` | 是 | 项目计划开始日期 |
| `planned_end_date` | 是 | 项目计划结束日期 |

**Success `data`**

```json
{
  "id": 1001,
  "name": "内部进度平台",
  "description": "用于内部项目进度跟踪",
  "owner_user_id": 1,
  "status": 1,
  "planned_start_date": "2026-03-20",
  "planned_end_date": "2026-06-30",
  "completed_at": null,
  "created_by": 1,
  "created_at": "2026-03-19T11:00:00+08:00",
  "updated_at": "2026-03-19T11:00:00+08:00"
}
```

业务约束：

- `planned_start_date <= planned_end_date`
- 创建者自动成为 `owner_user_id`
- 创建者必须自动写入 `project_member`

### 10.3 获取项目详情

**Method**

`GET`

**Path**

`/api/projects/{project_id}`

**Success `data`**

```json
{
  "id": 1001,
  "name": "内部进度平台",
  "description": "用于内部项目进度跟踪",
  "owner_user_id": 1,
  "owner_real_name": "张三",
  "status": 2,
  "planned_start_date": "2026-03-20",
  "planned_end_date": "2026-06-30",
  "completed_at": null,
  "created_by": 1,
  "created_by_real_name": "张三",
  "created_at": "2026-03-19T11:00:00+08:00",
  "updated_at": "2026-03-27T09:30:00+08:00",
  "member_count": 5,
  "node_count": 3,
  "completed_node_count": 1,
  "sub_task_count": 12,
  "completed_sub_task_count": 4,
  "permissions": {
    "can_edit_basic": true,
    "can_manage_members": true,
    "can_manage_nodes": true,
    "can_transfer_owner": true,
    "can_delete": true
  }
}
```

### 10.4 修改项目基础信息

**Method**

`PATCH`

**Path**

`/api/projects/{project_id}`

**Request Body**

```json
{
  "name": "内部进度平台 V2",
  "description": "用于内部项目进度跟踪",
  "planned_start_date": "2026-03-20",
  "planned_end_date": "2026-07-10"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 否 | 项目名称 |
| `description` | 否 | 项目描述 |
| `planned_start_date` | 否 | 项目计划开始日期 |
| `planned_end_date` | 否 | 项目计划结束日期 |

**Success `data`**

```json
{
  "id": 1001,
  "name": "内部进度平台 V2",
  "description": "用于内部项目进度跟踪",
  "status": 2,
  "planned_start_date": "2026-03-20",
  "planned_end_date": "2026-07-10",
  "updated_at": "2026-03-27T11:30:00+08:00"
}
```

业务约束：

- 不允许通过本接口直接修改 `owner_user_id`、`status`、`completed_at`
- 若项目已完成，必须先调用“撤销完成”
- 若项目下存在阶段节点，项目时间区间变更后仍必须覆盖全部阶段节点时间区间
- 若本次修改影响当前项目的超期判定，响应中应返回按最新日期重算后的 `status`

### 10.5 删除项目

**Method**

`DELETE`

**Path**

`/api/projects/{project_id}`

**Success `data`**

```json
{
  "id": 1001
}
```

业务约束：

- 删除项目时，由数据库外键级联删除项目成员、阶段节点、子任务和进度记录
- 管理员可以删除全部项目
- 非管理员仅可删除自己有权限删除的项目

### 10.6 手动开始项目

**Method**

`POST`

**Path**

`/api/projects/{project_id}/start`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 1001,
  "status": 2,
  "completed_at": null,
  "updated_at": "2026-03-27T12:00:00+08:00"
}
```

业务约束：

- 仅当项目下没有任何阶段节点时，才允许手动开始
- 若当前时间已超过 `planned_end_date`，则返回状态应为 `4=已延期`
- 已完成项目不能直接再次开始，必须先撤销完成

### 10.7 手动完成项目

**Method**

`POST`

**Path**

`/api/projects/{project_id}/complete`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 1001,
  "status": 3,
  "completed_at": "2026-03-27T12:10:00+08:00",
  "updated_at": "2026-03-27T12:10:00+08:00"
}
```

业务约束：

- 当项目下没有阶段节点时，项目负责人可直接手动完成
- 当项目下存在阶段节点时，只有全部阶段节点都已完成后才能完成项目
- 完成后项目进入冻结态

### 10.8 撤销项目完成

**Method**

`POST`

**Path**

`/api/projects/{project_id}/reopen`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 1001,
  "status": 2,
  "completed_at": null,
  "updated_at": "2026-03-27T12:20:00+08:00"
}
```

业务约束：

- 仅对当前状态为 `已完成` 的项目开放
- 撤销完成后，项目状态由 Service 层按当前时间和开始信号重新计算为 `未开始 / 进行中 / 已延期`
- 第一版当前 schema 没有项目级“开始信号”历史表；当前实现按“已存在开始信号”处理，因此撤销后实际返回 `进行中 / 已延期`
- 本接口只负责撤销完成，不负责后续编辑

### 10.9 获取负责人转交候选列表

**Method**

`GET`

**Path**

`/api/projects/{project_id}/owner/candidates`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `keyword` | 否 | 按用户名或姓名模糊搜索 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `10` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 12,
      "username": "lisi",
      "real_name": "李四",
      "system_role": 2,
      "status": 1,
      "is_project_member": false
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 10
}
```

接口说明：

- 当前项目负责人主动转交时，仅返回启用状态的项目经理用户
- 管理员调整团队项目负责人时，可返回启用状态的管理员和项目经理用户

### 10.10 转交项目负责人

**Method**

`POST`

**Path**

`/api/projects/{project_id}/owner/transfer`

**Request Body**

```json
{
  "target_user_id": 12
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `target_user_id` | 是 | 新项目负责人用户 ID |

**Success `data`**

```json
{
  "project_id": 1001,
  "previous_owner_user_id": 1,
  "owner_user_id": 12,
  "owner_real_name": "李四",
  "auto_added_as_member": true,
  "updated_at": "2026-03-27T12:30:00+08:00"
}
```

业务约束：

- 当前项目负责人主动转交时，目标用户必须是项目经理
- 管理员调整团队项目负责人时，目标用户必须是管理员或项目经理
- 若目标用户当前不在项目成员中，必须自动加入项目成员
- 原负责人转交后保留项目成员身份
- 普通员工创建的个人自用项目不允许转交负责人

---

## 11. 项目成员接口

说明：

- 项目成员管理只操作 `project_member` 关系
- 项目内不存在单独的成员角色字段
- 成员列表可供项目成员查看；增删成员属于受控写操作

### 11.1 获取项目成员列表

**Method**

`GET`

**Path**

`/api/projects/{project_id}/members`

**Success `data`**

```json
{
  "list": [
    {
      "project_id": 1001,
      "user_id": 1,
      "username": "zhangsan",
      "real_name": "张三",
      "system_role": 2,
      "status": 1,
      "joined_at": "2026-03-19T11:00:00+08:00",
      "is_owner": true
    },
    {
      "project_id": 1001,
      "user_id": 12,
      "username": "lisi",
      "real_name": "李四",
      "system_role": 3,
      "status": 1,
      "joined_at": "2026-03-20T09:00:00+08:00",
      "is_owner": false
    }
  ]
}
```

### 11.2 获取可添加成员候选列表

**Method**

`GET`

**Path**

`/api/projects/{project_id}/members/candidates`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `keyword` | 否 | 按用户名或姓名模糊搜索 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `10` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 18,
      "username": "wangwu",
      "real_name": "王五",
      "system_role": 3,
      "status": 1
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 10
}
```

业务约束：

- 默认排除已属于当前项目的成员
- 默认仅返回启用状态用户
- 返回字段应尽量精简，仅提供项目内选人所需信息
- 普通员工创建的个人自用项目禁止调用本接口

### 11.3 添加项目成员

**Method**

`POST`

**Path**

`/api/projects/{project_id}/members`

**Request Body**

```json
{
  "user_id": 18
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `user_id` | 是 | 待加入的用户 ID |

**Success `data`**

```json
{
  "project_id": 1001,
  "user_id": 18,
  "joined_at": "2026-03-27T13:00:00+08:00"
}
```

业务约束：

- 普通员工创建的个人自用项目不允许添加成员
- 非管理员场景下，只有管理员或项目经理身份的项目负责人可以添加成员
- 重复添加返回 `40903`

### 11.4 移除项目成员

**Method**

`DELETE`

**Path**

`/api/projects/{project_id}/members/{member_user_id}`

**Success `data`**

```json
{
  "project_id": 1001,
  "user_id": 18
}
```

业务约束：

- 若该成员仍存在未完成子任务，则拒绝移除
- 当前项目负责人不能通过本接口直接移除自己；如需变更负责人，应先转交负责人
- 普通员工创建的个人自用项目不允许移除成员

---

## 12. 阶段节点接口

说明：

- 阶段节点隶属于项目，项目内按 `sequence_no` 线性排序
- 节点的基础信息编辑与状态动作分离
- 节点时间区间必须落在所属项目时间区间内

### 12.1 获取项目阶段节点列表

**Method**

`GET`

**Path**

`/api/projects/{project_id}/nodes`

**Success `data`**

```json
{
  "list": [
    {
      "id": 2001,
      "project_id": 1001,
      "name": "需求分析",
      "description": "完成需求梳理",
      "sequence_no": 1,
      "status": 3,
      "planned_start_date": "2026-03-20",
      "planned_end_date": "2026-03-25",
      "completed_at": "2026-03-25T18:00:00+08:00",
      "created_by": 1,
      "created_at": "2026-03-19T11:20:00+08:00",
      "updated_at": "2026-03-25T18:00:00+08:00",
      "sub_task_count": 4,
      "completed_sub_task_count": 4
    }
  ]
}
```

### 12.2 创建阶段节点

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes`

**Request Body**

```json
{
  "name": "开发实现",
  "description": "完成核心功能开发",
  "planned_start_date": "2026-03-26",
  "planned_end_date": "2026-04-20"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 是 | 节点名称 |
| `description` | 否 | 节点说明 |
| `planned_start_date` | 是 | 节点计划开始日期 |
| `planned_end_date` | 是 | 节点计划结束日期 |

**Success `data`**

```json
{
  "id": 2002,
  "project_id": 1001,
  "name": "开发实现",
  "description": "完成核心功能开发",
  "sequence_no": 2,
  "status": 1,
  "planned_start_date": "2026-03-26",
  "planned_end_date": "2026-04-20",
  "completed_at": null,
  "created_by": 1,
  "created_at": "2026-03-27T13:30:00+08:00",
  "updated_at": "2026-03-27T13:30:00+08:00"
}
```

业务约束：

- 节点时间区间必须被项目时间区间包含
- 创建阶段节点时默认追加到末尾
- 项目已完成时不允许新增节点

### 12.3 获取阶段节点详情

**Method**

`GET`

**Path**

`/api/projects/{project_id}/nodes/{node_id}`

**Success `data`**

```json
{
  "id": 2002,
  "project_id": 1001,
  "name": "开发实现",
  "description": "完成核心功能开发",
  "sequence_no": 2,
  "status": 2,
  "planned_start_date": "2026-03-26",
  "planned_end_date": "2026-04-20",
  "completed_at": null,
  "created_by": 1,
  "created_at": "2026-03-27T13:30:00+08:00",
  "updated_at": "2026-03-28T09:00:00+08:00",
  "sub_task_count": 5,
  "completed_sub_task_count": 1
}
```

### 12.4 修改阶段节点基础信息

**Method**

`PATCH`

**Path**

`/api/projects/{project_id}/nodes/{node_id}`

**Request Body**

```json
{
  "name": "开发实现阶段",
  "description": "完成核心功能开发与联调",
  "planned_start_date": "2026-03-26",
  "planned_end_date": "2026-04-25"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 否 | 节点名称 |
| `description` | 否 | 节点说明 |
| `planned_start_date` | 否 | 节点计划开始日期 |
| `planned_end_date` | 否 | 节点计划结束日期 |

**Success `data`**

```json
{
  "id": 2002,
  "name": "开发实现阶段",
  "description": "完成核心功能开发与联调",
  "planned_start_date": "2026-03-26",
  "planned_end_date": "2026-04-25",
  "updated_at": "2026-03-27T14:00:00+08:00"
}
```

业务约束：

- 已完成节点必须先撤销完成
- 若节点下存在子任务，节点时间区间变更后仍必须覆盖全部子任务时间区间

### 12.5 调整阶段节点顺序

**Method**

`PUT`

**Path**

`/api/projects/{project_id}/nodes/reorder`

**Request Body**

```json
{
  "nodes": [
    {
      "node_id": 2001,
      "sequence_no": 1
    },
    {
      "node_id": 2002,
      "sequence_no": 2
    },
    {
      "node_id": 2003,
      "sequence_no": 3
    }
  ]
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `nodes` | 是 | 调整后的完整节点顺序集合 |
| `nodes[].node_id` | 是 | 节点 ID |
| `nodes[].sequence_no` | 是 | 新顺序号，从 `1` 连续递增 |

**Success `data`**

```json
{
  "project_id": 1001,
  "nodes": [
    {
      "node_id": 2001,
      "sequence_no": 1
    },
    {
      "node_id": 2002,
      "sequence_no": 2
    },
    {
      "node_id": 2003,
      "sequence_no": 3
    }
  ],
  "updated_at": "2026-03-27T14:10:00+08:00"
}
```

业务约束：

- 排序调整必须在一个事务内完成
- 已完成节点不允许参与重排
- 项目已完成时不允许调整节点顺序

### 12.6 删除阶段节点

**Method**

`DELETE`

**Path**

`/api/projects/{project_id}/nodes/{node_id}`

**Success `data`**

```json
{
  "id": 2002
}
```

业务约束：

- 删除节点时，由数据库外键级联删除其下子任务和进度记录
- 已完成节点不能直接删除，必须先撤销完成

### 12.7 手动开始阶段节点

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes/{node_id}/start`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 2002,
  "status": 2,
  "completed_at": null,
  "updated_at": "2026-03-27T14:20:00+08:00"
}
```

业务约束：

- 仅当节点下没有任何子任务时，才允许手动开始
- 若当前时间已超过 `planned_end_date`，则返回状态应为 `4=已延期`

### 12.8 手动完成阶段节点

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes/{node_id}/complete`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 2002,
  "status": 3,
  "completed_at": "2026-03-27T14:30:00+08:00",
  "updated_at": "2026-03-27T14:30:00+08:00"
}
```

业务约束：

- 当节点下没有子任务时，项目负责人可直接完成节点
- 当节点下存在子任务时，只有全部子任务都已完成后才能完成节点

### 12.9 撤销阶段节点完成

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes/{node_id}/reopen`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 2002,
  "status": 2,
  "completed_at": null,
  "updated_at": "2026-03-27T14:40:00+08:00"
}
```

业务约束：

- 仅对当前状态为 `已完成` 的节点开放
- 撤销完成后，节点状态由 Service 层按当前时间和开始信号重新计算
- 对某一节点撤销完成时，不自动联动回退后续节点状态

### 12.10 基于模板批量生成阶段节点

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes/apply-template`

**Request Body**

```json
{
  "template_id": 1,
  "nodes": [
    {
      "template_node_id": 11,
      "planned_start_date": "2026-03-20",
      "planned_end_date": "2026-03-25"
    },
    {
      "template_node_id": 12,
      "planned_start_date": "2026-03-26",
      "planned_end_date": "2026-04-20"
    }
  ]
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `template_id` | 是 | 模板 ID |
| `nodes` | 是 | 模板节点日期映射 |
| `nodes[].template_node_id` | 是 | 模板节点 ID |
| `nodes[].planned_start_date` | 是 | 目标阶段开始日期 |
| `nodes[].planned_end_date` | 是 | 目标阶段结束日期 |

**Success `data`**

```json
{
  "project_id": 1001,
  "template_id": 1,
  "generated_nodes": [
    {
      "id": 2001,
      "name": "需求分析",
      "sequence_no": 1,
      "planned_start_date": "2026-03-20",
      "planned_end_date": "2026-03-25"
    },
    {
      "id": 2002,
      "name": "开发实现",
      "sequence_no": 2,
      "planned_start_date": "2026-03-26",
      "planned_end_date": "2026-04-20"
    }
  ]
}
```

业务约束：

- 仅允许对当前没有任何阶段节点的项目调用，避免重复整体套模板
- `nodes` 必须完整覆盖模板中的全部节点
- 生成后的每个节点时间区间必须落在项目时间区间内
- 服务端以模板中的 `name`、`description`、`sequence_no` 为准创建节点

---

## 13. 子任务接口

说明：

- 子任务必须归属于某一个阶段节点
- 子任务负责人只能有一个，字段为 `responsible_user_id`
- 子任务的基础信息编辑与进度更新、开始动作、撤销完成分离

### 13.1 获取节点下子任务列表

**Method**

`GET`

**Path**

`/api/projects/{project_id}/nodes/{node_id}/subtasks`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `status` | 否 | 按状态筛选 |
| `responsible_user_id` | 否 | 按负责人筛选 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `20` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 3001,
      "node_id": 2002,
      "name": "完成登录接口开发",
      "description": "完成账号密码登录功能",
      "responsible_user_id": 18,
      "responsible_real_name": "王五",
      "status": 2,
      "progress_percent": 60,
      "priority": 2,
      "planned_start_date": "2026-03-27",
      "planned_end_date": "2026-03-31",
      "completed_at": null,
      "created_by": 1,
      "created_at": "2026-03-27T15:00:00+08:00",
      "updated_at": "2026-03-28T09:30:00+08:00"
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 20
}
```

### 13.2 获取当前用户负责的子任务列表

**Method**

`GET`

**Path**

`/api/my/subtasks`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `status` | 否 | 按状态筛选 |
| `project_id` | 否 | 按项目筛选 |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `10` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 3001,
      "project_id": 1001,
      "project_name": "内部进度平台",
      "node_id": 2002,
      "node_name": "开发实现",
      "name": "完成登录接口开发",
      "status": 2,
      "progress_percent": 60,
      "priority": 2,
      "planned_start_date": "2026-03-27",
      "planned_end_date": "2026-03-31",
      "completed_at": null,
      "updated_at": "2026-03-28T09:30:00+08:00"
    }
  ],
  "total": 1,
  "page": 1,
  "page_size": 10
}
```

### 13.3 创建子任务

**Method**

`POST`

**Path**

`/api/projects/{project_id}/nodes/{node_id}/subtasks`

**Request Body**

```json
{
  "name": "完成登录接口开发",
  "description": "完成账号密码登录功能",
  "responsible_user_id": 18,
  "priority": 2,
  "planned_start_date": "2026-03-27",
  "planned_end_date": "2026-03-31"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 是 | 子任务名称 |
| `description` | 否 | 子任务描述 |
| `responsible_user_id` | 否 | 不传时默认项目负责人 |
| `priority` | 否 | 默认 `2` |
| `planned_start_date` | 是 | 子任务计划开始日期 |
| `planned_end_date` | 是 | 子任务计划结束日期 |

**Success `data`**

```json
{
  "id": 3001,
  "node_id": 2002,
  "name": "完成登录接口开发",
  "description": "完成账号密码登录功能",
  "responsible_user_id": 18,
  "status": 1,
  "progress_percent": 0,
  "priority": 2,
  "planned_start_date": "2026-03-27",
  "planned_end_date": "2026-03-31",
  "completed_at": null,
  "created_by": 1,
  "created_at": "2026-03-27T15:00:00+08:00",
  "updated_at": "2026-03-27T15:00:00+08:00"
}
```

业务约束：

- 子任务时间区间必须被所属节点时间区间包含
- `responsible_user_id` 必须属于当前项目成员
- 初始状态固定为 `未开始`
- 初始进度固定为 `0`

### 13.4 获取子任务详情

**Method**

`GET`

**Path**

`/api/subtasks/{subtask_id}`

**Success `data`**

```json
{
  "id": 3001,
  "project_id": 1001,
  "project_name": "内部进度平台",
  "node_id": 2002,
  "node_name": "开发实现",
  "name": "完成登录接口开发",
  "description": "完成账号密码登录功能",
  "responsible_user_id": 18,
  "responsible_real_name": "王五",
  "status": 2,
  "progress_percent": 60,
  "priority": 2,
  "planned_start_date": "2026-03-27",
  "planned_end_date": "2026-03-31",
  "completed_at": null,
  "created_by": 1,
  "created_at": "2026-03-27T15:00:00+08:00",
  "updated_at": "2026-03-28T09:30:00+08:00"
}
```

### 13.5 修改子任务基础信息

**Method**

`PATCH`

**Path**

`/api/subtasks/{subtask_id}`

**Request Body**

```json
{
  "name": "完成登录与登出接口开发",
  "description": "完成账号密码登录与退出登录功能",
  "responsible_user_id": 18,
  "priority": 3,
  "planned_start_date": "2026-03-27",
  "planned_end_date": "2026-04-01"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `name` | 否 | 子任务名称 |
| `description` | 否 | 子任务描述 |
| `responsible_user_id` | 否 | 新责任人，必须属于当前项目成员 |
| `priority` | 否 | 优先级 |
| `planned_start_date` | 否 | 子任务计划开始日期 |
| `planned_end_date` | 否 | 子任务计划结束日期 |

**Success `data`**

```json
{
  "id": 3001,
  "name": "完成登录与登出接口开发",
  "responsible_user_id": 18,
  "priority": 3,
  "planned_start_date": "2026-03-27",
  "planned_end_date": "2026-04-01",
  "updated_at": "2026-03-28T10:00:00+08:00"
}
```

业务约束：

- 不允许通过本接口直接修改 `status`、`progress_percent`、`completed_at`
- 已完成子任务必须先撤销完成
- 时间变更后仍需落在所属节点时间区间内

### 13.6 删除子任务

**Method**

`DELETE`

**Path**

`/api/subtasks/{subtask_id}`

**Success `data`**

```json
{
  "id": 3001
}
```

业务约束：

- 删除子任务时，由数据库外键级联删除其进度记录
- 已完成子任务不能直接删除，必须先撤销完成

### 13.7 手动开始子任务

**Method**

`POST`

**Path**

`/api/subtasks/{subtask_id}/start`

**Request Body**

```json
{
  "progress_note": "开始处理该任务"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `progress_note` | 否 | 开始说明，默认空字符串 |

**Success `data`**

```json
{
  "subtask": {
    "id": 3001,
    "status": 2,
    "progress_percent": 0,
    "completed_at": null,
    "updated_at": "2026-03-28T10:10:00+08:00"
  },
  "progress_record": {
    "id": 5001,
    "sub_task_id": 3001,
    "operator_user_id": 18,
    "progress_note": "开始处理该任务",
    "progress_percent": 0,
    "status": 2,
    "created_at": "2026-03-28T10:10:00+08:00"
  }
}
```

业务约束：

- 允许子任务负责人手动开始任务
- 允许项目负责人在权限范围内手动将任务置为开始
- 若当前时间已超过 `planned_end_date`，则状态应计算为 `4=已延期`
- 本接口成功后，应同步写入一条 `sub_task_progress` 历史记录，用于保留开始信号

### 13.8 撤销子任务完成

**Method**

`POST`

**Path**

`/api/subtasks/{subtask_id}/reopen`

**Request Body**

```json
{}
```

**Success `data`**

```json
{
  "id": 3001,
  "status": 2,
  "progress_percent": 60,
  "completed_at": null,
  "updated_at": "2026-03-28T10:20:00+08:00"
}
```

业务约束：

- 仅对当前状态为 `已完成` 的子任务开放
- 撤销完成后，`progress_percent` 恢复为最近一条“未完成状态”的历史值；若不存在，则恢复为 `0`
- 随后由 Service 层按当前时间和开始信号重新计算状态为 `未开始 / 进行中 / 已延期`
- 本接口只负责撤销完成，不负责后续编辑

---

## 14. 子任务进度接口

说明：

- 子任务当前值与历史记录必须同时维护
- 进度百分比第一版按 `10%` 粒度录入
- 默认仅子任务负责人可提交进度

### 14.1 提交子任务进度

**Method**

`POST`

**Path**

`/api/subtasks/{subtask_id}/progress`

**Request Body**

```json
{
  "status": 2,
  "progress_percent": 60,
  "progress_note": "已完成接口定义和数据库连接"
}
```

**字段说明**

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `status` | 是 | `1/2/3/4` |
| `progress_percent` | 是 | `0~100` 且必须为 `10` 的倍数 |
| `progress_note` | 否 | 进度说明或备注 |

**Success `data`**

```json
{
  "subtask": {
    "id": 3001,
    "status": 2,
    "progress_percent": 60,
    "completed_at": null,
    "updated_at": "2026-03-28T10:30:00+08:00"
  },
  "progress_record": {
    "id": 5002,
    "sub_task_id": 3001,
    "operator_user_id": 18,
    "progress_note": "已完成接口定义和数据库连接",
    "progress_percent": 60,
    "status": 2,
    "created_at": "2026-03-28T10:30:00+08:00"
  }
}
```

业务约束：

- 默认仅子任务负责人可提交进度
- `status` 与 `progress_percent` 必须匹配
- 当 `status=3` 时，`progress_percent` 必须为 `100`
- 当 `status!=3` 时，`progress_percent` 必须小于 `100`
- 若首次提交的 `progress_percent > 0`，应视为触发开始信号
- 一旦开始信号已经成立，不应通过本接口把子任务状态回退为 `未开始`
- 提交成功后必须同时更新 `sub_task` 当前值，并新增一条 `sub_task_progress` 记录
- 对已完成子任务不允许直接提交进度，必须先撤销完成

### 14.2 获取子任务进度记录列表

**Method**

`GET`

**Path**

`/api/subtasks/{subtask_id}/progress`

**Query**

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `page` | 否 | 默认 `1` |
| `page_size` | 否 | 默认 `20` |

**Success `data`**

```json
{
  "list": [
    {
      "id": 5002,
      "sub_task_id": 3001,
      "operator_user_id": 18,
      "operator_real_name": "王五",
      "progress_note": "已完成接口定义和数据库连接",
      "progress_percent": 60,
      "status": 2,
      "created_at": "2026-03-28T10:30:00+08:00"
    },
    {
      "id": 5001,
      "sub_task_id": 3001,
      "operator_user_id": 18,
      "operator_real_name": "王五",
      "progress_note": "开始处理该任务",
      "progress_percent": 0,
      "status": 2,
      "created_at": "2026-03-28T10:10:00+08:00"
    }
  ],
  "total": 2,
  "page": 1,
  "page_size": 20
}
```

---

## 15. 甘特图接口

说明：

- 第一版甘特图主要由前端完成可视化，后端负责提供时间数据
- 项目甘特图需要覆盖项目、阶段节点、子任务和成员维度数据

### 15.1 获取项目甘特图数据

**Method**

`GET`

**Path**

`/api/projects/{project_id}/gantt`

**Success `data`**

```json
{
  "project": {
    "id": 1001,
    "name": "内部进度平台",
    "owner_user_id": 1,
    "owner_real_name": "张三",
    "status": 2,
    "planned_start_date": "2026-03-20",
    "planned_end_date": "2026-06-30",
    "completed_at": null
  },
  "nodes": [
    {
      "id": 2001,
      "name": "需求分析",
      "sequence_no": 1,
      "status": 3,
      "planned_start_date": "2026-03-20",
      "planned_end_date": "2026-03-25",
      "completed_at": "2026-03-25T18:00:00+08:00"
    },
    {
      "id": 2002,
      "name": "开发实现",
      "sequence_no": 2,
      "status": 2,
      "planned_start_date": "2026-03-26",
      "planned_end_date": "2026-04-20",
      "completed_at": null
    }
  ],
  "subtasks": [
    {
      "id": 3001,
      "node_id": 2002,
      "node_name": "开发实现",
      "name": "完成登录接口开发",
      "responsible_user_id": 18,
      "responsible_real_name": "王五",
      "status": 2,
      "progress_percent": 60,
      "priority": 2,
      "planned_start_date": "2026-03-27",
      "planned_end_date": "2026-03-31",
      "completed_at": null
    }
  ],
  "member_rows": [
    {
      "user_id": 18,
      "real_name": "王五",
      "subtasks": [
        {
          "subtask_id": 3001,
          "name": "完成登录接口开发",
          "node_id": 2002,
          "node_name": "开发实现",
          "status": 2,
          "progress_percent": 60,
          "planned_start_date": "2026-03-27",
          "planned_end_date": "2026-03-31",
          "completed_at": null
        }
      ]
    }
  ]
}
```

接口说明：

- 同一个接口同时返回项目时间轴和成员任务视角数据，减少前端多次请求
- 若后续页面性能需要优化，可再拆出成员甘特图专用接口

---

## 16. 关键服务层规则落点

以下规则不体现在单表结构中，但必须由 Service 层严格实现：

### 16.1 时间区间校验

- 阶段节点时间区间必须被项目时间区间包含
- 子任务时间区间必须被阶段节点时间区间包含
- 修改父级时间区间时，仍必须覆盖全部子级时间区间

### 16.2 自动联动规则

- 任一子任务进入 `进行中 / 已延期 / 已完成`，可将当前 `未开始` 的父节点推进为 `进行中`
- 任一阶段节点进入 `进行中 / 已延期 / 已完成`，可将当前 `未开始` 的项目推进为 `进行中`
- 下级自动联动只允许把父级从 `未开始` 推进为 `进行中`
- 下级自动联动不会自动把父级改写为 `已完成` 或 `已延期`

### 16.3 已延期判定

- 对未完成对象，若按北京时间（`Asia/Shanghai`）判断当前日期已进入 `planned_end_date` 的次日 `00:00` 及以后，即当前北京时间日期 `>` `planned_end_date`，则状态自动判定为 `已延期`
- 到达 `planned_start_date` 当天，不自动触发状态变为 `进行中`

### 16.4 子任务开始信号

子任务的开始信号包括：

- 子任务负责人手动点击开始
- 首次提交大于 `0` 的进度
- 项目负责人手动将其设为开始

一旦开始信号成立，Service 层应保证该信号在后续状态重算中可被识别。

### 16.5 撤销完成

- “撤销完成”和“编辑”必须是两个分离动作
- 项目、阶段节点、子任务撤销完成后，都应先恢复到可编辑的未完成状态，再允许执行编辑
- 子任务撤销完成时，需先恢复最近一条未完成进度值，以满足数据库约束

## 17. 下一步建议

接口设计定稿后，建议按以下顺序进入实现阶段：

1. 先确定 Drogon 后端目录结构与模块边界
2. 再定义 DTO、统一返回体、错误码和权限中间件
3. 最后按模块逐步实现 Controller、Service、Repository
