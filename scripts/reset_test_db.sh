#!/bin/bash
# 数据库重置脚本（用于在测试运行前清洗和重置数据）
set -e

# 检测是否安装了 psql 客户端，若未安装则自动安装（在 tests 容器中运行有用）
if ! command -v psql &> /dev/null; then
    echo "psql client not found, installing..."
    # 仅在有 root 权限的容器环境下自动安装
    if [ "$EUID" -ne 0 ]; then
        echo "Error: Please run as root or install postgresql-client manually."
        exit 1
    fi
    apt-get update && apt-get install -y postgresql-client
fi

DB_HOST=${DB_HOST:-"127.0.0.1"}
DB_PORT=${DB_PORT:-"5432"}
DB_NAME=${DB_NAME:-"tracker_db_dev"}
DB_USER=${DB_USER:-"postgres"}
DB_PASSWORD=${DB_PASSWORD:-"postgres"}

echo "Connecting to $DB_HOST:$DB_PORT ($DB_NAME) as $DB_USER..."

# 1. 清空 public 命名空间
PGPASSWORD=$DB_PASSWORD psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"

# 2. 重新初始化表结构
PGPASSWORD=$DB_PASSWORD psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -f db/migrations/001_init_schema.sql

# 3. 导入演示种子数据
PGPASSWORD=$DB_PASSWORD psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -f db/seeds/001_demo_data.sql

echo "Database reset successfully!"
