# AGENTS.md

## Project overview

Xero is a statically typed programming language. This repository contains its compiler toolchain.

### Architecture

| Module | Responsibility |
| - | - |
| Lexer | Reads source and produces a `Token` stream |
| Parser | Consumes tokens and builds an `Ast` |
| Sema | Walks the AST, resolving symbols, types, and compile-time checks |
| Xcompiler | Generates IR, then lowers, optimizes, and links via LLVM |

### Build

| Category | Technology | Version |
| - | - | - |
| Language | C++ | 23 |
| Build system | CMake | 3.21+ |
| Build tool | Ninja | 1.13.2 |
| Toolchain | MinGW-w64 | 16.1.0 |

### Dependencies

| Module | Purpose | Version |
| - | - | - |
| LLVM | IR optimization, target code generation | 22.1.8 |
| toml++ | Project config parsing | 3.4.0 |

Xero source files use the `.xe` extension. A project is configured by a single `xero.project.toml`, and the compiler is invoked as `xero.exe <xero.project.toml>`.

The language specification, per-type references, and guides live in the [Wiki](https://github.com/Fooxygen/Xero/wiki). This repository holds the implementation, the `example/` project, and editor support under `editors/`.

## Build and test commands

Run the following from `src/`.

### Setup

Copy `src/CMakeUserPresets.json.example` to `src/CMakeUserPresets.json`, then fill in `CMAKE_CXX_COMPILER` and `LLVM_DIR` for the local machine. This file is not checked in.

### Configure and build

```sh
cmake --preset mingw-debug-x64      # or: mingw-release-x64
cmake --build --preset mingw-debug-x64
```

The compiler binary is written to `build-{profile}/bin/xero.exe` — `build-debug` for the debug preset, `build-release` for release.

### Run the compiler

```sh
xero.exe <path-to-xero.project.toml>
```

Example:

```sh
.\build-debug\bin\xero.exe .\example\xero.project.toml
```

### Run the compiled program

```sh
.\example\build\debug\example.exe
```

The `profile` field in `xero.project.toml` selects the optimization level and output directory, independently of which Xero preset built the compiler.

### Tests

There is no automated test suite. Validate a change by building the compiler and compiling `example/`. See Testing instructions.

## Code style guidelines

No formatter or linter config is checked in (no `.clang-format`, `.clang-tidy`, or `.editorconfig`). Match the surrounding code.

### C++

- Language standard: C++23.
- Every source file starts with the license header and uses `#pragma once`.
- Namespaces are lowercase (`lexer`, `parser`, `sema`, `xcompiler`); indent their contents one level.
- Indentation is 4 spaces, with opening braces on the same line.
- Types and classes are PascalCase nouns (`Lexer`, `AstNode`, `BasicType`).
- Functions and methods are PascalCase and read **noun + verb**: the subject comes first, then the action — `FileRead`, `WhitespaceSkip`, `CastRecompute`, `SignLookup` — rather than the verb-first `readFile` style common elsewhere.
- Local variables and parameters are snake_case (`move_positions`, `expected`).
- Member variables are snake_case with a trailing underscore (`code_`, `tokens_`, `pos_`).
- Prefer `constexpr` for compile-time helpers.

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

For `.xe` source, follow the project [Naming-Specification](https://github.com/Fooxygen/Xero/wiki/Naming-Specification).

## Testing instructions

There is no automated test suite. Validate changes manually:

1. Build the compiler (see Build and test commands).
2. Compile the `example/` project and confirm the pipeline completes without errors:

   ```sh
   .\build-debug\bin\xero.exe .\example\xero.project.toml
   ```

3. Run the produced program and confirm its output:

   ```sh
   .\example\build\debug\example.exe
   ```

   The example prints a diamond made of `O` characters.

4. For Lexer, Parser, or Sema changes, turn on diagnostics in `example/xero.project.toml` to inspect intermediate output:

   - `diag.print_tokens = true` dumps the token stream.
   - `diag.print_ast = true` prints the AST.

Each stage reports completion through the Log module; a thrown `LogErr` means that stage failed.

When you add or change a language feature, extend `example/main.xe` so the new behavior is exercised. Note that CI does not run tests: the only workflow (`release-windows.yml`) builds and packages release artifacts when a tag is pushed.

## Security considerations

- Never commit secrets or machine-specific paths. `src/CMakeUserPresets.json` holds the local `CMAKE_CXX_COMPILER` and `LLVM_DIR`; it is gitignored and must stay out of the repository.
- Treat `.xe` sources and `xero.project.toml` as untrusted input. The Lexer, Parser, and Sema must reject invalid input by throwing `LogErr` — never by crashing, reading out of bounds, or invoking undefined behavior.
- Validate every config field before use, as `ProjectConfigLoad` does for `name`, `entry`, `profile`, and `build.path`; reject empty or missing values.
- Do not hand-edit vendored dependencies under `deps/vendor/`. Update them through the intended fetch/version process to keep versions pinned.
- Do not distribute locally built binaries. Release artifacts are built and packaged by CI (`release-windows.yml`), which controls the toolchain and static linkage.
- Keep credentials and private data out of sources, examples, and the Wiki.

## Project structure

```
Xero/
├── src/                    # compiler sources
│   ├── common/             # shared definitions and utilities
│   │   ├── defs/           # ast, opertype, signal, token
│   │   ├── utils/          # format, loc, utf8
│   │   ├── config.hpp
│   │   └── log.hpp         # log / logerr used across stages
│   ├── lexer/              # tokenizes source
│   ├── parser/             # builds the ast
│   ├── sema/               # semantic analysis
│   │   ├── defs/           # fn, type, var (semantic layer)
│   │   ├── analyzer.*
│   │   ├── builtin.*
│   │   └── sema.hpp
│   ├── xcompiler/          # llvm-based code generation
│   │   ├── backend/        # ir output, object code, linking
│   │   ├── defs/           # fn, type, var (implementation layer)
│   │   ├── ir/             # ir generation
│   │   ├── optimizer/      # ir optimization
│   │   ├── builtin.*
│   │   └── xcompiler.hpp
│   ├── build.hpp           # build/version info
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── CMakeUserPresets.json.example
│   └── xero.cpp            # entry
├── deps/
│   ├── fetch/              # dependency fetch process
│   └── vendor/             # vendored dependencies
├── example/                # sample project
├── editors/xero-vscode-ext/# vs code syntax highlighting
├── docs/images/            # brand assets
├── .github/workflows/      # ci
└── .vscode/                # tasks, launch, settings
```

Generated and gitignored: `build-debug/`, `build-release/`, `example/build/`, and `src/CMakeUserPresets.json`.

## Git workflow

- Default branch is `main`.
- Version tags: see the Wiki Home.
- Pushing a tag triggers `release-windows.yml`, which builds on Windows, packages `xero-windows-x64.zip` and `.tar.gz`, and opens a **draft** GitHub release. Run the workflow manually (`workflow_dispatch`) to build without tagging.
- Update the Release-Notes page and the draft release body together for each version.

## Boundaries

### Always do

- Match the surrounding C++ style and keep the four stages separate (Lexer, Parser, Sema, Xcompiler).
- Report errors through `LogErr`; never let a stage crash on malformed input.
- Keep the license header and `#pragma once` in source files.
- Build the compiler and compile `example/` before finishing.

### Ask first

- Changing language syntax, semantics, or the type system.
- Adding or updating dependencies, or editing CMake files.
- Modifying CI (`.github/workflows/`) or the release process.
- Updating anything under `deps/`.

### Never do

- Commit `src/CMakeUserPresets.json`, secrets, or machine-specific paths.
- Hand-edit vendored dependencies under `deps/vendor/`.
- Commit build output (`build-debug/`, `build-release/`, `example/build/`).
- Leave `example/` uncompilable.
