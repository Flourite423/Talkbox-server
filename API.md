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

除了注册和登录接口外，其他接口都需要在请求参数中提供有效的用户名：

```json
{
    "username": "用户名",
    // ...其他参数
}
```

用户名必须是已登录的在线用户。

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

**请求参数**:
```json
{
    "username": "用户名"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": "登出成功"
}
```

### 4. 获取用户资料

**接口**: `GET /api/user/profile`

**功能**: 获取用户个人资料

**请求参数**:
```json
{
    "username": "用户名"
}
```

**响应示例**:
```json
{
    "status": "success",
    "data": {
        "user_id": 1,
        "username": "用户名"
    }
}
```

## 即时通讯 API

### 5. 发送消息

**接口**: `POST /api/send_message`

**功能**: 发送私聊或群聊消息

**请求参数**:
```json
{
    "username": "发送者用户名",
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

**请求参数**:
```json
{
    "username": "用户名"
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

**说明**: 返回系统中除自己外的所有用户，无论是否有过聊天记录

**请求参数**:
```json
{
    "username": "用户名"
}
```

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

**注意**: `online`字段当前为静态值，实际的在线状态更新逻辑待完善。

## BBS论坛 API

### 7. 创建帖子

**接口**: `POST /api/create_post`

**功能**: 发布新帖子

**请求参数**:
```json
{
    "username": "发帖用户名",
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

**请求参数**:
```json
{
    "username": "回复用户名",
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

**请求参数** (JSON请求体):
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

**功能**: 创建新的聊天群组（创建者自动加入群组）

**请求参数**:
```json
{
    "username": "创建者用户名",
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

**请求参数**:
```json
{
    "username": "用户名",
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

**请求参数**:
```json
{
    "username": "用户名",
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

**功能**: 获取群组列表

**说明**: 
- 如果提供用户名，返回该用户加入的群组列表，包含`is_member`字段
- 如果不提供用户名，返回所有群组列表，不包含`is_member`字段

**请求参数** (可选):
```json
{
    "username": "用户名"
}
```

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

**请求参数** (JSON请求体):
```json
{
    "username": "用户名",
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

**请求参数**:
```json
{
    "username": "上传者用户名",
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

### 常见错误信息

- `"无效的用户名"`: 用户名验证失败或用户未在线
- `"用户名已存在"`: 注册时用户名重复
- `"用户名或密码错误"`: 登录时凭据错误
- `"用户名和密码不能为空"`: 注册或登录时缺少必要参数
- `"群组ID不能为空"`: 群组操作时缺少群组ID
- `"您已经是该群组的成员"`: 重复加入群组
- `"您不是该群组的成员"`: 非群组成员尝试群组操作
- `"文件不存在"`: 下载不存在的文件
- `"必须提供接收者ID或群组ID"`: 发送消息时参数错误
- `"无效的API路径"`: 请求的API路径不存在
- `"不支持的HTTP方法"`: 使用了不支持的HTTP方法

## 注意事项

1. 用户必须先登录才能使用大部分功能
2. 需要认证的接口必须在请求参数中提供有效的用户名
3. 群聊需要先创建群组，然后加入群组后才能发送消息
4. 私聊消息使用 `receiver_id`，群聊消息使用 `group_id`
5. 文件上传时需要将文件内容编码为字符串形式
6. 服务器默认在 8080 端口启动，可通过命令行参数修改
7. 上传的文件存储在服务器的 `uploads/` 目录下
8. 群组创建者会自动加入群组，无需手动加入
9. 联系人列表返回所有用户（除自己），无论是否有聊天记录
10. 部分GET接口使用JSON请求体传递参数（如`get_post_replies`、`get_group_messages`）
11. 重复加入已加入的群组会返回错误信息
12. 只有群组成员才能发送群组消息和获取群组消息历史
