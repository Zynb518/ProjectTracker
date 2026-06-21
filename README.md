# 运维工单管理系统 (Operations Ticket System)

基于 `C++20 + Drogon` 后端与 `Vue 3 + TS + Vite` 前端构建的规范化生产运维工单管理系统。

---

## 🌟 核心业务特性

本系统通过对底层数据关系的巧妙概念映射，实现了**零修改后端代码、零改动数据库表结构**的轻量级升级，全面对齐企业级运维工单系统的指标：

1. **规范生产与自动节点注入**：
   * 在工单大厅新建工单时，系统会自动在工单下注入三个标准顺序流转阶段：`部门审批` ➡️ `派单执行` ➡️ `现场回单`。
2. **逐层审批与流转防插队**：
   * 采用前向依赖拦截逻辑。若前置流转节点（例如：`部门审批`）未完成，则后续节点（如 `派单执行`）及名下的具体派单任务一律锁定。
   * 支持多级子任务（如：`【审批】市局初审`、`【审批】省厅终审`）以呈现多层级联合审批。
3. **操作留痕与责任追踪**：
   * 每次技术人员提交更新，系统均会记录：**操作人真实姓名、更新进度百分比、操作时间、现场回单备注**，全面入库备查。
4. **省市多部门快捷发起工单**：
   * 支持在创建时选择“故障、需求、巡检、配置”等工单分类，系统自动拼接规范的标题前缀（例如：`【故障】省级政专网骨干光缆中断`）。
5. **系统角色 rebranding 全新重塑**：
   * `管理员` ➡️ **系统管理员** (系统全局管理权限)
   * `项目经理` ➡️ **工单审批官** (负责审批与派发协作任务)
   * `普通员工` ➡️ **运维工程师** (现场执行并填写回单)
6. **可视化统计报表与超时预警**：
   * 专属**统计报表**页面。
   * **近 7 日工单流转趋势**：纯 Vue 3 + 响应式 SVG 渲染（无第三方库依赖），展示每日新建工单与结单完成数的对比。
   * **工单类型分布**：动态展现各类工单占比。
   * **超时预警中心**：自动检索超过计划截止日期未完结的工单，展示延期天数并提供一键督办链接。
7. **多端自适应支持（电脑 WEB 与手机 APP）**：
   * 针对大屏幕提供宽体网格与拖拽展示。
   * 针对手机端提供流式卡片折叠、触控弹窗拦截等操作优化，并即将支持底部 APP 导航适配。

---

## 📁 目录结构

* `src/`：C++ Drogon 后端源码
* `frontend/`：Vue 3 前端工程
* `db/migrations/`：PostgreSQL 数据库表结构脚本
* `db/seeds/`：演示数据与生产种子脚本
* `config/`：应用配置文件目录
* `docker/`：前后端编译运行的 Dockerfile
* `deploy/`：部署用配置文件（如 Nginx、systemd 等）

---

## 🚀 极速部署方案 (Docker Compose 推荐)

推荐使用 Docker Compose 进行一键拉起，无需本地安装 C++ 编译环境或 Nginx。

### 1. 运行部署命令

在项目根目录下执行：
```bash
docker compose down -v && docker compose up -d --build
```
> [!IMPORTANT]
> `-v` 参数会清除旧的 PostgreSQL 数据卷，触发 `/docker-entrypoint-initdb.d/` 种子脚本自动重新注入我们精心准备的**运维工单演示数据**。

### 2. 默认登录账号

数据库中默认种子数据的所有用户密码统一为 `123456`：
* **系统管理员**：`admin` (系统管理员角色)
* **工单审批官**：`li_ming`, `wang_jie` (工单审批官角色)
* **运维工程师**：`chen_xu`, `zhao_yue`, `sun_qiang`, `zhou_tao` (运维工程师角色)

---

## 🔧 本地手动部署方案 (Ubuntu 24.04 示例)

如果您不想使用 Docker，可以按照以下步骤手动在虚拟机中编译和部署。

### 1. 安装系统依赖
```bash
sudo apt update
sudo apt install -y \
  git build-essential cmake pkg-config \
  postgresql postgresql-client libpq-dev postgresql-all \
  libargon2-dev \
  nginx
```
*注意：如果您是手动编译安装 `Drogon`，请务必先安装 `libpq-dev` 后再开始安装 Drogon。*

### 2. 初始化 PostgreSQL 数据库
```bash
sudo -u postgres psql
```
在 psql 控制台中执行：
```sql
CREATE USER project_tracker WITH PASSWORD 'your_strong_password';
CREATE DATABASE project_tracker OWNER project_tracker;
GRANT ALL PRIVILEGES ON DATABASE project_tracker TO project_tracker;
ALTER DATABASE project_tracker OWNER TO project_tracker;
\c project_tracker
ALTER SCHEMA public OWNER TO project_tracker;
GRANT ALL ON SCHEMA public TO project_tracker;
```
导入表结构和运维种子数据：
```bash
psql -h 127.0.0.1 -U project_tracker -d project_tracker -f db/migrations/001_init_schema.sql
psql -h 127.0.0.1 -U project_tracker -d project_tracker -f db/seeds/001_demo_data.sql
```

### 3. 配置与启动后端
复制并修改后端配置文件：
```bash
cp config/config.example.json config/config.prod.json
```
修改 `config.prod.json` 中的 `db_clients` 项，配置正确的数据库 IP、用户名和密码。

开始编译并运行后端：
```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j2
./cmake-build-release/src/Project_Tracker ./config/config.prod.json
```

### 4. 前端打包与 Nginx 部署
在前端目录下安装依赖并打包：
```bash
cd frontend
npm install
npm run build
```
使用 Nginx 托管打包产物 `frontend/dist`，并反向代理后端 API（参考 `deploy/nginx/` 目录下的 Nginx 配置示例）。

---

## 📝 开发与回滚建议

若要在 `feature/ticket-system` 分支上继续开发，请遵循以下规范：
* **分支追踪**：请在 `feature/ticket-system` 分支上提交所有更改。
* **编译检查**：在提交前端代码前，必须在 `frontend/` 下运行 `npm run build` 确保 TypeScript 强类型检查和 Vite 打包正常通过。
* **安全回退**：如果您在后续开发中遇到异常需要回滚，可随时使用 `git reset --hard cc12326` 回退到包含“统计报表”在内的最稳定版本。
