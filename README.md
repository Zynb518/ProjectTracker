# Project Tracker

项目进度跟踪系统，后端基于 `C++20 + Drogon + PostgreSQL`，前端基于 `Vue 3 + TypeScript + Vite`。

当前仓库包含：

- 后端服务
- 前端页面
- 数据库初始化脚本
- 本地离线 AI 工具

## 技术栈

- Backend: `C++20`, `Drogon`, `PostgreSQL`
- Frontend: `Vue 3`, `TypeScript`, `Vite`
- Build: `CMake`

## 目录说明

- `src/`: 后端源码
- `frontend/`: 前端工程
- `db/migrations/`: 数据库建表脚本
- `db/seeds/`: 数据初始化脚本
- `config/`: 配置模板
- `tools/ai_project_manager/`: 本地离线 AI 工具

## Ubuntu 24 虚拟机最短部署路径

以下路径适用于“后端部署到虚拟机，前端可先本机打包，AI 模块暂不启用”的场景。

### 1. 安装后端依赖

先安装编译和运行依赖：

```bash
sudo apt update
sudo apt install -y \
  git build-essential cmake pkg-config \
  postgresql postgresql-client libpq-dev postgresql-all \
  libargon2-dev \
  nginx
```

如果你是源码安装 `Drogon`，务必先装好 PostgreSQL 开发环境，再编译安装 `Drogon`。

### 2. 初始化 PostgreSQL

建议创建独立数据库用户和数据库，并让数据库 owner 与应用用户保持一致。

```bash
sudo -u postgres psql
```

在 `psql` 中执行：

```sql
CREATE USER project_tracker WITH PASSWORD 'your_strong_password';
CREATE DATABASE project_tracker OWNER project_tracker;
GRANT ALL PRIVILEGES ON DATABASE project_tracker TO project_tracker;
ALTER DATABASE project_tracker OWNER TO project_tracker;
\c project_tracker
ALTER SCHEMA public OWNER TO project_tracker;
GRANT ALL ON SCHEMA public TO project_tracker;
```

导入表结构与生产用户种子：

```bash
psql -h 127.0.0.1 -U project_tracker -d project_tracker -f db/migrations/001_init_schema.sql
psql -h 127.0.0.1 -U project_tracker -d project_tracker -f db/seeds/002_prod_users.sql
```

### 3. 准备后端配置

从模板复制一份生产配置：

```bash
cp config/config.example.json config/config.prod.json
```

建议至少修改以下字段：

- `db_clients[0].host = 127.0.0.1`
- `db_clients[0].dbname = project_tracker`
- `db_clients[0].user = project_tracker`
- `db_clients[0].passwd = 你的数据库密码`
- 小内存虚拟机可把 `connection_number` 调低到 `2`
- 如果暂时不用 AI，可把 `custom_config` 留空对象

### 4. 编译并启动后端

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j2
./cmake-build-release/src/Project_Tracker ./config/config.prod.json
```

另开一个终端验证服务是否起来：

```bash
curl -i http://127.0.0.1:8080/api/auth/me
```

如果返回 `401` JSON，一般说明服务已经启动，只是当前未登录。

### 5. 前端运行方式

生产环境推荐：

- 在本机执行 `npm ci && npm run build`
- 把 `frontend/dist` 上传到虚拟机
- 由 `Nginx` 托管静态文件

临时联调也可以直接在虚拟机运行前端开发服务器：

```bash
cd frontend
npm ci
npm run dev
```

默认访问地址：

- `http://虚拟机IP:5173`

### 6. Nginx 托管前端并反代后端

生产态推荐由 `Nginx` 统一入口处理前端和 `/api`：

```nginx
server {
    listen 80;
    server_name _;

    root /var/www/project-tracker/dist;
    index index.html;

    location /api/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    location / {
        try_files $uri $uri/ /index.html;
    }
}
```

## 部署踩坑记录

### 1. `psql` 不加 `-h` 可能命中 `peer authentication`

如果直接执行：

```bash
psql -U project_tracker -d project_tracker
```

在 Ubuntu 默认配置下，常常会走本地 socket，并命中 `peer` 认证，出现类似：

```text
peer authentication failed for user "project_tracker"
```

解决方式是显式走 TCP：

```bash
psql -h 127.0.0.1 -U project_tracker -d project_tracker
```

### 2. `Drogon` 可能被编成“无数据库支持”版本

如果启动后端时报错：

```text
No database is supported by drogon, please install the database development library first.
```

说明当前机器上的 `Drogon` 在安装时没有正确带上 PostgreSQL 支持。常见原因是：

- 先装了 `Drogon`
- 后装了 PostgreSQL 开发库
- 然后直接编项目，没有重装 `Drogon`

正确顺序是：

- 先安装 `libpq-dev` / `postgresql-all`
- 再重新安装或重新编译 `Drogon`
- 最后删除项目构建目录并重新完整编译本项目

### 3. 小内存虚拟机不要把数据库连接数开太高

如果虚拟机内存只有 `4G` 左右，建议：

- `connection_number` 先从 `2` 起
- 暂时不要启用本地 AI 模型
- 生产态不要长期跑 `vite dev server`

### 4. 生产配置不要直接提交到 GitHub

以下文件或内容不适合直接提交到远程仓库：

- `config/config.dev.json`
- 生产数据库密码
- 本地临时产物
- 本机调试路径

建议：

- 只提交 `config/config.example.json`
- 在虚拟机本地创建 `config/config.prod.json`

### 5. 前端临时验证可以直接跑，正式部署再切静态托管

在虚拟机中直接运行：

```bash
cd frontend
npm ci
npm run dev
```

适合临时联调，但不适合作为正式部署方案。正式部署应切换为：

- 本机构建 `dist`
- 虚拟机由 `Nginx` 托管静态文件

## 相关文档

- 前端说明见 [frontend/README.md](/home/ubzy/CLionProjects/Project-Tracker/frontend/README.md)
- AI 工具说明见 [tools/ai_project_manager/README.md](/home/ubzy/CLionProjects/Project-Tracker/tools/ai_project_manager/README.md)
