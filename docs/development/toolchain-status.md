# 本地工具链状态

当前 Windows 主机工具链状态：

- Visual Studio Build Tools：已安装到默认路径 `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`
- MSVC：已验证，`cl` 版本为 19.44.35228
- MSBuild：已安装在 Build Tools 默认路径
- CMake：使用 VS Build Tools 自带 CMake，版本 `3.31.6-msvc6`
- Ninja：使用 VS Build Tools 自带 Ninja，版本 `1.12.1`
- Qt：当前使用 Anaconda 中已有 Qt 5.9.7，前缀为 `D:/anaconda/Library`

## PATH 状态

已将 VS Build Tools 自带的 CMake/Ninja 路径加入当前用户 PATH：

- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja`

MSVC 不建议直接裸加 PATH；需要通过 `VsDevCmd.bat` 或 CMake 的 VS/MSVC 环境加载使用，因为它依赖 INCLUDE/LIB 等编译环境变量。

## 验证结果

后端与 Qt 客户端均已通过 CMake + Ninja + MSVC 构建：

```powershell
cmake -S . -B build/ninja-msvc-client -G Ninja -DINDUSPILOT_BUILD_CLIENT=ON -DCMAKE_PREFIX_PATH="D:/anaconda/Library"
cmake --build build/ninja-msvc-client
ctest --test-dir build/ninja-msvc-client --output-on-failure
```

验证结果：

- `backend/induspilot-backend.exe`：已生成
- `backend/induspilot-backend-tests.exe`：已生成
- `client/induspilot-client.exe`：已生成
- CTest：`1/1` 通过

## 注意事项

当前项目路径包含中文特殊字符 `！`。CMake/MSVC 可以构建成功，但 qmake/nmake 对该路径的编码显示和相对路径处理较敏感。后续如果遇到 Qt 工具链问题，建议将仓库迁移到纯 ASCII 路径进行构建验证。