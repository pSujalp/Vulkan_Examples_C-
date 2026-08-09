# Vulkan Tutorial

A cross-platform Vulkan renderer built with **CMake** supporting **Windows**, **Linux**, and **macOS**.

## Features

- Cross-platform (Windows, Linux, macOS)
- C++23
- Vulkan API
- GLFW
- GLM
- Automatic shader compilation via `glslangValidator`
- Asset and shader copying through the provided Makefile

---

# Prerequisites

Before building, ensure the following are installed:

- CMake **3.20+**
- A C++23 compatible compiler
- Vulkan SDK
- GLFW
- GLM
- glslang (required for shader compilation)

---

# Quick Start

After installing the required dependencies for your platform:

```bash
make clean
make
make run
```

or simply

```bash
make clean && make && make run
```

The Makefile will automatically:

- Configure CMake
- Build the project
- Compile all shaders to SPIR-V
- Copy assets into the output directory
- Launch the application

---

# Installation

## Windows

### 1. Install Visual Studio

Install **Visual Studio 2022** with:

- Desktop development with C++
- C++ CMake tools

---

### 2. Install the Vulkan SDK

Download the latest SDK from:

https://vulkan.lunarg.com/sdk/home

Verify the installation:

```powershell
vulkaninfo
```

---

### 3. Install vcpkg (Recommended)

Clone vcpkg:

```powershell
git clone https://github.com/microsoft/vcpkg.git

cd vcpkg

./bootstrap-vcpkg.bat.exe
```

Install the required libraries:

```powershell
./vcpkg.exe install glfw3
./vcpkg.exe install glm
./vcpkg.exe install sdl2:x64-windows
```

---

### 4. Configure

```powershell
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Replace the toolchain path with the location of your own vcpkg installation.

---

### 5. Build

```powershell
cmake --build build --config Release
```

---

### 6. Run

```powershell
build\bin\Release\Vulkan_tutorial.exe
```

---

# Linux (Ubuntu / Debian)

## Install Dependencies

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    clang \
    gdb \
    libvulkan-dev \
    vulkan-tools \
    libglfw3-dev \
    libglm-dev \
    glslang-tools
```

Verify Vulkan:

```bash
vulkaninfo
```

---

## Configure

```bash
cmake -B build -S .
```

---

## Build

```bash
cmake --build build
```

---

## Run

```bash
./build/bin/Vulkan_tutorial
```

---

# macOS

> **Note**
>
> Apple does **not** provide native Vulkan support.
> Vulkan applications run through **MoltenVK**, which translates Vulkan calls to Apple's Metal API.

---

## 1. Install Xcode Command Line Tools

```bash
xcode-select --install
```

---

## 2. Install Homebrew

https://brew.sh

---

## 3. Install Dependencies

```bash
brew install glfw glm glslang
```

---

## 4. Install the Vulkan SDK

Download the latest SDK:

https://vulkan.lunarg.com/sdk/home

Install the macOS `.dmg` package.

Verify the installation:

```bash
vulkaninfo
```

If `vulkaninfo` cannot be found:

```bash
source ~/.zshrc
```

If necessary, manually configure the SDK:

```bash
export VULKAN_SDK=$HOME/VulkanSDK/<version>/macOS
export PATH=$VULKAN_SDK/bin:$PATH
```

---

## 5. Configure

```bash
cmake -B build -S .
```

---

## 6. Build

```bash
cmake --build build
```

---

## 7. Run

```bash
./build/bin/Vulkan_tutorial
```

---

# Project Structure

```text
Vulkan_tutorial
│
├── assets/              # Models, textures, fonts, etc.
├── build/               # Generated build files
├── external/            # Third-party libraries
├── include/             # Header files
├── lib/                 # Additional libraries
├── shaders/             # GLSL shaders
│
├── src/                 # Source files
│
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

# Common Commands

## Configure

```bash
cmake -B build -S .
```

---

## Build

```bash
cmake --build build
```

---

## Run

```bash
make run
```

---

## Clean

```bash
make clean
```

---

## Delete Build Directory

Linux/macOS

```bash
rm -rf build
```

Windows PowerShell

```powershell
Remove-Item build -Recurse -Force
```

---

# Common Issues

## Could NOT find Vulkan

Ensure the Vulkan SDK is correctly installed.

Verify:

```bash
vulkaninfo
```

If this command fails, reinstall the SDK or ensure its `bin` directory is present in your system `PATH`.

---

## GLFW Not Found

Install GLFW using your platform's package manager.

Examples:

- Windows: `vcpkg install glfw3`
- Ubuntu: `sudo apt install libglfw3-dev`
- macOS: `brew install glfw`

---

## GLM Not Found

Install GLM:

- Windows: `vcpkg install glm`
- Ubuntu: `sudo apt install libglm-dev`
- macOS: `brew install glm`

---

## glslangValidator Not Found

Install glslang:

Ubuntu:

```bash
sudo apt install glslang-tools
```

macOS:

```bash
brew install glslang
```

Verify:

```bash
glslangValidator --version
```

---

## No SOURCES Given to Target

Ensure your project contains a valid source file such as:

```text
src/main.cpp
```

or update `CMakeLists.txt` to include all required source files.

---

## macOS: No Physical Devices Found

This typically indicates that MoltenVK is unavailable.

Ensure:

- The Vulkan SDK is installed
- MoltenVK is included (bundled with the SDK)
- The Vulkan SDK environment variables are correctly configured

---

# Recommended IDEs

- Visual Studio 2022
- Visual Studio Code
- CLion
- Xcode (macOS)

---

# Useful Resources

| Resource | Link |
|----------|------|
| Vulkan Tutorial | https://vulkan-tutorial.com |
| Vulkan SDK | https://vulkan.lunarg.com/sdk/home |
| Vulkan Samples | https://github.com/KhronosGroup/Vulkan-Samples |
| MoltenVK | https://github.com/KhronosGroup/MoltenVK |
| GLFW | https://www.glfw.org |
| GLM | https://github.com/g-truc/glm |

---

# License

This project is intended for educational purposes and follows the Vulkan Tutorial while incorporating additional features and improvements.
