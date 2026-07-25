# Build Instructions

This project uses **CMake** and supports **Windows**, **Linux**, and **macOS**.

## Prerequisites

- CMake 3.20 or newer
- A C++23 compatible compiler
- Vulkan SDK
- GLFW
- GLM

---

# Windows

## Install Visual Studio

Install **Visual Studio 2022** with the following workloads:

- Desktop development with C++
- C++ CMake tools

---

## Install Vulkan SDK

Download and install the latest Vulkan SDK:

https://vulkan.lunarg.com/sdk/home

Verify installation:

```powershell
vulkaninfo
```

---

## Install vcpkg (Recommended)

Clone vcpkg:

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

Install dependencies:

```powershell
vcpkg install glfw3
vcpkg install glm
```

---

## Configure

```powershell
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Replace the toolchain path with your own vcpkg installation.

---

## Build

```powershell
cmake --build build --config Release
```

---

## Run

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
    libglm-dev
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
> Apple does **not** support native Vulkan.
>
> Vulkan applications run through **MoltenVK**, which translates Vulkan commands to Apple's Metal API.

---

## Install Xcode Command Line Tools

```bash
xcode-select --install
```

---

## Install Homebrew

https://brew.sh

---

## Install Dependencies

```bash
brew install glfw glm
```

---

## Install Vulkan SDK

Download the latest SDK from:

https://vulkan.lunarg.com/sdk/home

Install the macOS `.dmg`.

Verify:

```bash
vulkaninfo
```

If `vulkaninfo` is not found, reload your shell:

```bash
source ~/.zshrc
```

If necessary, add the SDK to your environment:

```bash
export VULKAN_SDK=$HOME/VulkanSDK/<version>/macOS
export PATH=$VULKAN_SDK/bin:$PATH
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

# Project Structure

```text
Vulkan_tutorial/
│
├── CMakeLists.txt
├── README.md
│
├── src/
├── include/
├── shaders/
├── assets/
├── external/
├── lib/
└── build/
```

---

# Common Commands

Generate build files:

```bash
cmake -B build -S .
```

Build project:

```bash
cmake --build build
```

Delete build directory:

```bash
rm -rf build
```

Windows PowerShell:

```powershell
Remove-Item build -Recurse -Force
```

---

# Common Issues

## Could NOT find Vulkan

Install the Vulkan SDK and verify with:

```bash
vulkaninfo
```

---

## No SOURCES given to target

Ensure your project contains:

```text
src/main.cpp
```

or update `CMakeLists.txt` to include your source files.

---

## GLFW not found

Install GLFW using your platform's package manager or vcpkg.

---

## GLM not found

Install GLM using your platform's package manager or vcpkg.

---

## macOS: No Physical Devices Found

Make sure:

- MoltenVK is installed (included with the Vulkan SDK).
- `VK_ENABLE_BETA_EXTENSIONS` is enabled in the CMake configuration.

---

# Recommended IDEs

- Visual Studio 2022 (Windows)
- Visual Studio Code
- CLion
- Xcode (macOS)

---

# Useful Resources

- Vulkan Tutorial: https://vulkan-tutorial.com
- Vulkan SDK: https://vulkan.lunarg.com/sdk/home
- GLFW: https://www.glfw.org
- GLM: https://github.com/g-truc/glm
- MoltenVK: https://github.com/KhronosGroup/MoltenVK
- Vulkan Samples: https://github.com/KhronosGroup/Vulkan-Samples