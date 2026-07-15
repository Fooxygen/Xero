
<a id="readme-top"></a>

English |
[简体中文](./README.md)

<div align="center">
  <h3 align="center">Xero</h3>
  <p align="center">
    The Xero Programming Language & Toolchain
    <br />
    <a href="https://github.com/Fooxygen/Xero"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    &middot;
    <a href="https://github.com/Fooxygen/Xero/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>

## About

Xero is a statically typed programming language. A sample source file `main.xe` is located at the project root.

The repository includes the language specification and its toolchain.

To execute a program, provide the path to a `.xe` source file at startup.

You can find more information about Xero on the [Wiki](https://github.com/Fooxygen/Xero/wiki).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build Stack

| Category       | Technology | Version |
| -              | -          | -       |
| Language       | C++        | 23      |
| Build System   | CMake      | 3.21+   |
| Build Tools    | Ninja      | 1.13.2   |
| Compiler       | MinGW-w64  | 16.1.0  |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build & Run

> The following steps assume a Visual Studio Code environment.

### Build

- Use the **CMake: Build** task to compile.
- Use the **CMake: Clean Rebuild** task to clean and rebuild.

### Run

Use the **Run Xero** task:

```json
{
    "label": "Run Xero",
    "type": "shell",
    "command": "./build/bin/xero.exe"
}
```

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## License

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
