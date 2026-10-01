
<a id="readme-top"></a>

简体中文 |
[English](./README.en.md)

<div align="center">

  <img src="https://cdn.jsdelivr.net/gh/Fooxygen/Xero@main/docs/images/brand_xero.svg" style="margin=0">
  <h3 align="center">Xero</h3>
  <p align="center">
    静态类型编程语言及其编译器
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

## 关于仓库

Xero 是一门静态类型编程语言。仓库主要包含其编译器，你可以：

- 在 `example/` 中浏览示例项目；
- 在 [Wiki](https://github.com/Fooxygen/Xero/wiki) 中获取更多内容来了解 Xero；

要执行程序，请参阅下文的 **编译与运行**。

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 架构

| 模块 | 任务 |
| - | - |
| Lexer | 词法分析 |
| Parser | 语法分析 |
| Sema | 语义分析 |
| Xcompiler | 代码生成：IR 生成、目标文件链接、可执行文件生成 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 构建

| 类别 | 技术 | 版本 |
| - | - | - |
| 开发语言 | C++ | 23 |
| 构建系统 | CMake | 3.21+ |
| 构建工具 | Ninja | 1.13.2 |
| 构建工具链 | MinGW-w64 | 16.1.0 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 依赖

| 模块 | 任务 | 版本 |
| - | - | - |
| LLVM | IR 优化、目标代码生成 | 22.1.8 |
| toml++ | 配置文件解析 | 3.4.0 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 编译与运行

> 暂只提供 **Visual Studio Code** 环境的参考步骤。

### 准备

复制 `src/CMakeUserPresets.json.example` 为 `src/CMakeUserPresets.json`，填入本机的 `CMAKE_CXX_COMPILER` 与 `LLVM_DIR`。

### 编译

分为 **Debug** 和 **Release** 预设。

- **Debug**：可执行程序位于 `build-debug/bin/xero.exe`；
- **Release**：可执行程序位于 `build-release/bin/xero.exe`；

### 运行

使用示例项目 `example/` 展示编译 Xero 项目的方式与产物，结构如下。

- `main.xe`：源代码文件；
- `xero.project.toml`：项目配置文件；

#### 编译示例项目

使用 `tasks.json` 中的任务：
- **Xero Debug**：使用 **Debug** 预设的 Xero 编译示例项目；
- **Xero Release**：使用 **Release** 预设的 Xero 编译示例项目；

#### 运行示例产物

使用 `tasks.json` 中的任务：
- **Example Debug**：快捷启动 `example/build/debug/example.exe`；
- **Example Release**：快捷启动 `example/build/release/example.exe`；

> [!WARNING]
> 项目配置文件里的 `profile` 决定优化级别与产物目录，与用哪个 Xero 预设编译无关。

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 许可证

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
