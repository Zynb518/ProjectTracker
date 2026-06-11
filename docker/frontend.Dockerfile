# --- 第一阶段：构建 ---
FROM node:20 AS builder

WORKDIR /app

# 先拷贝 package.json 及其 lock 文件
# 这样做的好处是：只要你没改依赖，这一层安装就会被缓存，下次构建秒过
COPY frontend/package*.json ./

# 进入 frontend 目录的逻辑在 COPY 时已经处理了，现在我们在容器的 /app 目录下
RUN npm install

# 拷贝前端所有源码
COPY frontend/ .

# 执行构建
RUN npm run build

# --- 第二阶段：运行 (使用极其轻量的 Nginx) ---
FROM nginx:stable-alpine

# 将第一阶段生成的 dist 文件夹内容，拷贝到 Nginx 的默认静态文件目录
COPY --from=builder /app/dist /usr/share/nginx/html

# 暴露 80 端口
EXPOSE 80
