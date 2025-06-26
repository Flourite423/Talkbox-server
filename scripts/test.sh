#!/bin/bash

# Talkbox 完整API测试脚本

SERVER_URL="http://localhost:8080"
ALICE_TOKEN=""
BOB_TOKEN=""
CHARLIE_TOKEN=""
GROUP_ID=""
POST_ID=""

echo "=========================================="
echo "         Talkbox 完整API测试"
echo "=========================================="
echo "请确保服务器已经在 8080 端口启动"
echo ""

# ========================================
# 用户管理API测试
# ========================================
echo "=========================================="
echo "1. 用户管理API测试"
echo "=========================================="

echo "1.1 测试用户注册 - Alice..."
REGISTER_RESPONSE=$(curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"123456"}')
echo $REGISTER_RESPONSE | jq .
echo ""

echo "1.2 测试用户注册 - Bob..."
curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"bob","password":"123456"}' | jq .
echo ""

echo "1.3 测试用户注册 - Charlie..."
curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"charlie","password":"123456"}' | jq .
echo ""

echo "1.4 测试重复注册（应该失败）..."
curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"123456"}' | jq .
echo ""

echo "1.5 测试Alice用户登录..."
LOGIN_RESPONSE=$(curl -s -X POST $SERVER_URL/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"123456"}')
echo $LOGIN_RESPONSE | jq .
ALICE_TOKEN=$(echo $LOGIN_RESPONSE | jq -r '.data.token // empty')
echo "Alice Token: $ALICE_TOKEN"
echo ""

echo "1.6 测试Bob用户登录..."
BOB_LOGIN_RESPONSE=$(curl -s -X POST $SERVER_URL/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"bob","password":"123456"}')
echo $BOB_LOGIN_RESPONSE | jq .
BOB_TOKEN=$(echo $BOB_LOGIN_RESPONSE | jq -r '.data.token // empty')
echo "Bob Token: $BOB_TOKEN"
echo ""

echo "1.7 测试Charlie用户登录..."
CHARLIE_LOGIN_RESPONSE=$(curl -s -X POST $SERVER_URL/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"charlie","password":"123456"}')
echo $CHARLIE_LOGIN_RESPONSE | jq .
CHARLIE_TOKEN=$(echo $CHARLIE_LOGIN_RESPONSE | jq -r '.data.token // empty')
echo "Charlie Token: $CHARLIE_TOKEN"
echo ""

echo "1.8 测试错误密码登录（应该失败）..."
curl -s -X POST $SERVER_URL/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"wrongpassword"}' | jq .
echo ""

# ========================================
# 即时通讯API测试
# ========================================
echo "=========================================="
echo "2. 即时通讯API测试"
echo "=========================================="

echo "2.1 Alice发送私聊消息给Bob..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"receiver_id":"2","content":"Hi Bob, 这是来自Alice的私聊消息","type":"text"}' | jq .
echo ""

echo "2.2 Bob回复私聊消息给Alice..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d '{"receiver_id":"1","content":"Hi Alice, 收到你的消息了！","type":"text"}' | jq .
echo ""

echo "2.3 Charlie发送私聊消息给Alice..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d '{"receiver_id":"1","content":"Hi Alice, 我是Charlie","type":"text"}' | jq .
echo ""

echo "2.4 Alice回复Charlie..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"receiver_id":"3","content":"Hi Charlie, 很高兴认识你！","type":"text"}' | jq .
echo ""

echo "2.5 测试无效token发送消息（应该失败）..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer invalidtoken" \
  -d '{"receiver_id":"1","content":"这条消息不应该发送成功","type":"text"}' | jq .
echo ""

echo "2.6 Alice获取所有消息..."
curl -s -X GET $SERVER_URL/api/get_messages \
  -H "Authorization: Bearer $ALICE_TOKEN" | jq .
echo ""

echo "2.7 Alice获取联系人列表..."
curl -s -X GET $SERVER_URL/api/get_contacts \
  -H "Authorization: Bearer $ALICE_TOKEN" | jq .
echo ""

echo "2.8 Bob获取联系人列表..."
curl -s -X GET $SERVER_URL/api/get_contacts \
  -H "Authorization: Bearer $BOB_TOKEN" | jq .
echo ""

# ========================================
# 群组管理API测试
# ========================================
echo "=========================================="
echo "3. 群组管理API测试"
echo "=========================================="

echo "3.1 Alice创建群组..."
curl -s -X POST $SERVER_URL/api/create_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"group_name":"技术讨论群","description":"讨论技术问题的群组"}' | jq .
echo ""

echo "3.2 Alice创建第二个群组..."
curl -s -X POST $SERVER_URL/api/create_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"group_name":"生活分享群","description":"分享生活趣事"}' | jq .
echo ""

