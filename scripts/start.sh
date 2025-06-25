#!/bin/bash

# Talkbox 服务器启动脚本

PORT=${1:-8080}

echo "启动 Talkbox 聊天服务器..."
echo "端口: $PORT"
echo "按 Ctrl+C 停止服务器"
echo ""

# 创建必要的目录
mkdir -p uploads

# 启动服务器
../build/talkbox-server $PORT
