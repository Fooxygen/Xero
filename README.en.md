
<a id="readme-top"></a>

English |
[简体中文](./README.md)

<div align="center">

  <img src="https://cdn.jsdelivr.net/gh/Fooxygen/Xero@main/docs/images/brand_xero.png" style="margin=0">
  <h3 align="center">Xero</h3>
  <p align="center">
    Statically typed programming language & toolchain
    <br /><br />
    <a href="https://github.com/Fooxygen/Xero">
      <strong>» Read Wiki</strong>
    </a>
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=bug&template=bug-report---.md">
      <strong>» Report Bug</strong>
    </a>
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=enhancement&template=feature-request---.md">
      <strong>» Request Feature</strong>
    </a>
  </p>
</div>

## About

Xero is a statically typed programming language. You can find some sample source code at `example/main.xe`.

The repository includes the language specification and its toolchain.

To execute a program, refer to **Build & Run** below.

You can find more information about Xero on the [Wiki](https://github.com/Fooxygen/Xero/wiki).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build Stack

| Category       | Technology | Version |
| -              | -          | -       |
| Language       | C++        | 23      |
| Build System   | CMake      | 3.21+   |
| Build Tools    | Ninja      | 1.13.2   |
| Xcompiler       | MinGW-w64  | 16.1.0  |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Architecture

| Module | Task |
| - | - |
| Lexer | Lexical analysis |
| Parser | Syntax analysis |
| Sema | Semantic analysis |
| Xcompiler | Compiler: IR generation, object file linking, executable generation |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Framework

| Module | Task | Version |
| - | - | - |
| LLVM | IR optimization, object file generation | 22.1.8 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build & Run

> The following steps assume a Visual Studio Code environment.

### Build

Debug and Release use different CMake configurations:

- **Debug**: outputs to `build-debug/`;
- **Release**: outputs to `build-release/`;

Use the `CMake: Build` task to compile.

Use the `CMake: Clean Rebuild` task to clean and rebuild.

### Run

Use the `Run Xero` task to run the Release build:
```json
{
    "label": "Run Xero",
    "type": "shell",
    "command": "${workspaceFolder}/build-release/bin/xero.exe",
    "options": { "cwd": "${workspaceFolder}/build-release/bin" },
    "args": ["${workspaceFolder}/example/main.xe", "-cp"]
}
```

Use the `Run Xero with options` task to print the token stream and the abstract syntax tree:
```json
{
    "label": "Run Xero with options",
    "type": "shell",
    "command": "${workspaceFolder}/build-release/bin/xero.exe",
    "options": { "cwd": "${workspaceFolder}/build-release/bin" },
    "args": ["${workspaceFolder}/example/main.xe", "--ast", "--tok", "-cp"]
}
```

The Debug build corresponds to the `Debug Xero` and `Debug Xero with options` tasks; the executable is located at `build-debug/bin/xero.exe`.

The compiled program is output to `build-release/bin/xero.exe` (Release) or `build-debug/bin/xero.exe` (Debug).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## License

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
