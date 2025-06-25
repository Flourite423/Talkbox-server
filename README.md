# Talkbox-server

Talkbox-server 是一个基于C++开发的Linux平台聊天软件后端，提供即时通讯、BBS论坛和群组管理功能。项目使用HTTP协议进行通信，数据存储在SQLite数据库中。

## 功能特点

- **用户管理**: 用户注册、登录、登出等功能
- **即时通讯**: 支持私聊和群聊
- **BBS论坛**: 发布帖子、回复帖子、查看帖子列表
- **群组管理**: 创建群组、加入/退出群组、获取群组消息
- **文件管理**: 上传和下载文件

## 技术栈

- **语言**: C++17
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
│   ├── database.cpp   # 数据库操作实现
│   ├── database.h     # 数据库操作接口
│   ├── main.cpp       # 主程序入口
│   ├── server.cpp     # 服务器实现
│   └── server.h       # 服务器接口
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