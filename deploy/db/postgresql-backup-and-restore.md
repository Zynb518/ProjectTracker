# PostgreSQL Backup And Restore

本文档说明如何为当前项目的 PostgreSQL 数据库执行正式备份、恢复与恢复演练。

当前阶段约定：

- 当前项目暂不单独引入数据库迁移工具
- 如果后续确实需要调整表结构，先备份，再执行新增 SQL
- 正式数据库连接参数以 `/opt/project-tracker/config/config.prod.json` 为准
- 逻辑备份目录建议使用 `/var/backups/project-tracker`
- 正式发布顺序固定为：先备份，再发布，再做健康检查

## 1. 适用范围

当前仓库里的数据库脚本分工如下：

- `db/migrations/001_init_schema.sql`：当前初始化基线脚本
- `db/seeds/001_demo_data.sql`：开发/联调用演示数据
- `db/seeds/002_prod_users.sql`：正式环境初始化账号数据

由于当前项目表结构基本稳定，本阶段优先把备份与恢复做扎实，不额外引入 Flyway、Liquibase 一类迁移框架。

## 2. 准备数据库连接参数

先确认正式环境的数据库连接参数。建议直接对照：

```text
/opt/project-tracker/config/config.prod.json
```

至少要确认：

- 数据库地址
- 数据库端口
- 数据库名
- 数据库用户名
- 数据库密码

如果当前机器还没有安装 PostgreSQL 客户端命令，可先安装：

```bash
sudo apt update
sudo apt install -y postgresql-client
```

## 3. 配置 .pgpass

为了避免每次备份或恢复都手动输入密码，建议为部署用户配置 `~/.pgpass`。

写入示例：

```bash
cat > ~/.pgpass <<'EOF'
127.0.0.1:5432:project_tracker:project_tracker:your-db-password
EOF
```

限制权限：

```bash
chmod 600 ~/.pgpass
```

格式说明：

```text
host:port:dbname:user:password
```

如果正式环境使用的不是 `127.0.0.1:5432:project_tracker:project_tracker`，请按实际值替换。

## 4. 手工执行一次正式备份

发布前至少手工执行一次数据库备份。

```bash
BACKUP_DIR=/var/backups/project-tracker
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=project_tracker
DB_USER=project_tracker

mkdir -p "$BACKUP_DIR"

ts=$(date +%F-%H%M%S)

pg_dump \
  -h "$DB_HOST" \
  -p "$DB_PORT" \
  -U "$DB_USER" \
  -d "$DB_NAME" \
  -Fc \
  --no-owner \
  --no-privileges \
  -f "$BACKUP_DIR/${DB_NAME}-${ts}.dump"

echo "backup done: $BACKUP_DIR/${DB_NAME}-${ts}.dump"
```

说明：

- `-Fc` 表示使用 PostgreSQL custom format
- `--no-owner` 和 `--no-privileges` 可以减少跨环境恢复时的权限干扰
- 该命令支持在线逻辑备份，不要求先停后端服务

备份完成后，至少做两个快速检查：

```bash
ls -lh /var/backups/project-tracker
```

```bash
pg_restore -l /var/backups/project-tracker/project_tracker-2026-04-20-010000.dump | head
```

如果第二条命令能正常列出 TOC 内容，说明备份文件至少不是一个空壳文件。

## 5. 推荐使用固定备份脚本

为了避免手工敲错参数，建议把备份命令固化为脚本，例如：

```text
/usr/local/bin/project-tracker-backup.sh
```

脚本示例：

```bash
#!/usr/bin/env bash
set -euo pipefail

BACKUP_DIR=/var/backups/project-tracker
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=project_tracker
DB_USER=project_tracker
KEEP_DAYS=14

mkdir -p "$BACKUP_DIR"

ts=$(date +%F-%H%M%S)
backup_file="$BACKUP_DIR/${DB_NAME}-${ts}.dump"

pg_dump \
  -h "$DB_HOST" \
  -p "$DB_PORT" \
  -U "$DB_USER" \
  -d "$DB_NAME" \
  -Fc \
  --no-owner \
  --no-privileges \
  -f "$backup_file"

find "$BACKUP_DIR" -type f -name "${DB_NAME}-*.dump" -mtime +$KEEP_DAYS -delete

echo "backup done: $backup_file"
```

