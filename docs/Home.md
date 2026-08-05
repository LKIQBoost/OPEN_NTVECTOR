# NTVECTOR_Platform Wiki

跨平台 **Minecraft 基岩版客户端平台**（Vector）的文档中心。

## 快速开始

| 文档 | 说明 |
|------|------|
| [构建项目文档](./构建项目文档.md) | 从源码构建 Windows / Linux / Android 版本，环境要求、交叉编译、故障排查 |
| [插件开发文档](./插件开发文档.md) | 使用 Python 2.7 开发脚本插件：结构、API、事件系统、热重载、MCP 打包 |

## 项目一览

- **语言 / 构建**：C++17 · CMake 3.20+
- **目标平台**：Windows (MSVC)、Linux x86_64、Android ARM64 (Termux)
- **依赖**：全部随仓库源码编译，无需外部包管理器、无需联网
- **运行时**：内嵌 Python 2.7 脚本引擎（`source/`），插件目录 `scripts/`

## 配置文件

| 文件 | 作用 |
|------|------|
| `mc.cfg` | 启动配置（JSON）：服务器、账号、皮肤、版本信息 |
| `client_cfg.json` | 客户端配置（JSON）：`no_launch_mcp` 等 |
| `skin_data.json` | 皮肤数据 |

## 其他

- 项目主页与详细依赖表见仓库 [README](../README.md)
