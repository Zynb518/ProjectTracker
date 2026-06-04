#!/usr/bin/env bash
# ==============================================================================
# PostgreSQL 自动备份脚本
# ==============================================================================

set -euo pipefail

# --- 配置区 ---
BACKUP_DIR="/var/backups/project-tracker"
DB_NAME="project_tracker"
DB_USER="project_tracker"
KEEP_DAYS=14
# 从 config.prod.json 中读取密码 (假设在 /opt/project-tracker/config/config.prod.json)
CONFIG_FILE="/opt/project-tracker/config/config.prod.json"

# 颜色定义
GREEN='\033[0;32m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

mkdir -p "$BACKUP_DIR"

# 尝试获取密码
if [ -f "$CONFIG_FILE" ]; then
    # 更加兼容的提取方式，处理可能存在的空格
    DB_PASS=$(grep '"passwd"' "$CONFIG_FILE" | head -n 1 | cut -d '"' -f 4 || echo "")
    if [ -n "$DB_PASS" ]; then
        export PGPASSWORD="$DB_PASS"
        log "已从配置文件加载数据库密码。"
    else
        log "警告: 未能从 $CONFIG_FILE 中提取到密码，将尝试免密连接。"
    fi
else
    log "错误: 找不到配置文件 $CONFIG_FILE"
    exit 1
fi

ts=$(date +%F-%H%M%S)
backup_file="$BACKUP_DIR/${DB_NAME}-${ts}.dump"

log "开始备份数据库 [$DB_NAME] 到 $backup_file ..."

pg_dump \
  -h 127.0.0.1 \
  -U "$DB_USER" \
  -d "$DB_NAME" \
  -Fc \
  --no-owner \
  --no-privileges \
  -f "$backup_file"

log "备份完成。"

log "清理超过 $KEEP_DAYS 天的历史备份..."
find "$BACKUP_DIR" -type f -name "${DB_NAME}-*.dump" -mtime +$KEEP_DAYS -delete

log "清理完成。"