echo "3.3 获取群组列表（未登录用户）..."
GROUPS_RESPONSE=$(curl -s -X GET $SERVER_URL/api/get_groups)
echo $GROUPS_RESPONSE | jq .
GROUP_ID=$(echo $GROUPS_RESPONSE | jq -r '.data[0].group_id // empty')
echo "第一个群组ID: $GROUP_ID"
echo ""

echo "3.4 获取群组列表（Alice已登录）..."
curl -s -X GET $SERVER_URL/api/get_groups \
  -H "Authorization: Bearer $ALICE_TOKEN" | jq .
echo ""
echo ""

echo "3.5 Alice加入第一个群组..."
curl -s -X POST $SERVER_URL/api/join_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.6 Bob加入第一个群组..."
curl -s -X POST $SERVER_URL/api/join_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.7 Charlie加入第一个群组..."
curl -s -X POST $SERVER_URL/api/join_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.8 测试重复加入群组（应该失败）..."
curl -s -X POST $SERVER_URL/api/join_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.9 Alice在群组中发送消息..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\",\"content\":\"大家好，我是Alice！欢迎来到技术讨论群\",\"type\":\"text\"}" | jq .
echo ""

echo "3.10 Bob在群组中发送消息..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\",\"content\":\"Hi Alice，我是Bob！很高兴加入这个群\",\"type\":\"text\"}" | jq .
echo ""

echo "3.11 Charlie在群组中发送消息..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\",\"content\":\"大家好，我是Charlie，请多指教！\",\"type\":\"text\"}" | jq .
echo ""

echo "3.12 获取群组消息历史..."
curl -s -X GET $SERVER_URL/api/get_group_messages \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.13 Charlie退出群组..."
curl -s -X POST $SERVER_URL/api/leave_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.14 Charlie退出群组后尝试获取群组消息（应该失败）..."
curl -s -X GET $SERVER_URL/api/get_group_messages \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

echo "3.15 Charlie退出群组后尝试发送群组消息（应该失败）..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\",\"content\":\"我还能发消息吗？\",\"type\":\"text\"}" | jq .
echo ""

# Charlie重新加入群组以便后续测试
echo "3.16 Charlie重新加入群组..."
curl -s -X POST $SERVER_URL/api/join_group \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"group_id\":\"$GROUP_ID\"}" | jq .
echo ""

# ========================================
# BBS论坛API测试
# ========================================
echo "=========================================="
echo "4. BBS论坛API测试"
echo "=========================================="

echo "4.1 Alice创建帖子..."
curl -s -X POST $SERVER_URL/api/create_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"title":"欢迎使用Talkbox","content":"这是第一个测试帖子，欢迎大家使用Talkbox聊天系统！这个系统支持私聊、群聊、论坛讨论和文件分享等功能。"}' | jq .
echo ""

echo "4.2 Bob创建帖子..."
curl -s -X POST $SERVER_URL/api/create_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d '{"title":"技术讨论","content":"有什么好的技术文章推荐吗？特别是关于C++和网络编程方面的资料。"}' | jq .
echo ""

echo "4.3 Charlie创建帖子..."
curl -s -X POST $SERVER_URL/api/create_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d '{"title":"新手求助","content":"刚开始学习编程，有什么好的学习路径推荐吗？"}' | jq .
echo ""

echo "4.4 获取帖子列表..."
POSTS_RESPONSE=$(curl -s -X GET $SERVER_URL/api/get_posts)
echo $POSTS_RESPONSE | jq .
POST_ID=$(echo $POSTS_RESPONSE | jq -r '.data[0].post_id // empty')
echo "第一个帖子ID: $POST_ID"
echo ""

echo "4.5 Alice回复第一个帖子..."
curl -s -X POST $SERVER_URL/api/reply_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d "{\"post_id\":\"$POST_ID\",\"content\":\"感谢大家的支持！希望大家能多多交流。\"}" | jq .
echo ""

echo "4.6 Bob回复第一个帖子..."
curl -s -X POST $SERVER_URL/api/reply_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d "{\"post_id\":\"$POST_ID\",\"content\":\"这个系统很不错！界面简洁，功能齐全。\"}" | jq .
echo ""

echo "4.7 Charlie回复第一个帖子..."
curl -s -X POST $SERVER_URL/api/reply_post \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d "{\"post_id\":\"$POST_ID\",\"content\":\"同意楼上，期待更多功能！\"}" | jq .
echo ""

echo "4.8 获取第一个帖子的回复列表..."
curl -s -X GET $SERVER_URL/api/get_post_replies \
  -H "Content-Type: application/json" \
  -d "{\"post_id\":\"$POST_ID\"}" | jq .
echo ""