给脚本增加执行权限：

```bash
chmod +x /usr/local/bin/project-tracker-backup.sh
```

手工执行一次：

```bash
/usr/local/bin/project-tracker-backup.sh
```

当前脚本只负责：

- 生成一份新备份
- 保留最近 `14` 天备份
- 清理更早的历史备份

如果你后续要接入 `cron` 或 `systemd timer`，直接复用这份脚本即可。

## 6. 恢复演练

备份文件生成成功，不代表它一定可恢复。建议至少每月做一次恢复演练。

恢复演练时，不要直接覆盖正式库；先恢复到一个临时库。

先创建恢复测试库：

```bash
RESTORE_DB=project_tracker_restore

createdb \
  -h 127.0.0.1 \
  -p 5432 \
  -U project_tracker \
  "$RESTORE_DB"
```

再执行恢复：

```bash
pg_restore \
  -h 127.0.0.1 \
  -p 5432 \
  -U project_tracker \
  -d "$RESTORE_DB" \
  --no-owner \
  --no-privileges \
  /var/backups/project-tracker/project_tracker-2026-04-20-010000.dump
```

恢复后至少检查：

```bash
psql -h 127.0.0.1 -p 5432 -U project_tracker -d "$RESTORE_DB" -c '\dt'
```

```bash
psql -h 127.0.0.1 -p 5432 -U project_tracker -d "$RESTORE_DB" -c 'select count(*) from sys_user;'
```

```bash
psql -h 127.0.0.1 -p 5432 -U project_tracker -d "$RESTORE_DB" -c 'select count(*) from project;'
```

如果只是恢复演练，验证完成后可以删除该临时库：

```bash
dropdb -h 127.0.0.1 -p 5432 -U project_tracker "$RESTORE_DB"
```

## 7. 正式环境恢复原则

如果未来需要真的从备份恢复正式库，建议遵循这个顺序：

1. 先停后端服务
2. 确认没有活跃数据库连接
3. 先保留当前故障现场信息，再决定是否重建正式库
4. 使用 `pg_restore` 把最近一次可用备份恢复到目标库
5. 启动后端服务
6. 执行健康检查，确认 `/api/health` 和 `/api/health/ready` 正常

如果没有先验证过恢复流程，不要在正式环境第一次尝试。

## 8. 可选：备份数据库全局对象

如果你未来需要做整机迁移或重建 PostgreSQL 实例，可以额外备份角色和权限定义：

```bash
ts=$(date +%F-%H%M%S)

pg_dumpall \
  -h 127.0.0.1 \
  -p 5432 \
  -U postgres \
  --globals-only \
  > /var/backups/project-tracker/project_tracker-globals-$ts.sql
```

这一步不是每次发布都必须，但当你需要迁移整套 PostgreSQL 实例时会更有帮助。

## 9. 建议的正式规则

当前项目建议至少执行以下规则：

- 正式库每天自动备份 `1` 次
- 每次发布前手工再备份 `1` 次
- 备份文件至少保留最近 `14` 天
- 每月至少做 `1` 次恢复演练
- 发布流程固定为：先备份，再发布，再健康检查

## 10. 常见坑

- 只做了备份，没有做恢复演练
  现象：文件在，但真正出问题时不知道是否能恢复。
  处理：至少每月恢复到临时库演练一次。

- 直接沿用开发库连接参数
  现象：备份脚本能跑，但实际备份的不是正式库。
  处理：所有正式库参数都以 `/opt/project-tracker/config/config.prod.json` 为准。

- 备份文件保存在临时目录
  现象：系统清理后备份文件丢失。
  处理：固定保存到 `/var/backups/project-tracker` 这类长期目录。

- 只看到 `backup done` 就认为备份可用
  现象：真正恢复时才发现文件损坏或流程不完整。
  处理：至少执行一次 `pg_restore -l`，并定期做恢复演练。

- 发布前忘记先备份
  现象：上线后发现问题，但没有发布前快照可回退。
  处理：把“先备份”当成固定发布步骤，而不是临时决定。
