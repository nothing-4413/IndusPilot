# 閺堫剙婀村鈧崣鎴濇儙閸?
## 娓氭繆绂嗛張宥呭

```powershell
cd deployment
docker compose up -d
```

閸氼垰濮╅崥搴濈窗閺嗘挳婀堕敍?
- MySQL閿涙瓪127.0.0.1:3306`
- Redis閿涙瓪127.0.0.1:6379`
- MongoDB閿涙瓪127.0.0.1:27017`

## 闁板秶鐤嗛弬鍥︽

婢跺秴鍩楃粈杞扮伐闁板秶鐤嗛崥搴″晙鐠嬪啯鏆ｉ張顒佹簚鐎靛棛鐖滈崪宀€顏崣锝忕窗

```powershell
copy config/backend.example.yaml config/backend.yaml
copy config/client.example.json config/client.json
copy config/ai.example.yaml config/ai.yaml
```

## 缂傛牞鐦ч崥搴ｎ伂閸滃苯顓归幋椋庮伂

姒涙顓婚弸鍕紦娴ｈ法鏁ら崘鍛摠娴兼俺鐦界€涙ê鍋嶉敍宀勨偓鍌氭値韫囶偊鈧喐绁寸拠鏇窗

```powershell
cmake -S . -B build/ninja-msvc-client -G Ninja -DINDUSPILOT_BUILD_CLIENT=ON -DCMAKE_PREFIX_PATH="D:/anaconda/Library"
cmake --build build/ninja-msvc-client
ctest --test-dir build/ninja-msvc-client --output-on-failure
```

## 閸氼垳鏁?Redis 娴兼俺鐦界€涙ê鍋?
Redis C++ 鐎广垺鍩涚粩顖氬嚒鐎瑰顥婇崷?`C:\Users\20106\vcpkg`閿涘苯鑻熺拋鍓х枂娴滃棛鏁ら幋椋庡箚婢у啫褰夐柌?`VCPKG_ROOT`閵嗗倸鎯庨悽?Redis-backed session 閺冩湹濞囬悽顭掔窗

```powershell
cmake -S . -B build/ninja-msvc-redis -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/20106/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DINDUSPILOT_WITH_REDIS=ON `
  -DINDUSPILOT_BUILD_CLIENT=OFF
cmake --build build/ninja-msvc-redis
```

閸氬海顏禒锝囩垳娑擃厾娈?`RedisSessionStore` 娴ｈ法鏁?`redis-plus-plus` 鏉╃偞甯?Redis閿涘奔绶ユ俊鍌︾窗`tcp://127.0.0.1:6379`閵嗗倻鏁撴禍褏骞嗘晶鍐ㄧ安娴犲酣鍘ょ純顔芥瀮娴犳儼顕伴崣?URI閵嗕浇绻涢幒銉︾潨閵嗕礁鐣ㄩ崗銊吇鐠囦礁鎷版导姘崇樈 TTL閵?
## CMake Preset 蹇嵎鍛戒护

`powershell
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

cmake --preset dev-redis
cmake --build --preset dev-redis
ctest --preset dev-redis
`澧
