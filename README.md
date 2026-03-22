# Planetary Engineering

A 3D voxel planet exploration and spacecraft engineering game built with **raylib** and **C++17**.

---

## Requirements

- [CMake](https://cmake.org/) 3.21 or newer
- A C++17-compatible compiler (MSVC 2019+, GCC 11+, or Clang 13+)
- Git (used by CMake to fetch raylib automatically)
- An internet connection on first build (to download raylib)

---

## Build Instructions

### Windows (Visual Studio / MSVC)

```bat
git clone https://github.com/NoahWhiteson/Voxonaut.git
cd Voxonaut
cmake -B build
cmake --build build --config Release
```

The executable will be at:
```
build\Release\Voxonaut.exe
```

### Linux / macOS

```bash
git clone https://github.com/NoahWhiteson/Voxonaut.git
cd Voxonaut
cmake -B build
cmake --build build --config Release
```

The executable will be at:
```
build/Release/Voxonaut
```

> **Note:** On Linux you may need to install raylib system dependencies first:
> ```bash
> sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
> ```

---

## Controls

| Input | Action |
|---|---|
| Right-click drag | Orbit / pan camera |
| Scroll wheel | Zoom in/out (descend to surface) |
| `B` | Toggle build mode |
| Left-click | Place part (build mode) |
| Right-click | Remove part (build mode) |

---

## Notes

- raylib is fetched automatically via CMake FetchContent — no manual install required.
- First build will take a few minutes while raylib compiles.
- Subsequent builds are fast (only your source files recompile).
