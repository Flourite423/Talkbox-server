# Talkbox-server

Talkbox-server 是基于C++开发的Linux平台聊天软件 Talkbox 的服务端，提供即时通讯、BBS论坛等功能。该项目采用模块化设计，各个功能相互独立，易于扩展和维护。

## 功能特点

- **用户管理**: 用户注册、登录、登出等功能
- **即时通讯**: 支持私聊和群聊
- **群组管理**: 创建群组、加入/退出群组、获取群组消息
- **BBS论坛**: 发布帖子、回复帖子、查看帖子列表
- **文件管理**: 上传和下载文件

## 技术栈

- **语言**: C++17
- **设计模式**: 模块化设计、单一职责原则
- **数据库**: SQLite3
- **网络**: HTTP协议
- **构建工具**: Make
- **依赖库**: pthread, sqlite3

## 快速开始

### 1. 编译

```bash
make
```

### 2. 运行

```bash
# 使用默认端口(8080)
./scripts/start.sh

# 或指定端口
./scripts/start.sh 9000
```

### 3. 测试

提供了完整的API测试脚本：

```bash
./scripts/test.sh
```

## 安装与卸载

安装：

```bash
sudo make install
```

卸载：

```bash
sudo make uninstall
```

## 项目结构

```
├── API.md             # API接口文档
├── LICENSE            # 许可证
├── Makefile           # 构建脚本
├── README.md          # 项目说明文档
├── build/             # 编译输出目录
├── scripts/           # 脚本目录
│   ├── start.sh       # 启动脚本
│   └── test.sh        # 测试脚本
├── src/               # 源代码目录
│   ├── common.cpp     # 通用工具函数实现
│   ├── common.h       # 通用工具函数接口
│   ├── database.cpp   # 数据库操作实现
│   ├── database.h     # 数据库操作接口
│   ├── file_manager.cpp # 文件管理模块实现
│   ├── file_manager.h   # 文件管理模块接口
│   ├── forum_service.cpp # 论坛服务模块实现
│   ├── forum_service.h   # 论坛服务模块接口
│   ├── main.cpp       # 主程序入口
│   ├── message_service.cpp # 消息服务模块实现
│   ├── message_service.h   # 消息服务模块接口
│   ├── server.cpp     # 服务器实现
│   ├── server.h       # 服务器接口
│   ├── user_manager.cpp # 用户管理模块实现
│   └── user_manager.h   # 用户管理模块接口
└── uploads/           # 文件上传目录
```

## API接口

详细的API接口文档请参考 [API.md](API.md)。

主要API包括：

- 用户管理API: 注册、登录、登出
- 即时通讯API: 发送消息、获取消息
- BBS论坛API: 创建帖子、获取帖子列表、回复帖子
- 群组管理API: 创建群组、加入/退出群组、获取群组列表/消息
- 文件管理API: 上传文件、下载文件

## 模块化设计

Talkbox-server 采用了模块化设计，将不同功能解耦为独立的模块：

1. **服务器核心模块** (server.h/cpp): 
   - 负责网络通信和请求分发
   - 不包含具体业务逻辑，仅将请求路由到相应的功能模块

2. **用户管理模块** (user_manager.h/cpp): 
   - 处理用户注册、登录、登出
   - 管理用户会话和Token验证

3. **即时通讯模块** (message_service.h/cpp): 
   - 处理私聊和群聊消息
   - 管理群组创建、加入和退出

4. **论坛服务模块** (forum_service.h/cpp): 
   - 处理帖子的创建、查询和回复

5. **文件管理模块** (file_manager.h/cpp): 
   - 处理文件的上传和下载

6. **数据库模块** (database.h/cpp): 
   - 提供数据存储和查询接口
   - 封装SQLite操作

7. **通用工具模块** (common.h/cpp): 
   - 提供共享的工具函数和数据结构
   - 包含JSON解析和HTTP响应创建等功能


## 身份验证

除了注册和登录接口外，其他接口都需要在HTTP请求头中提供有效的token：

```
Authorization: Bearer <token>
```

token通过登录接口获取，有效期为用户在线期间。

## 配置

服务器默认在8080端口启动，可以通过命令行参数修改端口号：

```bash
./build/talkbox-server <端口号>
```

## 许可证

[LICENSE](LICENSE)文件中包含详细的许可证信息。
