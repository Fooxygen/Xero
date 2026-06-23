
<a id="readme-top"></a>

<div align="center">

  <!--<img src="Logo.png" width=150>-->
  <h3 align="center">Xero</h3>
  <p align="center">
    静态类型编程语言 Xero 及其工具链
    <br />
    <a href="https://github.com/Fooxygen/Xero"><strong>» 阅读文档 »</strong></a>
    <br />
    <br />
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=bug&template=bug-report---.md">报告问题</a>
    &middot;
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=enhancement&template=feature-request---.md">请求特性</a>
  </p>
  
</div>

## 关于项目

Xero 是一门静态类型编程语言。你可以在仓库根目录找到示例源代码 ``main.xe``.

项目包含 Xero 的设计文档及其工具链。

要执行程序，请在启动后输入任意 ``.xe`` 源代码文件路径。

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 已实现功能

- 词法解析器 Lexer：生成 Token 流；
- 语法解析器 Parser：生成 AST；

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 构建技术

| 类别 | 技术 | 版本 |
| - | - | - |
| 开发语言 | C++ | 23 |
| 构建系统 | CMake | 3.15+ |
| 编译工具 | MinGW-w64 | 16.1.0|

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 编译与启动

暂只提供 Visual Studio Code 环境的参考步骤。

### 编译

使用任务 ``CMake: Build`` 编译；

使用任务 ``CMake: Clean Rebuild`` 清理并重编译；

### 启动

使用任务 ``Run Xero`` 启动；
```json
{
    "label": "Run Xero",
    "type": "shell",
    "command": "./build/bin/xero.exe"
}
```

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## 许可证

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
