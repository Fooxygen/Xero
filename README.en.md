
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

Debug and Release use different CMake configurations, outputting to `build-debug/` and `build-release/` respectively.

The following tasks all use `example/main.xe` as the source file and automatically launch the generated program once compilation succeeds. You can refer to these tasks and write your own.

### `Debug Xero`:
```json
{
    "label": "Debug Xero",
    "type": "shell",
    "command": [
        "${workspaceFolder}/build-debug/bin/xero.exe",
        "${workspaceFolder}/example/main.xe",
        "&&",
        "${workspaceFolder}/build-debug/bin/build/main/main.exe"
    ],
    "options": {
        "cwd": "${workspaceFolder}/build-debug/bin",
        "shell": {
            "executable": "cmd.exe",
            "args": ["/d", "/c"]
        }
    }
}
```

### `Debug Xero (Token and Ast)`:

Use this when the token stream and the abstract syntax tree need to be printed.

```json
{
    "label": "Debug Xero (Token and Ast)",
    "type": "shell",
    "command": [
        "${workspaceFolder}/build-debug/bin/xero.exe",
        "${workspaceFolder}/example/main.xe",
        "--tok",
        "--ast",
        "&&",
        "${workspaceFolder}/build-debug/bin/build/main/main.exe"
    ],
    "options": {
        "cwd": "${workspaceFolder}/build-debug/bin",
        "shell": {
            "executable": "cmd.exe",
            "args": ["/d", "/c"]
        }
    }
}
```

The Release build corresponds to the `Release Xero` and `Release Xero (Token and Ast)` tasks.

### Output

The executable is located at `build-release/bin/xero.exe`.

The compiled program is output to `build-<config>/bin/build/main/main.exe`.

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## License

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
