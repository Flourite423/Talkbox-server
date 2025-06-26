# Talkbox 后端 API 接口文档

## 概述

Talkbox 是一个基于 Linux 平台的聊天软件后端，使用 HTTP 协议进行通信，数据存储在 SQLite 数据库中。

## 基本信息

- **协议**: HTTP/1.1
- **数据格式**: JSON
- **默认端口**: 8080
- **数据库**: SQLite

## 通用响应格式

所有 API 响应都采用以下 JSON 格式：

```json
{
    "status": "success|error",
    "data": "响应数据或错误信息"
}
```

## 身份验证

除了注册和登录接口外，其他接口都需要在HTTP请求头中提供有效的token：

```
Authorization: Bearer <token>
```

token通过登录接口获取，有效期为用户在线期间。

## 用户管理 API

### 1. 用户注册

**接口**: `POST /api/register`

**功能**: 注册新用户

**请求参数**:
```json
{
    "username": "用户名",
    "password": "密码"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "注册成功"
}
```

**错误响应**:
```json
{
    "status": "error",
    "data": "用户名已存在"
}
```

### 2. 用户登录

**接口**: `POST /api/login`

**功能**: 用户登录验证

**请求参数**:
```json
{
    "username": "用户名",
    "password": "密码"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": {
        "user_id": 1,
        "username": "用户名",
        "token": "JBAEquRaO9So5RED4jBMuM78xLWdgbkk"
    }
}
```

### 3. 用户登出

**接口**: `POST /api/logout`

**功能**: 用户登出

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**: 无需参数

**响应示例**:
```json
{
    "status": "success",
    "data": "登出成功"
}
```

## 即时通讯 API

### 4. 发送消息

**接口**: `POST /api/send_message`

**功能**: 发送私聊或群聊消息

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "receiver_id": "接收者ID（私聊时使用）",
    "group_id": "群组ID（群聊时使用）",
    "content": "消息内容",
    "type": "消息类型（text/file）"
}
```

**说明**:
- 私聊时只需要提供 `receiver_id`
- 群聊时只需要提供 `group_id`
- 两者不能同时提供

**响应示例**:
```json
{
    "status": "success",
    "data": "消息发送成功"
}
```

### 5. 获取消息

**接口**: `GET /api/get_messages`

**功能**: 获取用户相关的所有消息（私聊和群聊）

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**: 无需参数

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "message_id": 1,
            "sender_id": 2,
            "receiver_id": 1,
            "group_id": -1,
            "content": "你好",
            "type": "text",
            "timestamp": "Mon Jun 25 14:30:00 2025"
        }
    ]
}
```

### 6. 获取联系人列表

**接口**: `GET /api/get_contacts`

**功能**: 获取所有可以私聊的用户列表（除了自己），包含在线状态

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**: 无需参数

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "user_id": 2,
            "username": "bob",
            "online": false
        },
        {
            "user_id": 3,
            "username": "charlie",
            "online": true
        }
    ]
}
```

## BBS论坛 API

### 7. 创建帖子

**接口**: `POST /api/create_post`

**功能**: 发布新帖子

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "title": "帖子标题",
    "content": "帖子内容"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "发帖成功"
}
```

### 8. 获取帖子列表

**接口**: `GET /api/get_posts`

**功能**: 获取所有帖子列表

**请求参数**: 无需参数

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "post_id": 1,
            "user_id": 1,
            "title": "帖子标题",
            "content": "帖子内容",
            "timestamp": "Mon Jun 25 14:30:00 2025"
        }
    ]
}
```

### 9. 回复帖子

**接口**: `POST /api/reply_post`

**功能**: 回复指定帖子

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "post_id": "帖子ID",
    "content": "回复内容"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "回帖成功"
}
```

### 10. 获取帖子回复

**接口**: `GET /api/get_post_replies`

**功能**: 获取指定帖子的所有回复

**请求参数**:
```json
{
    "post_id": "帖子ID"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "reply_id": 1,
            "post_id": 1,
            "user_id": 2,
            "content": "这是第一个回复",
            "timestamp": "Wed Jun 26 16:30:00 2025"
        },
        {
            "reply_id": 2,
            "post_id": 1,
            "user_id": 3,
            "content": "这是第二个回复",
            "timestamp": "Wed Jun 26 16:35:00 2025"
        }
    ]
}
```

## 群组管理 API

### 11. 创建群组

**接口**: `POST /api/create_group`

**功能**: 创建新的聊天群组

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "group_name": "群组名称",
    "description": "群组描述"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "群组创建成功"
}
```

### 12. 加入群组

**接口**: `POST /api/join_group`

**功能**: 加入指定群组

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "group_id": "群组ID"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "加入群组成功"
}
```

### 13. 退出群组

**接口**: `POST /api/leave_group`

**功能**: 退出指定群组

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "group_id": "群组ID"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "退出群组成功"
}
```

### 14. 获取群组列表

**接口**: `GET /api/get_groups`

**功能**: 获取所有群组列表

**请求头** (可选):
```
Authorization: Bearer <token>
```

**请求参数**: 无需参数

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "group_id": 1,
            "group_name": "技术讨论群",
            "description": "讨论技术问题的群组",
            "creator_id": 1,
            "created_time": "Mon Jun 25 14:30:00 2025",
            "is_member": true
        }
    ]
}
```

### 15. 获取群组消息

**接口**: `GET /api/get_group_messages`

**功能**: 获取指定群组的消息历史

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "group_id": "群组ID"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": [
        {
            "message_id": 1,
            "sender_id": 2,
            "group_id": 1,
            "content": "大家好！",
            "type": "text",
            "timestamp": "Mon Jun 25 14:30:00 2025"
        }
    ]
}
```

## 文件管理 API

### 16. 上传文件

**接口**: `POST /api/upload_file`

**功能**: 上传文件到服务器

**请求头**:
```
Authorization: Bearer <token>
```

**请求参数**:
```json
{
    "filename": "文件名",
    "data": "文件数据（二进制数据）"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "文件上传成功"
}
```

### 17. 下载文件

**接口**: `GET /api/download_file?filename=文件名`

**功能**: 从服务器下载文件

**请求参数**: 通过 URL 参数传递文件名

**响应示例**:
```json
{
    "status": "success",
    "data": "文件内容（二进制数据）"
}
```

## 错误码说明

- `success`: 操作成功
- `error`: 操作失败，具体错误信息在 `data` 字段中

## 注意事项

1. 用户必须先登录获取token才能使用大部分功能
2. token需要在请求头中以 `Authorization: Bearer <token>` 的格式传递
3. 群聊需要先创建群组，然后加入群组后才能发送消息
4. 私聊消息使用 `receiver_id`，群聊消息使用 `group_id`
5. 文件上传时需要将文件内容编码为字符串形式
6. 服务器默认在 8080 端口启动，可通过命令行参数修改
7. 上传的文件存储在服务器的 `uploads/` 目录下
