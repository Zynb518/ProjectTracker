# Nginx Deployment

本文档说明如何在 Linux 机器上为当前项目部署正式 `nginx` 入口。

当前约定：

- 站点配置模板：`deploy/nginx/project-tracker.conf`
- 系统站点文件：`/etc/nginx/sites-available/project-tracker.local`
- 系统启用链接：`/etc/nginx/sites-enabled/project-tracker.local`
- 前端正式静态目录：`/var/www/project-tracker/dist`
- 后端服务地址：`http://127.0.0.1:8080`

当前这份正式配置的目标是：

- `nginx` 统一监听 `80` 端口
- 前端页面由 `nginx` 直接托管
- `/api` 请求反向代理到 Drogon 后端
- 不硬编码固定局域网 IP，使用 `server_name _;` 承接当前机器入口

## 1. 准备前端正式产物目录

先创建静态资源目录：

```bash
sudo mkdir -p /var/www/project-tracker/dist
```

如果当前机器上目录权限过紧，至少要确保 `nginx` 可以读取该目录及其文件。

## 2. 构建前端正式包

进入前端目录并构建：

```bash
cd frontend
npm ci
npm run build
```


构建完成后，前端产物位于：

```text
frontend/dist
```

## 3. 拷贝前端正式产物

把前端构建结果复制到正式静态目录：

```bash
sudo cp -r dist/. /var/www/project-tracker/dist/
```

如果后续前端有更新，重新构建后再次覆盖该目录即可。

## 4. 安装站点配置

把仓库中的正式模板复制到系统目录：

```bash
sudo cp deploy/nginx/project-tracker.conf /etc/nginx/sites-available/project-tracker.local
```

当前模板内容见：

- `deploy/nginx/project-tracker.conf`

这份模板的核心行为是：

- `root /var/www/project-tracker/dist;`
- `location /api/ -> http://127.0.0.1:8080`
- `location / -> try_files $uri $uri/ /index.html`

## 5. 启用项目站点

如果当前还没有软链接，则创建：

```bash
sudo ln -s /etc/nginx/sites-available/project-tracker.local /etc/nginx/sites-enabled/project-tracker.local
```

如果已经存在该链接，就不需要重复创建。

## 6. 下线默认站点

正式部署时，建议移除系统默认站点，避免它继续作为 `default_server` 干扰项目入口：

```bash
sudo unlink /etc/nginx/sites-enabled/default
```

这样做的目的不是“清理样板文件”，而是明确让当前项目站点成为唯一正式入口。

## 7. 检查配置并重载

先检查配置语法：

```bash
sudo nginx -t
```

如果输出 `syntax is ok` 和 `test is successful`，再重载：

```bash
sudo systemctl reload nginx
```

## 8. 验证正式入口

至少验证以下几项：

### 8.1 页面入口

访问：

```text
http://当前机器局域网IP/
```

预期：

- 可以打开前端页面

### 8.2 SPA 路由回退

直接访问或刷新：

```text
http://当前机器局域网IP/login
http://当前机器局域网IP/projects
```

预期：

- 不会返回 `404`

这是因为正式配置里使用了：

```nginx
try_files $uri $uri/ /index.html;
```

用于兼容当前前端的 `createWebHistory()` 路由模式。

### 8.3 接口请求

确认页面里的 `/api` 请求还能正常到达后端。

例如未登录时访问：

```text
/api/auth/me
```

返回 `401` 是正常现象，说明请求已经到达后端鉴权链路，而不是 `nginx` 配置错误。

## 9. 配置更新后的常规操作

如果只是修改了 `nginx` 站点配置：

```bash
sudo nginx -t
sudo systemctl reload nginx
```

如果只是前端页面更新：

1. 重新执行 `npm run build`
2. 重新复制 `dist` 到 `/var/www/project-tracker/dist/`

通常不需要重启后端服务。

## 10. 常见坑

- 忘记构建前端正式包
  现象：`nginx` 配置是对的，但页面空白或目录下没有实际资源。
  处理：重新执行 `npm ci && npm run build`，再复制 `dist`。

- `default` 站点还留在 `sites-enabled`
  现象：请求入口行为混乱，看起来“偶尔能打开页面”，但不确定命中了哪个站点。
  处理：移除 `/etc/nginx/sites-enabled/default`，只保留项目站点。

- `/var/www/project-tracker/dist` 路径不对或没有内容
  现象：访问首页返回 `404` 或静态资源加载失败。
  处理：确认 `root` 配置与实际文件路径一致，并重新复制前端产物。

- 后端没有稳定运行在 `127.0.0.1:8080`
  现象：页面能打开，但接口报 `502` 或请求失败。
  处理：先检查后端服务状态，再检查 `nginx` 错误日志。

- 只检查页面，不检查 `nginx` 结果
  现象：浏览器里只看到“打不开”或“接口失败”，但不知道是前端、`nginx` 还是后端的问题。
  处理：优先执行：
  ```bash
  sudo nginx -t
  sudo systemctl status nginx
  sudo tail -n 100 /var/log/nginx/error.log
  ```
