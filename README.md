# OpenGL Classroom Model

This repository contains several OpenGL example programs and helper code used for teaching and experimentation. Two notable examples are:

- `main.cpp`: a modern GLFW/GLAD core-profile demo that compiles embedded GLSL shaders (Phong & Gouraud) from string literals.

This README explains how to build and run the project on Windows using MSYS2 (MinGW-w64) and the included `Makefile`.

Prerequisites (Windows + MSYS2)
-------------------------------
- Windows 10/11
- Install MSYS2 from: https://www.msys2.org/
- After installing MSYS2, open the *MSYS2 MinGW 64-bit* shell for a 64-bit build.

Install required packages (run inside MSYS2 MinGW64 shell)
---------------------------------------------------------
These commands update the package database and install the common toolchain and libraries used by the project. Run them in the *MSYS2 MinGW 64-bit* shell (not the plain MSYS shell):

```bash
# Update package DB and core packages (may require restart of MSYS2 shell)
pacman -Syu
# After the update finishes you may need to close and re-open the MinGW64 shell.

# Install toolchain and libraries (single command)
# This list contains the common packages used by OpenGL projects. Add/remove packages depending on Build errors.
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-glew mingw-w64-x86_64-glfw mingw-w64-x86_64-assimp \
    mingw-w64-x86_64-freeglut mingw-w64-x86_64-pkg-config
```

Notes:
- If you prefer 32-bit, use the *MSYS2 MinGW 32-bit* shell and replace package names with `mingw-w64-i686-*` variants.
- The project includes some external libraries under `external/`. You may not need system packages for every library, but installing these reduces missing-header/linker issues.

Build using Makefile (MSYS2 MinGW64)
-----------------------------------
1. Open **MSYS2 MinGW 64-bit** shell (important: not the normal MSYS shell)
   - You can start it from the Start menu: "MSYS2 MinGW 64-bit"
   - Or from PowerShell (adapt path if your MSYS2 is installed elsewhere):

```powershell
# Start MSYS2 MinGW64 shell from PowerShell (example path)
C:\msys64\msys2_shell.cmd -mingw64
```

2. Inside the MinGW64 shell, change directory to the project root (where the `Makefile` is located):

```bash
cd /c/College/7th_Sem/CS-550/ogl-master/ogl-master
```

3. Build with `make`:

```bash
make
```

- The project `Makefile` should build the executable. If the `Makefile` assumes GNU/Linux tools, run `make` from the MinGW64 shell which provides typical Unix-like tools.
- If `make` fails due to missing libraries, install the missing `mingw-w64-x86_64-*` package via `pacman` and re-run `make`.

Run the program
---------------
From the same MinGW64 shell (so the runtime DLLs are on PATH), run the generated executable (the `Makefile` in this repository builds `main.exe`). Example:

```bash
./main.exe
```

If you need to run from PowerShell, ensure the MinGW64 `bin` directory is on your PATH so required DLLs (libstdc++, libgcc, GLFW/GLEW DLLs) are found:

```powershell
$env:PATH += ";C:\msys64\mingw64\bin"
# then run the exe from the project folder
C:\College\7th_Sem\CS-550\ogl-master\ogl-master\main.exe
```

Project-specific runtime notes
------------------------------
- `main.cpp` runtime notes (this is the default example built by the `Makefile`):
  - Shading modes: press `1` for Phong (per-fragment) and `2` for Gouraud (per-vertex).
  - Shadow mapping: `main.cpp` does not perform a shadow-pass — shadows are implemented only in `CLASSROOM.cpp`.
  - The program uses `tinyobj` for OBJ loading and expects materials/textures referenced by the OBJ to be present under their original paths (check the `assets/` folder). If textures are missing, the program falls back to material colors.

Shader file paths (important)
-----------------------------
- `main.cpp` embeds GLSL shader sources directly in the C++ source as string literals. You do not need to supply external shader files for `main.cpp`.
- `CLASSROOM.cpp` is the example that loads shader files from disk and implements shadow mapping; for that example you must provide the shader files (or edit its `basePath` to a relative `shaders/` folder containing the shader files).

If you are unsure which example you are running, prefer building/running `main.cpp` first — it is self-contained and easier to get working.

Shadow mapping
--------------
- The code includes a two-pass shadow mapping approach (depth pass into a `depthMapFBO` and a lit pass using the resulting shadow texture). Enable shadows with `M`.
- Ensure the shader versions used in the project support sampler2D/shadow sampling compatible with your GL context.

Troubleshooting
---------------
- "missing header" / "cannot find -l..." errors during compilation:
  - Install or enable the appropriate MSYS2 packages listed above.
  - Verify you are using the MinGW64 shell (64-bit) if building for 64-bit.
- Shader compilation failure:
  - The program prints GLSL compilation logs (vertex/fragment shader logs). Inspect those messages and ensure your GPU/drivers support the used GLSL version.
- Runtime errors like "cannot load texture" or missing textures:
  - The app prints texture loading messages. Ensure texture files in OBJ MTL references are present and `basePath` is set correctly.
- Linking errors referencing external libs:
  - Confirm packages (GLFW, GLEW, ASSIMP, etc.) were installed with `pacman` and that the `Makefile` references the correct library names.

Alternative: CMake build
------------------------
This repo contains `CMakeLists.txt`. If you prefer CMake, install `mingw-w64-x86_64-cmake` and use:

```bash
mkdir build && cd build
cmake -G "MSYS Makefiles" ..
make
```

(Using CMake may make dependency resolution easier if the `CMakeLists.txt` is configured to locate system libraries.)

How to contribute
-----------------
- Keep shader files in a `shaders/` folder and reference them via a relative `basePath` in source.
- Add build notes to this README if you add platform-specific dependencies.

Contact / Author
----------------
This README was created to help build the project with MSYS2/MinGW. If any specific build errors occur on your machine, paste the build log and I can help debug the errors.

---

(End of README)
