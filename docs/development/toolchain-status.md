# 鏈湴宸ュ叿閾剧姸鎬?
褰撳墠 Windows 涓绘満宸ュ叿閾剧姸鎬侊細

- Visual Studio Build Tools锛氬凡瀹夎鍒伴粯璁よ矾寰?`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`
- MSVC锛氬凡楠岃瘉锛宍cl` 鐗堟湰涓?`19.44.35228`
- MSBuild锛氬凡瀹夎鍦?Build Tools 榛樿璺緞
- CMake锛氫娇鐢?VS Build Tools 鑷甫 CMake锛岀増鏈?`3.31.6-msvc6`
- Ninja锛氫娇鐢?VS Build Tools 鑷甫 Ninja锛岀増鏈?`1.12.1`
- Qt锛氬綋鍓嶄娇鐢?Anaconda 涓凡鏈?Qt 5.9.7锛屽墠缂€涓?`D:/anaconda/Library`
- vcpkg锛氬凡瀹夎鍒?`C:\Users\20106\vcpkg`
- Redis C++ 瀹㈡埛绔細宸查€氳繃 vcpkg 瀹夎 `redis-plus-plus:x64-windows@1.3.15` 涓?`hiredis:x64-windows@1.3.0`

## PATH 涓庣幆澧冨彉閲忕姸鎬?
宸插皢 VS Build Tools 鑷甫鐨?CMake/Ninja 璺緞鍔犲叆褰撳墠鐢ㄦ埛 PATH锛?
- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja`

宸茶缃敤鎴风幆澧冨彉閲忥細

- `VCPKG_ROOT=C:\Users\20106\vcpkg`

MSVC 涓嶅缓璁洿鎺ュ姞鍏?PATH锛涢渶瑕侀€氳繃 `VsDevCmd.bat` 鎴?CMake 鐨?VS/MSVC 鐜鍔犺浇浣跨敤锛屽洜涓哄畠渚濊禆 INCLUDE/LIB 绛夌紪璇戠幆澧冨彉閲忋€?
## 楠岃瘉缁撴灉

鍚庣涓?Qt 瀹㈡埛绔潎宸查€氳繃 CMake + Ninja + MSVC 鏋勫缓锛?
```powershell
cmake -S . -B build/ninja-msvc-client -G Ninja -DINDUSPILOT_BUILD_CLIENT=ON -DCMAKE_PREFIX_PATH="D:/anaconda/Library"
cmake --build build/ninja-msvc-client
ctest --test-dir build/ninja-msvc-client --output-on-failure
```

鍚敤 Redis 瀹㈡埛绔緷璧栨椂浣跨敤 vcpkg toolchain锛?
```powershell
cmake -S . -B build/ninja-msvc-redis -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/20106/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DINDUSPILOT_WITH_REDIS=ON `
  -DINDUSPILOT_BUILD_CLIENT=OFF
cmake --build build/ninja-msvc-redis
```

## 娉ㄦ剰浜嬮」

褰撳墠椤圭洰璺緞鍖呭惈涓枃鐗规畩瀛楃 `锛乣锛孋Make/MSVC 鍙互鏋勫缓鎴愬姛锛屼絾 qmake/nmake 瀵硅璺緞鐨勭紪鐮佹樉绀哄拰鐩稿璺緞澶勭悊杈冩晱鎰熴€傚悗缁鏋滈亣鍒?Qt 宸ュ叿閾鹃棶棰橈紝寤鸿灏嗕粨搴撹縼绉诲埌绾?ASCII 璺緞杩涜鏋勫缓楠岃瘉銆