# Systemd Deployment

本文档说明如何在 Linux 机器上使用 `systemd` 托管当前项目后端。

当前约定：

- 部署用户：`ubzy`
- 部署根目录：`/opt/project-tracker`
- 后端二进制路径：`/opt/project-tracker/bin/Project_Tracker`
- 生产配置路径：`/opt/project-tracker/config/config.prod.json`
- `systemd` 服务文件样例：`deploy/systemd/project-tracker.service`

## 1. 准备部署目录

先创建部署目录：

```bash
sudo mkdir -p /opt/project-tracker/bin
sudo mkdir -p /opt/project-tracker/config
```

把目录所有者交给当前部署用户：

```bash
sudo chown -R ubzy:ubzy /opt/project-tracker
```

## 2. 准备后端程序与配置

先在本地编译发布版本：

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j2
```

把后端二进制复制到部署目录：

```bash
cp cmake-build-release/src/Project_Tracker /opt/project-tracker/bin/Project_Tracker
```

从模板复制生产配置：

```bash
cp config/config.example.json /opt/project-tracker/config/config.prod.json
```

然后手动编辑：

- 数据库地址、库名、用户名、密码
- `connection_number`
- 是否保留 `custom_config.local_ai`

## 3. 安装 systemd 服务文件

把仓库中的服务文件复制到系统目录：

```bash
sudo cp deploy/systemd/project-tracker.service /etc/systemd/system/project-tracker.service
```

让 `systemd` 重新加载配置：

```bash
sudo systemctl daemon-reload
```

## 4. 启动服务

先手动启动一次：

```bash
sudo systemctl start project-tracker.service
```

查看当前状态：

```bash
sudo systemctl status project-tracker.service
```

如果确认没问题，再启用开机自启：

```bash
sudo systemctl enable project-tracker.service
```

也可以一步完成启用并立即启动：

```bash
sudo systemctl enable --now project-tracker.service
```

## 5. 查看日志

查看服务最近日志：

```bash
sudo journalctl -u project-tracker.service -n 100
```

持续跟踪日志输出：

```bash
sudo journalctl -u project-tracker.service -f
```

## 6. 更新程序后的重启方式

如果后端二进制或配置文件有更新，覆盖部署目录中的文件后重启服务：

```bash
sudo systemctl restart project-tracker.service
```

重启后再次确认状态：

```bash
sudo systemctl status project-tracker.service
```

## 7. 常用命令

启动：

```bash
sudo systemctl start project-tracker.service
```

停止：

```bash
sudo systemctl stop project-tracker.service
```

重启：

```bash
sudo systemctl restart project-tracker.service
```

查看是否开机自启：

```bash
sudo systemctl is-enabled project-tracker.service
```

## 8. 常见坑

- `config.prod.json` 里的数据库用户、密码与本机 PostgreSQL 实际账号不一致
  现象：`/api/auth/me` 这类未登录接口还能返回 `401`，但登录接口长时间无响应或明显变慢。
  排查：先用 `psql` 按配置文件中的同一组参数手动连接数据库，确认账号密码确实可用。

- 修改了 `project-tracker.service` 之后忘记执行 `daemon-reload`
  现象：你已经改了服务文件，但 `systemd` 仍按旧配置启动。
  处理：
  ```bash
  sudo systemctl daemon-reload
  sudo systemctl restart project-tracker.service
  ```

- 只重新编译了程序，但没有把新二进制复制到 `/opt/project-tracker/bin`
  现象：仓库里的代码已经更新，但服务实际跑的还是旧版本。
  处理：重新复制 `cmake-build-release/src/Project_Tracker` 到部署目录，再重启服务。

- `config.prod.json` 里保留了本机不可用的 `local_ai` 路径
  现象：涉及 AI 功能的接口报错，或者启动后部分功能不可用。
  处理：如果当前机器不打算启用本地 AI，就先删除 `custom_config.local_ai`，只保留 `threads_num`。

- 部署目录权限不对
  现象：服务启动失败，日志中可能出现“文件不存在”“无权限访问配置文件”之类问题。
  处理：确认 `/opt/project-tracker` 及其子目录属于当前运行用户 `ubzy`。

- 只看前端页面，不看服务日志
  现象：浏览器里只表现为“登录失败”或“没有反应”，但看不到真正原因。
  处理：优先执行：
  ```bash
  sudo systemctl status project-tracker.service
  sudo journalctl -u project-tracker.service -n 100
  ```
