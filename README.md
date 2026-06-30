# Project Tracker (项目进度管理系统)

这是一个基于 C++ Drogon 后端框架和 Vue 3 + TypeScript 前端框架构建的高性能项目进度与任务协作管理系统。项目支持多用户角色、项目生命周期管理、阶段里程碑节点管理、子任务派发协作以及项目甘特图可视化。

---

## 🏗️ 架构与技术栈

### 后端 (Backend)
* **核心框架**：[Drogon C++ Web Framework](https://github.com/drogonframework/drogon)（基于事件驱动与非阻塞 I/O 的超高性能 C++ Web 框架）
* **数据库**：PostgreSQL 16
* **安全验证**：基于 JWT 令牌的无状态身份验证与 Argon2 密码哈希
* **依赖库**：`libargon2`、`gettext-base`、`jsoncpp`、`UUID` 等

### 前端 (Frontend)
* **核心框架**：Vue 3 (Composition API)
* **开发构建**：Vite + TypeScript
* **状态管理**：Pinia
* **路由管理**：Vue Router
* **样式系统**：原生 CSS (Vanilla CSS) 与自适应暗色/亮色主题
* **画板与图表**：基于自研的高性能原生甘特图时间轴

---

## 📁 目录结构

```text
.
├── config/                 # 后端配置文件模板与开发配置
├── db/
│   ├── migrations/         # 数据库 Schema 迁移定义
│   └── seeds/              # 数据库种子数据演示
├── deploy/                 # 部署配置文件（Nginx 等）
├── docker/                 # 后端与前端的 Dockerfile
├── frontend/               # 单页前端项目根目录
├── src/                    # C++ 后端源码目录
│   └── modules/            # 后端核心业务模块目录
│       ├── auth/           # 用户认证授权模块
│       ├── device_log/     # 设备与审计日志模块
│       ├── health/         # 服务健康检查与数据库状态监测
│       ├── project/        # 项目全生命周期管理
│       ├── project_member/ # 项目成员分配与管理
│       ├── project_node/   # 项目里程碑节点管理
│       ├── task/           # 任务与子任务协作模块
│       └── user/           # 用户及系统角色管理
├── tests/                  # 测试用例目录 (GTest & HTTP 接口测试)
├── docker-compose.yml      # 本地一键启动容器编排文件
└── README.md
```

---

## 🚀 快速启动

系统所有组件均已容器化，推荐使用 Docker Compose 在本地一键拉起运行。

### 1. 准备工作
确保本地已安装 [Docker](https://www.docker.com/) 和 [Docker Compose v2+](https://docs.docker.com/compose/)。

复制环境变量模板文件并配置（默认已配置好开发环境变量）：
```bash
cp .env.example .env
```

### 2. 启动服务
在项目根目录下执行以下命令编译镜像并运行：
```bash
docker compose up --build -d
```
启动完成后，容器服务将映射到宿主机端口：
* **前端与网关地址**：[http://localhost/](http://localhost/) (运行在宿主机 80 端口)
* **后端 API**：接口已通过前端 Nginx 进行反向代理转发，地址为 `http://localhost/api/` (后端容器实际运行在 8080 端口)
* **数据库**：PostgreSQL 容器运行在 5432 端口

---

## 🧪 运行测试

系统包含后端单元/集成测试与前端组件测试。

### 1. 运行后端测试 (GoogleTest)
使用独立的测试容器进行 CMake 编译与测试套件执行：
```bash
docker compose run --rm tests
```

### 2. 运行前端测试 (Vitest)
进入 `frontend` 目录，安装依赖后执行测试：
```bash
cd frontend
npm install
npm run test
```

---

## 🔒 演示账号

系统启动时默认会导入 `db/seeds/001_demo_data.sql` 中的种子用户，您可以使用以下账号登录系统：

| 角色 | 用户名 | 密码        | 说明 |
| :--- | :--- |:----------| :--- |
| **系统管理员 (Admin)** | `admin` | `123456`  | 具备全部管理权限、用户管理权限 |
| **普通成员 (Member)** | `member` | `123456!` | 具备项目创建、里程碑及任务协作权限 |

---

## 📝 核心业务流程说明

1. **项目生命周期**：项目支持 `未开始` ➡️ `进行中` ➡️ `已完成` 或是由于超期标记为 `已延期`，并且支持重新打开。
2. **新手引导路径**：系统在首次登录进入时，提供向导式新手教程，帮助用户直接走完第一个项目的手动创建。
3. **甘特图视图**：提供了项目级的甘特图展示，用户可以在时间轴上直观查看各个里程碑阶段及子任务的排期，并支持将图表导出为图片。
