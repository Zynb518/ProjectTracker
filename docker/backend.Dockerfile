# 第一阶段 编译阶段 官方镜像
FROM drogonframework/drogon:latest AS builder

ARG APT_MIRROR=http://mirrors.aliyun.com/ubuntu

WORKDIR /app

COPY docker/use-apt-mirror.sh /tmp/use-apt-mirror.sh

RUN set -eux; \
    sh /tmp/use-apt-mirror.sh "$APT_MIRROR"; \
    apt-get update; \
    apt-get install -y --no-install-recommends libargon2-dev; \
    rm -rf /var/lib/apt/lists/*

# 拷贝源码
COPY . .

# 直接编译
# 官方镜像可能没有 NINJA, 使用默认的MAKEFILE
RUN cmake -S . -B docker-build-release -DCMAKE_BUILD_TYPE=Release -DPROJECT_TRACKER_BUILD_TESTS=OFF
RUN cmake --build docker-build-release --target Project_Tracker



# 第二阶段 运行环境
FROM ubuntu:22.04

ARG APT_MIRROR=http://mirrors.aliyun.com/ubuntu

WORKDIR /app

COPY --from=builder /tmp/use-apt-mirror.sh /tmp/use-apt-mirror.sh

# 1. 拷贝你自己的程序
COPY --from=builder /app/docker-build-release/src/Project_Tracker /app/
COPY --from=builder /app/config/config.template.json /app/config/config.template.json

# 2. 关键：拷贝官方镜像里已经编译好的 Drogon 库及其依赖
COPY --from=builder /usr/local/lib/libdrogon* /usr/local/lib/
# 2. 关键：直接拷贝 builder 里的 hiredis 库
COPY --from=builder /usr/lib/x86_64-linux-gnu/libhiredis.so.0.14 /usr/lib/x86_64-linux-gnu/

# 3. 安装系统级的基础库（这些是 Ubuntu 仓库里有的）
RUN set -eux; \
    sh /tmp/use-apt-mirror.sh "$APT_MIRROR"; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        gettext-base \
        libargon2-1 \
        libpq5 \
        libsqlite3-0 \
        libmariadb3 \
        libhiredis0.14 \
        libjsoncpp25 \
        uuid-runtime \
        libc-ares2 \
        libssl3; \
    rm -f /tmp/use-apt-mirror.sh; \
    rm -rf /var/lib/apt/lists/*


# 4. 刷新动态库缓存，让系统认出刚拷过来的 libdrogon
RUN ldconfig

# 创建非 root 用户并修改 /app 拥有权
RUN groupadd -r appgroup && useradd -r -g appgroup -d /app -s /sbin/nologin appuser \
    && chown -R appuser:appgroup /app

# 切换为普通非 root 用户
USER appuser

# 声明容器运行时监听的端口（仅作说明）
EXPOSE 8080

# 容器启动时执行的最终命令
CMD ["sh", "-c", "envsubst < /app/config/config.template.json > /app/config/config.dev.json && ./Project_Tracker"]
