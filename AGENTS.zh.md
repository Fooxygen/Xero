# AGENTS.zh.md

> 本文是 `AGENTS.md` 的中文译本，内容以英文版为准。

## 项目概览

Xero 是一门静态类型编程语言。本仓库包含其编译器工具链。

### 架构

| 模块 | 职责 |
| - | - |
| Lexer | 读取源码并生成 `Token` 流 |
| Parser | 消费 Token 并构建 `Ast` |
| Sema | 遍历 AST，完成符号、类型与编译期检查 |
| Xcompiler | 生成 IR，再经 LLVM 下降、优化与链接 |

### 构建

| 类别 | 技术 | 版本 |
| - | - | - |
| 开发语言 | C++ | 23 |
| 构建系统 | CMake | 3.21+ |
| 构建工具 | Ninja | 1.13.2 |
| 工具链 | MinGW-w64 | 16.1.0 |

### 依赖

| 模块 | 用途 | 版本 |
| - | - | - |
| LLVM | IR 优化、目标代码生成 | 22.1.8 |
| toml++ | 项目配置解析 | 3.4.0 |

Xero 源文件使用 `.xe` 扩展名。项目由单个 `xero.project.toml` 配置，编译器调用方式为 `xero.exe <xero.project.toml>`。

语言规范、逐类型参考与指南位于 [Wiki](https://github.com/Fooxygen/Xero/wiki)。本仓库包含实现、`example/` 示例项目，以及 `editors/` 下的编辑器支持。

## 构建与测试命令

以下命令在 `src/` 下执行。

### 准备

将 `src/CMakeUserPresets.json.example` 复制为 `src/CMakeUserPresets.json`，填入本机的 `CMAKE_CXX_COMPILER` 与 `LLVM_DIR`。该文件不纳入版本控制。

### 配置与构建

```sh
cmake --preset mingw-debug-x64      # 或：mingw-release-x64
cmake --build --preset mingw-debug-x64
```

编译器产物位于 `build-{profile}/bin/xero.exe`——Debug 预设对应 `build-debug`，Release 预设对应 `build-release`。

### 运行编译器

```sh
xero.exe <path-to-xero.project.toml>
```

示例：

```sh
.\build-debug\bin\xero.exe .\example\xero.project.toml
```

### 运行编译产物

```sh
.\example\build\debug\example.exe
```

`xero.project.toml` 中的 `profile` 决定优化级别与产物目录，与使用哪个 Xero 预设编译无关。

### 测试

暂无自动化测试套件。通过构建编译器并编译 `example/` 来验证改动，参见测试说明。

## 代码风格

仓库未提供格式化或静态检查配置（无 `.clang-format`、`.clang-tidy`、`.editorconfig`）。请对齐周边代码。

### C++

- 语言标准：C++23。
- 每个源文件以许可证头开头，并使用 `#pragma once`。
- 命名空间小写（`lexer`、`parser`、`sema`、`xcompiler`），其内容缩进一级。
- 缩进 4 空格，左花括号置于同行。
- 类型与类使用 PascalCase 名词（`Lexer`、`AstNode`、`BasicType`）。
- 函数与方法使用 PascalCase，且读作**名词 + 动词**：主语在前、动作在后——`FileRead`、`WhitespaceSkip`、`CastRecompute`、`SignLookup`——而非常见的动词在前写法 `readFile`。
- 局部变量与参数使用 snake_case（`move_positions`、`expected`）。
- 成员变量使用 snake_case 并带尾下划线（`code_`、`tokens_`、`pos_`）。
- 编译期辅助函数优先使用 `constexpr`。

```cpp
namespace lexer {
    class Lexer {
    private:
        std::string_view code_;
        size_t           pos_ = 0;

        static constexpr bool IsAlpha(char c) {
            return (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z');
        }

    public:
        std::vector<Token>& tokens() { return tokens_; }

        Token TokenScanWord();
    };
}
```

### Xero

`.xe` 源码请遵循项目的 [Naming-Specification](https://github.com/Fooxygen/Xero/wiki/Naming-Specification)。

## 测试说明

暂无自动化测试套件。手动验证改动：

1. 构建编译器（参见构建与测试命令）。
2. 编译 `example/` 项目，确认流水线无错误完成：

   ```sh
   .\build-debug\bin\xero.exe .\example\xero.project.toml
   ```

3. 运行产物并确认输出：

   ```sh
   .\example\build\debug\example.exe
   ```

   示例程序会打印一个由 `O` 组成的菱形。

4. 修改 Lexer、Parser 或 Sema 时，在 `example/xero.project.toml` 中开启诊断以查看中间输出：

   - `diag.print_tokens = true` 输出 Token 流。
   - `diag.print_ast = true` 输出 AST。

各阶段通过 Log 模块报告完成；抛出的 `LogErr` 表示该阶段失败。

新增或修改语言特性时，扩展 `example/main.xe` 以覆盖新行为。注意 CI 不运行测试：唯一的 workflow（`release-windows.yml`）仅在推送标签时构建并打包发布产物。

## 安全注意事项

- 不提交机密或本机路径。`src/CMakeUserPresets.json` 保存本机的 `CMAKE_CXX_COMPILER` 与 `LLVM_DIR`，已被 gitignore，不得进入仓库。
- 将 `.xe` 源码与 `xero.project.toml` 视为不可信输入。Lexer、Parser、Sema 遇到非法输入必须抛出 `LogErr`，绝不崩溃、越界访问或触发未定义行为。
- 使用前校验每个配置字段，如 `ProjectConfigLoad` 对 `name`、`entry`、`profile`、`build.path` 所做的那样；拒绝空值或缺失值。
- 不要手改 `deps/vendor/` 下的 vendored 依赖。通过既定的抓取/版本流程更新，以保持版本锁定。
- 不要分发本地构建的二进制。发布产物由 CI（`release-windows.yml`）在受控工具链与静态链接下构建打包。
- 源码、示例与 Wiki 中不得出现凭据与私密数据。

## 项目结构

```
Xero/
├── src/                    # 编译器源码
│   ├── common/             # 共享定义与工具
│   │   ├── defs/           # ast、opertype、signal、token
│   │   ├── utils/          # format、loc、utf8
│   │   ├── config.hpp
│   │   └── log.hpp         # 各阶段共用的 log / logerr
│   ├── lexer/              # 词法分析，生成 token
│   ├── parser/             # 语法分析，构建 ast
│   ├── sema/               # 语义分析
│   │   ├── defs/           # fn、type、var（语义层）
│   │   ├── analyzer.*
│   │   ├── builtin.*
│   │   └── sema.hpp
│   ├── xcompiler/          # 基于 llvm 的代码生成
│   │   ├── backend/        # ir 输出、目标代码、链接
│   │   ├── defs/           # fn、type、var（实现层）
│   │   ├── ir/             # ir 生成
│   │   ├── optimizer/      # ir 优化
│   │   ├── builtin.*
│   │   └── xcompiler.hpp
│   ├── build.hpp           # 构建/版本信息
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── CMakeUserPresets.json.example
│   └── xero.cpp            # 入口
├── deps/
│   ├── fetch/              # 依赖抓取流程
│   └── vendor/             # vendored 依赖
├── example/                # 示例项目
├── editors/xero-vscode-ext/# vs code 语法高亮
├── docs/images/            # 品牌资源
├── .github/workflows/      # ci
└── .vscode/                # 任务、启动、设置
```

生成物且已被 gitignore：`build-debug/`、`build-release/`、`example/build/`、`src/CMakeUserPresets.json`。

## Git 工作流

- 默认分支为 `main`。
- 版本标签参见 Wiki 的 Home。
- 推送标签会触发 `release-windows.yml`：在 Windows 上构建，打包 `xero-windows-x64.zip` 与 `.tar.gz`，并创建**草稿** GitHub Release。可手动运行该工作流（`workflow_dispatch`）在不打标签的情况下构建。
- 每个版本需同步更新 Release-Notes 页面与草稿 Release 正文。

## 边界

### Always do

- 遵循周边 C++ 风格，保持四个阶段分离（Lexer、Parser、Sema、Xcompiler）。
- 通过 `LogErr` 报告错误；任何阶段都不得因畸形输入崩溃。
- 源文件保留许可证头与 `#pragma once`。
- 收尾前构建编译器并编译 `example/`。

### Ask first

- 改动语言的语法、语义或类型系统。
- 新增或更新依赖，或修改 CMake 文件。
- 修改 CI（`.github/workflows/`）或发布流程。
- 改动 `deps/` 下的任何内容。

### Never do

- 提交 `src/CMakeUserPresets.json`、机密或本机路径。
- 手改 `deps/vendor/` 下的 vendored 依赖。
- 提交构建产物（`build-debug/`、`build-release/`、`example/build/`）。
- 让 `example/` 处于无法编译的状态。
