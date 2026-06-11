# 第一阶段 编译环境
FROM ubuntu:24.04 AS builder

# 设置环境变量，避免交互式
ENV DEBIAN_FRONTEND=noninteractive

# RUN 命令用于在镜像内执行 shell 命令
# 这里安装 c++ 编译必须的工具链和 drogon 依赖库
RUN apt-get update && apt-get install -y \
    gcc g++ cmake \
    ninja-build \
    libdrogon-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    openssl libssl-dev \
    pkg-config \
    libpq-dev \
    libsqlite3-dev \
    libargon2-dev \
    libmariadb-dev-compat \
    libbrotli-dev \
    libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*  # 用&&连接, 防止镜像层保存软件包列表信息

# 设置工作目录
WORKDIR /app

# 把当前目录的所有文件都拷贝到 容器的/app 目录下
COPY . .

# 执行编译步骤
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --target Project_Tracker

# 第二阶段 运行环境
FROM ubuntu:24.04

WORKDIR /app

# 从builder阶段，把编译好的可执行文件偷过来
COPY --from=builder /app/build/src/Project_Tracker /app/

# 假设后端有配置文件 也需要拷贝过去
COPY --from=builder /app/config/config.example.json /app/config.json

# 安装运行二进制文件所必须的动态库
RUN apt-get update && apt-get install -y \
    libdrogon1.1 \
    libargon2-1 \
    libjsoncpp25 \
    libssl3 \
    libpg5 \
    libsqlite3-0 \
    libmariadb3 \
    libbrotli1 \
    libhiredis1.1.0 \
    && rm -rf /var/lib/apt/lists/*

# 声明容器运行时监听的端口（仅作说明）
EXPOSE 8080

# 容器启动时执行的最终命令
CMD ["./Project_Tracker"]
