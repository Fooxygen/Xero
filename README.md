
<a id="readme-top"></a>

简体中文 |
[English](./README.en.md)

<div align="center">

  <img src="https://cdn.jsdelivr.net/gh/Fooxygen/Xero@main/docs/images/brand_xero.png" style="margin=0">
  <h3 align="center">Xero</h3>
  <p align="center">
    静态类型编程语言及其工具链
    <br /><br />
    <a href="https://github.com/Fooxygen/Xero">
      <strong>» 阅读 Wiki</strong>
    </a>
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=bug&template=bug-report---.md">
      <strong>» 报告问题</strong>
    </a>
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=enhancement&template=feature-request---.md">
      <strong>» 请求特性</strong>
    </a>
  </p>
  
</div>

## 关于项目

Xero 是一门静态类型编程语言。你可以在 `example/main.xe` 找到部分示例源代码.

项目包含 Xero 的设计文档及其工具链。

要执行程序，请参阅下文的 **编译与启动**。

你可以在 [Wiki](https://github.com/Fooxygen/Xero/wiki) 中获取更多内容来了解 Xero。

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 构建技术

| 类别 | 技术 | 版本 |
| - | - | - |
| 开发语言 | C++ | 23 |
| 构建系统 | CMake | 3.21+ |
| 构建工具 | Ninja | 1.13.2 |
| 编译工具链 | MinGW-w64 | 16.1.0 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 架构

| 模块 | 任务 |
| - | - |
| Lexer | 词法分析 |
| Parser | 语法分析 |
| Sema | 语义分析 |
| Xcompiler | 编译器：IR 生成、目标文件链接、可执行文件生成 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 框架
| 模块 | 任务 | 版本 |
| - | - | - |
| LLVM | IR 优化、目标文件生成 | 22.1.8 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 编译与启动

> 暂只提供 Visual Studio Code 环境的参考步骤。

### 编译

Debug 与 Release 使用不同的 CMake 配置：

- **Debug**：输出到 `build-debug/`；
- **Release**：输出到 `build-release/`；

使用任务 `CMake: Build` 编译；

使用任务 `CMake: Clean Rebuild` 清理并重编译；

### 启动

Release 版本使用任务 `Run Xero` 启动；
```json
{
    "label": "Run Xero",
    "type": "shell",
    "command": "${workspaceFolder}/build-release/bin/xero.exe",
    "options": { "cwd": "${workspaceFolder}/build-release/bin" },
    "args": ["${workspaceFolder}/example/main.xe", "-cp"]
}
```

需要输出 Token 流与抽象语法树时使用 `Run Xero with options`：
```json
{
    "label": "Run Xero with options",
    "type": "shell",
    "command": "${workspaceFolder}/build-release/bin/xero.exe",
    "options": { "cwd": "${workspaceFolder}/build-release/bin" },
    "args": ["${workspaceFolder}/example/main.xe", "--ast", "--tok", "-cp"]
}
```

Debug 版本对应 `Debug Xero` 与 `Debug Xero with options` 任务，可执行文件位于 `build-debug/bin/xero.exe`。

编译后的程序输出到 `build-release/bin/xero.exe`（Release）或 `build-debug/bin/xero.exe`（Debug）。

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 许可证

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
