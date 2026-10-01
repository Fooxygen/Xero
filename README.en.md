
<a id="readme-top"></a>

English |
[简体中文](./README.md)

<div align="center">

  <img src="https://cdn.jsdelivr.net/gh/Fooxygen/Xero@main/docs/images/brand_xero.png" style="margin=0">
  <h3 align="center">Xero</h3>
  <p align="center">
    Statically typed programming language & compiler
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

## About the Repository

Xero is a statically typed programming language. The repository mainly contains its compiler. You can:

- browse the sample project in `example/`;
- find more information about Xero on the [Wiki](https://github.com/Fooxygen/Xero/wiki);

To run a program, refer to **Build & Run** below.

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Architecture

| Module | Task |
| -         | - |
| Lexer     | Lexical analysis |
| Parser    | Syntax analysis |
| Sema      | Semantic analysis |
| Xcompiler | Code generation: IR generation, object file linking, executable generation |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build

| Category        | Technology | Version |
| -               | -          | -       |
| Language        | C++        | 23      |
| Build System    | CMake      | 3.21+   |
| Build Tools     | Ninja      | 1.13.2  |
| Build Toolchain | MinGW-w64  | 16.1.0  |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Dependency

| Module | Task | Version |
| - | - | - |
| LLVM | IR Optimization, Object Code Generation | 22.1.8 |
| toml++ | Configuration File Parsing | 3.4.0 |

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## Build & Run

> The following steps assume a **Visual Studio Code** environment.

### Setup

Copy `src/CMakeUserPresets.json.example` to `src/CMakeUserPresets.json`, and fill in the machine-specific `CMAKE_CXX_COMPILER` and `LLVM_DIR`.

### Build

Available as the **Debug** and **Release** presets.

- **Debug**: the executable is located at `build-debug/bin/xero.exe`;
- **Release**: the executable is located at `build-release/bin/xero.exe`;

### Run

The sample project `example/` demonstrates how to compile a Xero project and what it produces; its structure is as follows.

- `main.xe`: source file;
- `xero.project.toml`: project configuration file;

#### Compile the Sample Project

Using the tasks in `tasks.json`:
- **Xero Debug**: compile the sample project with the **Debug** preset of Xero;
- **Xero Release**: compile the sample project with the **Release** preset of Xero;

#### Run the Sample Output

Using the tasks in `tasks.json`:
- **Example Debug**: quickly launch `example/build/debug/example.exe`;
- **Example Release**: quickly launch `example/build/release/example.exe`;

> [!WARNING]
> The `profile` in the project configuration file determines the optimization level and the output directory, independent of which Xero preset is used to compile.

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>

## License

Copyright (c) 2026 Fooxygen. Licensed under the [MIT License](LICENSE).

<p align="right"><a href="#readme-top">⭱ Back to top</a></p>
