# Nginx 本地统一入口实践计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在本机通过 `http://192.168.40.74` 建立 `Nginx -> Vite dev server + /api -> Drogon` 的统一入口，并保留前端热更新。

**Architecture:** 第一阶段不强制前端打包，`Nginx` 监听 `80` 端口，前端页面请求反向代理到 `Vite dev server`，`/api` 请求反向代理到 `127.0.0.1:8080`。前端稳定后，再切换为 `Nginx` 直接托管静态产物。

**Tech Stack:** Nginx, Vue 3, Vite, Drogon, Ubuntu 24.04

---

### Task 1: 让 Vite dev server 可作为 Nginx 上游

**Files:**
- Modify: `frontend/vite.config.ts`
- Record: `design-flow/05-nginx-local-learning-and-deployment.md`

- [ ] **Step 1: 放宽本地学习阶段的 Vite 访问限制**

把 `frontend/vite.config.ts` 的 `server` 段改为允许局域网访问，并固定开发端口：

```ts
  server: {
    host: '0.0.0.0',
    port: 5173,
    strictPort: true,
    allowedHosts: true,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true,
      },
    },
  },
```

- [ ] **Step 2: 解释这一步为什么必要**

说明：

- `host: '0.0.0.0'` 让开发服务器不只监听本机回环地址
- `port: 5173` 和 `strictPort: true` 让后续 `Nginx` 上游目标固定
- `allowedHosts: true` 避免当前仅允许 `www.project.test` 导致 IP 访问被拦截

- [ ] **Step 3: 记录到学习文档**

把本步骤的目标、改动原因和后续验证方式补到 `design-flow/05-nginx-local-learning-and-deployment.md`。

### Task 2: 启动并验证 Vite dev server

**Files:**
- Record: `design-flow/05-nginx-local-learning-and-deployment.md`

- [ ] **Step 1: 在前端目录启动开发服务器**

运行：

```bash
npm run dev
```

Expected:

- Vite 成功启动
- 监听地址包含 `0.0.0.0:5173`

- [ ] **Step 2: 验证本机可直接访问**

访问：

```text
http://127.0.0.1:5173
```

Expected:

- 能打开当前项目前端页面

- [ ] **Step 3: 验证局域网 IP 可直连**

访问：

```text
http://192.168.40.74:5173
```

Expected:

- 当前机器和局域网其他设备都能打开页面

### Task 3: 准备 Nginx 统一入口配置模板

**Files:**
- Create: `design-flow/nginx-project-tracker.local.conf`
- Record: `design-flow/05-nginx-local-learning-and-deployment.md`

- [ ] **Step 1: 编写本地统一入口配置模板**

模板内容应包含：

```nginx
server {
    listen 80;
    server_name 192.168.40.74;

    location /api/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    location / {
        proxy_pass http://127.0.0.1:5173;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

- [ ] **Step 2: 解释配置分工**

说明：

- `/api/` 走后端
- `/` 走 `Vite dev server`
- `Upgrade/Connection` 用于支持开发期 HMR 所需的 WebSocket

### Task 4: 在系统 Nginx 中启用该站点

**Files:**
- System file: `/etc/nginx/sites-available/project-tracker.local`
- System link: `/etc/nginx/sites-enabled/project-tracker.local`
- Record: `design-flow/05-nginx-local-learning-and-deployment.md`

- [ ] **Step 1: 将模板写入系统站点配置**

运行：

```bash
sudo cp design-flow/nginx-project-tracker.local.conf /etc/nginx/sites-available/project-tracker.local
```

- [ ] **Step 2: 启用站点**

运行：

```bash
sudo ln -s /etc/nginx/sites-available/project-tracker.local /etc/nginx/sites-enabled/project-tracker.local
```

- [ ] **Step 3: 检查配置**

运行：

```bash
sudo nginx -t
```

Expected:

- `syntax is ok`
- `test is successful`

- [ ] **Step 4: 重载 Nginx**

运行：

```bash
sudo systemctl reload nginx
```

Expected:

- Nginx 成功重载，无报错

### Task 5: 通过统一入口验证完整链路

**Files:**
- Record: `design-flow/05-nginx-local-learning-and-deployment.md`

- [ ] **Step 1: 验证页面入口**

访问：

```text
http://192.168.40.74
```

Expected:

- 页面由 `Nginx` 统一入口打开
- 前端改动仍可热更新

- [ ] **Step 2: 验证接口入口**

Expected:

- 前端页面中的 `/api` 请求通过 `Nginx` 到达 Drogon
- 登录态 Cookie 仍可工作

- [ ] **Step 3: 验证局域网其他设备访问**

Expected:

- 其他设备也可通过 `http://192.168.40.74` 打开页面

- [ ] **Step 4: 记录问题与结论**

把访问结果、碰到的问题和修正方案写回 `design-flow/05-nginx-local-learning-and-deployment.md`。
