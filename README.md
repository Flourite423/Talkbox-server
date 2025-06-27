# Talkbox Server

一个基于 C++ 的聊天服务器，支持实时消息传递、论坛功能和文件共享。

## 目录

- [Talkbox Server](#talkbox-server)
  - [目录](#目录)
  - [技术栈](#技术栈)
  - [快速开始](#快速开始)
    - [环境要求](#环境要求)
    - [安装依赖](#安装依赖)
    - [编译运行](#编译运行)
  - [API 文档](#api-文档)
    - [主要接口概览](#主要接口概览)
    - [请求示例](#请求示例)
  - [项目结构](#项目结构)
  - [构建说明](#构建说明)
  - [许可证](#许可证)

## 技术栈

- **编程语言**: C++17
- **数据库**: SQLite3
- **网络协议**: HTTP/1.1
- **数据格式**: JSON
- **构建工具**: Make
- **线程库**: pthread

## 快速开始

### 环境要求

- Linux 操作系统
- GCC 7.0+ (支持 C++17)
- SQLite3 开发库
- pthread 库

### 安装依赖

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel sqlite
```

### 编译运行

1. **克隆项目**
```bash
git clone <repository-url>
cd Talkbox-server
```

2. **编译项目**
```bash
make
```

3. **启动服务器**
```bash
# 使用默认端口 8080
./scripts/start.sh

# 或指定端口
./scripts/start.sh 9000
```

4. **测试服务器**
```bash
# 运行测试脚本
./scripts/test.sh
```

## API 文档

详细的 API 接口文档请查看 [API.md](API.md)。

### 主要接口概览

- `POST /api/register` - 用户注册
- `POST /api/login` - 用户登录
- `POST /api/logout` - 用户登出
- `GET /api/messages` - 获取消息列表
- `POST /api/messages` - 发送消息
- `GET /api/forums` - 获取论坛帖子
- `POST /api/forums` - 发布帖子
- `POST /api/upload` - 上传文件
- `GET /api/download` - 下载文件

### 请求示例

```bash
# 用户注册
curl -X POST http://localhost:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username": "alice", "password": "123456"}'

# 用户登录
curl -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username": "alice", "password": "123456"}'
```

## 项目结构

```
Talkbox-server/
├── src/                    # 源代码目录
│   ├── main.cpp           # 程序入口
│   ├── server.cpp/h       # 服务器核心
│   ├── database.cpp/h     # 数据库操作
│   ├── user_manager.cpp/h # 用户管理
│   ├── message_service.cpp/h # 消息服务
│   ├── forum_service.cpp/h   # 论坛服务
│   ├── file_manager.cpp/h    # 文件管理
│   └── common.cpp/h       # 通用工具
├── build/                 # 编译输出目录
├── scripts/               # 脚本目录
│   ├── start.sh          # 启动脚本
│   └── test.sh           # 测试脚本
├── uploads/               # 文件上传目录
├── Makefile              # 构建配置
├── API.md                # API 文档
├── README.md             # 项目说明
└── LICENSE               # 许可证
```

## 构建说明

```bash
# 编译项目
make

# 清理构建文件
make clean

# 安装到系统 (需要 root 权限)
sudo make install

# 卸载
sudo make uninstall
```

## 许可证

详细信息请查看 [LICENSE](LICENSE) 文件。


