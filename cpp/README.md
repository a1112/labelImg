# labelImgCpp

Qt 6 C++ / Qt Widgets port of the Python labelImg application.

See [docs/feature-parity-design.md](docs/feature-parity-design.md) for the parity matrix, implementation boundaries, and regression checklist.

## Windows MSVC Build

Install:

- Visual Studio Build Tools with MSVC C++ workload
- Qt 6 MSVC package
- CMake 3.24+

Configure and build:

```powershell
cmake -S cpp -B cpp/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.6.3\msvc2019_64"
cmake --build cpp/build --config Release
ctest --test-dir cpp/build -C Release --output-on-failure
```

Deploy manually when needed:

```powershell
windeployqt cpp\build\Release\labelImgCpp.exe
```

The C++ app reuses resources from the repository root:

- `resources/icons`
- `resources/strings`
- `data/predefined_classes.txt`
