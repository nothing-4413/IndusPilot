# Redis 浼氳瘽涓庤繍琛屾€佺紦瀛?
## 瀹㈡埛绔緷璧?
Windows 鏈満宸查€氳繃 vcpkg 瀹夎 Redis C++ 瀹㈡埛绔細

- vcpkg 鏍圭洰褰曪細`C:\Users\20106\vcpkg`
- C++ 瀹㈡埛绔細`redis-plus-plus:x64-windows@1.3.15`
- C 瀹㈡埛绔緷璧栵細`hiredis:x64-windows@1.3.0`
- 鐢ㄦ埛鐜鍙橀噺锛歚VCPKG_ROOT=C:\Users\20106\vcpkg`

鍚敤 Redis 浼氳瘽瀛樺偍鏃讹紝CMake 闇€瑕佷娇鐢?vcpkg toolchain锛?
```powershell
cmake -S . -B build/ninja-msvc-redis -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/20106/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DINDUSPILOT_WITH_REDIS=ON `
  -DINDUSPILOT_BUILD_CLIENT=OFF
```

## 閿┖闂磋鍒?
浼氳瘽閿娇鐢?`induspilot:session:{token}`锛屽€间负鍚庣鍐呴儴鐨勯暱搴﹀墠缂€搴忓垪鍖栫粨鏋勶紝杩囨湡鏃堕棿鐢?Redis TTL 鎺у埗銆傜櫥鍑烘椂鍒犻櫎瀵瑰簲 session key锛岄獙璇佹椂濡傛灉 Redis 涓嶅瓨鍦ㄨ閿垨 TTL 宸插埌鏈燂紝鍒欒姹傜敤鎴烽噸鏂拌璇併€?
鍚庣画妯″潡寤鸿缁х画娌跨敤浠ヤ笅鍓嶇紑锛?
- `induspilot:cache:*`锛氫綆棰戜富鏁版嵁缂撳瓨銆?- `induspilot:state:*`锛氳澶囪繍琛屾€佸揩鐓с€?- `induspilot:queue:*`锛氬憡璀︺€丄I 鍒嗘瀽銆佸伐鍗曟祦杞槦鍒椼€