echo "4.9 测试未登录用户创建帖子（应该失败）..."
curl -s -X POST $SERVER_URL/api/create_post \
  -H "Content-Type: application/json" \
  -d '{"title":"未登录帖子","content":"这个帖子不应该创建成功"}' | jq .
echo ""

# ========================================
# 文件管理API测试
# ========================================
echo "=========================================="
echo "5. 文件管理API测试"
echo "=========================================="

echo "5.1 Alice上传文件..."
curl -s -X POST $SERVER_URL/api/upload_file \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $ALICE_TOKEN" \
  -d '{"filename":"alice_document.txt","data":"这是Alice上传的文档内容，包含了一些重要的项目信息和技术文档。"}' | jq .
echo ""

echo "5.2 Bob上传文件..."
curl -s -X POST $SERVER_URL/api/upload_file \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $BOB_TOKEN" \
  -d '{"filename":"bob_notes.txt","data":"Bob的学习笔记：\n1. C++基础语法\n2. 网络编程\n3. 数据库操作"}' | jq .
echo ""

echo "5.3 Charlie上传文件..."
curl -s -X POST $SERVER_URL/api/upload_file \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d '{"filename":"charlie_code.txt","data":"// Charlie的代码示例\n#include <iostream>\nint main() {\n    std::cout << \"Hello World!\" << std::endl;\n    return 0;\n}"}' | jq .
echo ""

echo "5.4 下载Alice上传的文件..."
curl -s -X GET "$SERVER_URL/api/download_file?filename=alice_document.txt" | jq .
echo ""

echo "5.5 下载Bob上传的文件..."
curl -s -X GET "$SERVER_URL/api/download_file?filename=bob_notes.txt" | jq .
echo ""

echo "5.6 下载不存在的文件（应该失败）..."
curl -s -X GET "$SERVER_URL/api/download_file?filename=nonexistent.txt" | jq .
echo ""

echo "5.7 测试未登录用户上传文件（应该失败）..."
curl -s -X POST $SERVER_URL/api/upload_file \
  -H "Content-Type: application/json" \
  -d '{"filename":"unauthorized.txt","data":"未授权文件"}' | jq .
echo ""

# ========================================
# 用户登出测试
# ========================================
echo "=========================================="
echo "6. 用户登出测试"
echo "=========================================="

echo "6.1 Charlie用户登出..."
curl -s -X POST $SERVER_URL/api/logout \
  -H "Authorization: Bearer $CHARLIE_TOKEN" | jq .
echo ""

echo "6.2 已登出用户尝试发送消息（应该失败）..."
curl -s -X POST $SERVER_URL/api/send_message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $CHARLIE_TOKEN" \
  -d '{"receiver_id":"1","content":"我已经登出了","type":"text"}' | jq .
echo ""

echo "6.3 Alice用户登出..."
curl -s -X POST $SERVER_URL/api/logout \
  -H "Authorization: Bearer $ALICE_TOKEN" | jq .
echo ""

echo "6.4 Bob用户登出..."
curl -s -X POST $SERVER_URL/api/logout \
  -H "Authorization: Bearer $BOB_TOKEN" | jq .
echo ""

# ========================================
# 错误处理测试
# ========================================
echo "=========================================="
echo "7. 错误处理测试"
echo "=========================================="

echo "7.1 测试无效的API路径..."
curl -s -X POST $SERVER_URL/api/invalid_endpoint \
  -H "Content-Type: application/json" \
  -d '{}' | jq .
echo ""

echo "7.2 测试不支持的HTTP方法..."
curl -s -X DELETE $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{}' | jq .
echo ""

echo "7.3 测试缺少必要参数的注册..."
curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":""}' | jq .
echo ""

echo "7.4 测试空请求体..."
curl -s -X POST $SERVER_URL/api/register \
  -H "Content-Type: application/json" \
  -d '{}' | jq .
echo ""

echo "7.5 测试无效token..."
curl -s -X GET $SERVER_URL/api/get_messages \
  -H "Authorization: Bearer invalid_token_here" | jq .
echo ""

echo "7.6 测试获取不存在帖子的回复..."
curl -s -X GET $SERVER_URL/api/get_post_replies \
  -H "Content-Type: application/json" \
  -d '{"post_id":"999"}' | jq .
echo ""

# ========================================
# 测试总结
# ========================================
echo "=========================================="
echo "8. 测试总结"
echo "=========================================="
echo "✅ 用户管理API测试完成"
echo "✅ 即时通讯API测试完成（包括联系人列表）"
echo "✅ 群组管理API测试完成"
echo "✅ BBS论坛API测试完成（包括帖子回复）"
echo "✅ 文件管理API测试完成"
echo "✅ 用户登出测试完成"
echo "✅ 错误处理测试完成"
echo ""
echo "所有API接口测试完成！"
echo "如果看到任何错误，请检查服务器状态和网络连接。"
echo "=========================================="
