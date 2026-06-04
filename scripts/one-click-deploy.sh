#!/usr/bin/env bash

# ==============================================================================
# Project Tracker 一键自动化部署脚本
# 适用环境: Ubuntu 24.04+ (建议)
# 功能: 安装依赖、初始化数据库、编译后端、打包前端、配置 Nginx 与 Systemd
# ==============================================================================

set -euo pipefail

# --- 配置区 ---
APP_NAME="project-tracker"
INSTALL_DIR="/opt/${APP_NAME}"
DB_NAME="project_tracker"
DB_USER="project_tracker"
NGINX_CONF="/etc/nginx/sites-available/${APP_NAME}.conf"
SERVICE_FILE="/etc/systemd/system/${APP_NAME}.service"
CURRENT_USER=$(whoami)

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[INFO] $1${NC}"
}

error() {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

# 1. 权限检查
if [[ $EUID -ne 0 ]]; then
   error "本脚本必须以 root 权限运行 (sudo ./scripts/one-click-deploy.sh)"
fi

log "开始自动化部署流程..."

# 2. 安装系统依赖
log "安装系统基础依赖..."
apt update
apt install -y \
    git build-essential cmake pkg-config \
    postgresql postgresql-client libpq-dev \
    libargon2-dev \
    nginx curl

# 3. 安装 Node.js (用于前端构建)
if ! command -v npm &> /dev/null; then
    log "未检测到 Node.js，正在安装..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
    apt install -y nodejs
fi

# 4. 安装 Drogon (如果系统中不存在)
if ! cmake --find-package -DNAME=Drogon -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST &> /dev/null; then
    log "未检测到 Drogon，准备从源码编译安装..."
    TEMP_DIR=$(mktemp -d)
    pushd "$TEMP_DIR"
    git clone https://github.com/drogonframework/drogon
    cd drogon
    git submodule update --init
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
    cmake --build build -j$(nproc)
    cmake --install build
    popd
    rm -rf "$TEMP_DIR"
fi

# 5. 初始化数据库
log "配置 PostgreSQL 数据库..."
# 检查用户是否存在
if ! sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'" | grep -q 1; then
    read -sp "请输入新数据库用户 [$DB_USER] 的密码: " DB_PASS
    echo ""
    sudo -u postgres psql -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASS';"
    sudo -u postgres psql -c "CREATE DATABASE $DB_NAME OWNER $DB_USER;"
    sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;"
    
    # 刷新 Schema 权限 (PostgreSQL 15+ 默认不给 public 权限)
    sudo -u postgres psql -d "$DB_NAME" -c "ALTER SCHEMA public OWNER TO $DB_USER;"
    
    # 保存临时配置用于构建
    echo "DB_PASS=\"$DB_PASS\"" > .db_env
else
    log "数据库用户已存在，跳过创建。请确保 .db_env 中有正确密码或手动更新配置。"
    if [ ! -f .db_env ]; then
        read -sp "数据库已存在，请输入用户 [$DB_USER] 的密码以继续: " DB_PASS
        echo ""
        echo "DB_PASS=\"$DB_PASS\"" > .db_env
    fi
    source .db_env
fi

# 导入初始数据
log "导入数据库 Schema 与种子数据..."
PGPASSWORD="$DB_PASS" psql -h 127.0.0.1 -U "$DB_USER" -d "$DB_NAME" -f db/migrations/001_init_schema.sql
PGPASSWORD="$DB_PASS" psql -h 127.0.0.1 -U "$DB_USER" -d "$DB_NAME" -f db/seeds/002_prod_users.sql

# 6. 准备安装目录
log "创建安装目录 $INSTALL_DIR..."
mkdir -p "${INSTALL_DIR}/bin"
mkdir -p "${INSTALL_DIR}/config"
mkdir -p "${INSTALL_DIR}/www"
mkdir -p "${INSTALL_DIR}/uploads/tmp"

# 7. 编译后端
log "编译后端程序..."
if [ ! -f "config/config.prod.json" ]; then
    cp config/config.example.json config/config.prod.json
    # 替换数据库密码
    sed -i "s/\"passwd\": \".*\"/\"passwd\": \"$DB_PASS\"/" config/config.prod.json
fi

cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j$(nproc)
cp cmake-build-release/src/Project_Tracker "${INSTALL_DIR}/bin/"
cp config/config.prod.json "${INSTALL_DIR}/config/"

# 8. 打包前端
log "构建前端页面..."
pushd frontend
npm ci
npm run build
cp -r dist/* "${INSTALL_DIR}/www/"
popd

# 9. 配置 Nginx
log "配置 Nginx..."
if [ -f "deploy/nginx/project-tracker.conf" ]; then
    cp deploy/nginx/project-tracker.conf "$NGINX_CONF"
    # 修正 root 路径
    sed -i "s|root .*|root ${INSTALL_DIR}/www;|" "$NGINX_CONF"
    ln -sf "$NGINX_CONF" /etc/nginx/sites-enabled/
    rm -f /etc/nginx/sites-enabled/default || true
    nginx -t && systemctl restart nginx
fi

# 10. 配置 Systemd
log "配置 Systemd 服务..."
if [ -f "deploy/systemd/project-tracker.service" ]; then
    cp deploy/systemd/project-tracker.service "$SERVICE_FILE"
    # 修正路径与用户 (默认使用执行脚本的用户，或者根据 service 文件中的 User)
    sed -i "s|WorkingDirectory=.*|WorkingDirectory=${INSTALL_DIR}|" "$SERVICE_FILE"
    sed -i "s|ExecStart=.*|ExecStart=${INSTALL_DIR}/bin/Project_Tracker ${INSTALL_DIR}/config/config.prod.json|" "$SERVICE_FILE"
    # 如果 service 里是 ubzy，我们改成本脚本的执行者
    sed -i "s|User=.*|User=${CURRENT_USER}|" "$SERVICE_FILE"
    sed -i "s|Group=.*|Group=${CURRENT_USER}|" "$SERVICE_FILE"

    systemctl daemon-reload
    systemctl enable "${APP_NAME}"
    systemctl restart "${APP_NAME}"
fi

# 11. 权限修正
chown -R "${CURRENT_USER}:${CURRENT_USER}" "$INSTALL_DIR"

log "===================================================================="
log "部署完成！"
log "后端状态: $(systemctl is-active ${APP_NAME})"
log "Nginx 状态: $(systemctl is-active nginx)"
log "你可以通过 http://$(hostname -I | awk '{print $1}') 访问系统"
log "===================================================================="
