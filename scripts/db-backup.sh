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

# 尝试获取密码 (简单 grep 提取，实际生产建议用 jq)
if [ -f "$CONFIG_FILE" ]; then
    DB_PASS=$(grep -oP '"passwd":\s*"\K[^"]+' "$CONFIG_FILE" || echo "")
    if [ -n "$DB_PASS" ]; then
        export PGPASSWORD="$DB_PASS"
    fi
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
