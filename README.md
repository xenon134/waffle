# waffle
Lightweight Qt GUI application that displays "Hello World".

## Requirements

- CMake 3.16+
- Qt 5 or Qt 6 (Widgets module)
- C++17-capable compiler

## Installing Qt

### Option A — vcpkg (recommended if already installed)

```sh
vcpkg install qtbase:x64-windows
```

Then pass the toolchain file to CMake (see Build section below).

### Option B — Qt Online Installer

Download and install from <https://www.qt.io/download>. During installation select
the **MSVC 2019 64-bit** (or later) pre-built component.

## Build

### With vcpkg toolchain

```sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### With a manual Qt installation (Qt Online Installer)

```sh
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\msvc2022_64"
cmake --build build
```

### Running

```sh
.\build\Debug\waffle.exe
```
