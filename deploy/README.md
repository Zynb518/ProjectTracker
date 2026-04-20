# Deploy Assets

当前目录用于存放项目部署相关资产，避免把部署文件散落在仓库根目录。

当前目录下已约定：

- `systemd/`: 后端服务单元文件样例
- `nginx/`: 前端静态托管与 `/api` 反向代理相关配置

当前已提供的部署文件包括：

- `deploy/systemd/project-tracker.service`
- `deploy/systemd/systemd-deployment.md`
- `deploy/nginx/project-tracker.conf`
- `deploy/nginx/nginx-deployment.md`

后续如果继续补齐部署方案，可在这里继续按类型扩展子目录，例如 `scripts/`。
