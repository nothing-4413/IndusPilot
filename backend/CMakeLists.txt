set(INDUSPILOT_BACKEND_CORE_SOURCES
  src/app/application.cpp
  src/app/config.cpp
  src/app/logger.cpp
  src/api/api_types.cpp
  src/api/router.cpp
  src/data/data_connectors.cpp
  src/data/in_memory_repositories.cpp
  src/modules/ai_service.cpp
  src/modules/alert_service.cpp
  src/modules/asset_service.cpp
  src/modules/identity_service.cpp
  src/modules/maintenance_service.cpp
  src/modules/monitoring_service.cpp
  src/modules/session_store.cpp
)

if(INDUSPILOT_WITH_DROGON)
  list(APPEND INDUSPILOT_BACKEND_CORE_SOURCES src/http/drogon_server.cpp src/data/mysql_repositories.cpp)
endif()

add_library(induspilot_backend_core ${INDUSPILOT_BACKEND_CORE_SOURCES})

target_include_directories(induspilot_backend_core PUBLIC include)

target_compile_options(induspilot_backend_core PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
)

if(WIN32)
  target_link_libraries(induspilot_backend_core PUBLIC Ws2_32)
endif()

if(INDUSPILOT_WITH_REDIS)
  find_package(redis++ CONFIG REQUIRED)
  target_compile_definitions(induspilot_backend_core PRIVATE INDUSPILOT_WITH_REDIS)
  target_link_libraries(induspilot_backend_core PUBLIC redis++::redis++)
endif()

if(INDUSPILOT_WITH_DROGON)
  find_package(Drogon CONFIG REQUIRED)
  target_compile_definitions(induspilot_backend_core PRIVATE INDUSPILOT_WITH_DROGON)
  target_link_libraries(induspilot_backend_core PUBLIC Drogon::Drogon)
endif()

add_executable(induspilot-backend src/main.cpp)
target_link_libraries(induspilot-backend PRIVATE induspilot_backend_core)
target_compile_options(induspilot-backend PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
)

if(INDUSPILOT_WITH_DROGON)
  target_compile_definitions(induspilot-backend PRIVATE INDUSPILOT_WITH_DROGON)
endif()

if(INDUSPILOT_BUILD_TESTS)
  add_executable(induspilot-backend-tests tests/backend_foundation_tests.cpp)
  target_link_libraries(induspilot-backend-tests PRIVATE induspilot_backend_core)
  target_compile_options(induspilot-backend-tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
  )
  add_test(NAME induspilot-backend-tests COMMAND induspilot-backend-tests)

  if(INDUSPILOT_WITH_DROGON AND WIN32)
    add_test(NAME induspilot-http-integration-smoke
      COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/http_integration_smoke.ps1
        -BackendExe $<TARGET_FILE:induspilot-backend>
        -ConfigPath ${CMAKE_SOURCE_DIR}/config/backend.example.yaml)
  endif()
endif()