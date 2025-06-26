#!/bin/bash

# 测试新添加的接口

# 启动服务器
echo "启动服务器..."
cd /home/ltc/Program/Talkbox/Talkbox-server
./build/talkbox-server &
SERVER_PID=$!

# 等待服务器启动
sleep 2

echo "测试API接口..."

# 1. 注册用户
echo "1. 注册用户测试..."
curl -X POST http://localhost:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"testpass"}'
echo ""

# 2. 登录用户
echo "2. 登录用户测试..."
LOGIN_RESPONSE=$(curl -s -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"testpass"}')
echo $LOGIN_RESPONSE

# 提取token (简单的提取方法)
TOKEN=$(echo $LOGIN_RESPONSE | grep -o '"token":"[^"]*"' | sed 's/"token":"\([^"]*\)"/\1/')
echo "提取的Token: $TOKEN"
echo ""

# 3. 测试获取用户信息
echo "3. 获取用户信息测试..."
curl -X GET http://localhost:8080/api/user/profile \
  -H "Authorization: Bearer $TOKEN"
echo ""

# 4. 创建帖子
echo "4. 创建帖子测试..."
curl -X POST http://localhost:8080/api/create_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"测试帖子","content":"这是一个测试帖子的内容"}'
echo ""

# 5. 获取帖子列表
echo "5. 获取帖子列表测试..."
curl -X GET http://localhost:8080/api/get_posts
echo ""

# 6. 测试获取单个帖子详情 (假设帖子ID为1)
echo "6. 获取帖子详情测试..."
curl -X GET http://localhost:8080/api/post/1
echo ""

# 关闭服务器
echo "关闭服务器..."
kill $SERVER_PID

echo "测试完成！"
