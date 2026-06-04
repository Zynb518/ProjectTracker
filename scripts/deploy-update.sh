#!/usr/bin/env bash

# ==============================================================================
# Project Tracker 增量部署/更新脚本
# 适用环境: 环境已就绪 (DB/Nginx/Drogon 已装)，仅需更新代码并重新发布
# ==============================================================================

set -euo pipefail

# --- 配置区 ---
APP_NAME="project-tracker"
INSTALL_DIR="/opt/${APP_NAME}"
SERVICE_NAME="${APP_NAME}.service"

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log() { echo -e "${GREEN}[INFO] $1${NC}"; }
step() { echo -e "${BLUE}>> $1${NC}"; }

# 1. 更新代码
log "开始部署更新流程..."
step "拉取最新代码 (git pull)..."
git pull

# 2. 编译后端
step "开始编译后端..."
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j2

# 3. 构建前端
step "开始构建前端..."
pushd frontend
npm install  # 使用 install 确保依赖同步
npm run build
popd

# 4. 同步产物到目标目录
step "同步产物到 ${INSTALL_DIR}..."

# 确保目标目录存在
sudo mkdir -p "${INSTALL_DIR}/bin" "${INSTALL_DIR}/www"

# 拷贝后端二进制
sudo cp cmake-build-release/src/Project_Tracker "${INSTALL_DIR}/bin/"

# 拷贝前端静态文件 (先清理旧的)
sudo rm -rf "${INSTALL_DIR}/www/*"
sudo cp -r frontend/dist/* "${INSTALL_DIR}/www/"

# 5. 重启服务
step "重启系统服务..."
if systemctl list-unit-files | grep -q "${SERVICE_NAME}"; then
    sudo systemctl restart "${SERVICE_NAME}"
    log "服务已重启。"
else
    log "未检测到 systemd 服务 [${SERVICE_NAME}]，请手动启动或运行安装脚本配置。"
fi

log "===================================================================="
log "部署更新完成！"
log "===================================================================="
