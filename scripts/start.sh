#!/bin/bash

# Talkbox 服务器启动脚本

PORT=${1:-8080}

echo "启动 Talkbox 聊天服务器..."
echo "端口: $PORT"
echo "按 Ctrl+C 停止服务器"
echo ""

# 创建必要的目录
mkdir -p uploads

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"

# 启动服务器
"$PROJECT_ROOT/build/talkbox-server" $PORT
