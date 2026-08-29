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
  src/modules/audit_service.cpp
  src/modules/asset_service.cpp
  src/modules/identity_service.cpp
  src/modules/login_rate_limiter.cpp
  src/modules/maintenance_service.cpp
  src/modules/metrics_service.cpp
  src/modules/monitoring_service.cpp
  src/modules/password_hasher.cpp
  src/modules/session_store.cpp
  src/http/http_server_context.cpp
  src/http/http_request_lifecycle.cpp
)

if(INDUSPILOT_WITH_DROGON)
  list(APPEND INDUSPILOT_BACKEND_CORE_SOURCES
    src/http/drogon_server.cpp
    src/http/http_common.cpp
    src/http/routes/auth_routes.cpp
    src/http/routes/asset_routes.cpp
    src/http/routes/monitoring_routes.cpp
    src/http/routes/alert_routes.cpp
    src/http/routes/work_order_routes.cpp
    src/http/routes/audit_routes.cpp
    src/http/routes/ai_routes.cpp
    src/http/routes/platform_routes.cpp
    src/data/mysql_repositories.cpp)
endif()

if(INDUSPILOT_WITH_MONGODB)
  list(APPEND INDUSPILOT_BACKEND_CORE_SOURCES
    src/data/mongodb_repositories.cpp)
endif()

add_library(induspilot_backend_core ${INDUSPILOT_BACKEND_CORE_SOURCES})

target_include_directories(induspilot_backend_core PUBLIC include)
target_link_libraries(induspilot_backend_core PUBLIC Threads::Threads)

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

if(INDUSPILOT_WITH_MONGODB)
  find_package(bsoncxx CONFIG REQUIRED)
  find_package(mongocxx CONFIG REQUIRED)
  if(TARGET mongo::mongocxx_static)
    set(INDUSPILOT_MONGOCXX_TARGET mongo::mongocxx_static)
  else()
    set(INDUSPILOT_MONGOCXX_TARGET mongo::mongocxx_shared)
  endif()
  if(TARGET mongo::bsoncxx_static)
    set(INDUSPILOT_BSONCXX_TARGET mongo::bsoncxx_static)
  else()
    set(INDUSPILOT_BSONCXX_TARGET mongo::bsoncxx_shared)
  endif()
  target_compile_definitions(induspilot_backend_core PUBLIC INDUSPILOT_WITH_MONGODB)
  if(WIN32)
    target_compile_definitions(induspilot_backend_core PRIVATE NOMINMAX)
  endif()
  target_include_directories(induspilot_backend_core BEFORE PUBLIC
    $<TARGET_PROPERTY:${INDUSPILOT_MONGOCXX_TARGET},INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:${INDUSPILOT_BSONCXX_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(induspilot_backend_core PUBLIC
    ${INDUSPILOT_MONGOCXX_TARGET}
    ${INDUSPILOT_BSONCXX_TARGET})
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
  set_tests_properties(induspilot-backend-tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

  if(WIN32)
    add_test(NAME induspilot-deployment-preflight
      COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/deployment/preflight.ps1)
  endif()

  if(INDUSPILOT_WITH_DROGON AND WIN32)
    add_test(NAME induspilot-http-integration-smoke
      COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/http_integration_smoke.ps1
        -BackendExe $<TARGET_FILE:induspilot-backend>
        -ConfigPath ${CMAKE_SOURCE_DIR}/config/backend.example.yaml)
    add_test(NAME induspilot-ai-provider-http-smoke
      COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/ai_provider_http_smoke.ps1
        -BackendExe $<TARGET_FILE:induspilot-backend>
        -ConfigPath ${CMAKE_SOURCE_DIR}/config/backend.example.yaml)
    add_test(NAME induspilot-http-readiness-unavailable-smoke
      COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/tests/http_integration_smoke.ps1
        -BackendExe $<TARGET_FILE:induspilot-backend>
        -ConfigPath ${CMAKE_SOURCE_DIR}/config/backend.example.yaml
        -BaseUrl http://127.0.0.1:18082
        -RepositoryStore mysql
        -MySqlHost 127.0.0.1
        -MySqlPort 1
        -ReadinessOnly
        -ExpectNotReady)

    add_executable(induspilot-http-shutdown-smoke tests/http_shutdown_smoke.cpp)
    target_link_libraries(induspilot-http-shutdown-smoke PRIVATE induspilot_backend_core)
    target_compile_options(induspilot-http-shutdown-smoke PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
    )
    target_compile_definitions(induspilot-http-shutdown-smoke PRIVATE INDUSPILOT_WITH_DROGON)
    add_test(NAME induspilot-http-shutdown-smoke COMMAND $<TARGET_FILE:induspilot-http-shutdown-smoke>)

    add_executable(induspilot-http-common-tests tests/http_common_tests.cpp)
    target_link_libraries(induspilot-http-common-tests PRIVATE induspilot_backend_core)
    target_compile_options(induspilot-http-common-tests PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
    )
    target_compile_definitions(induspilot-http-common-tests PRIVATE INDUSPILOT_WITH_DROGON)
    add_test(NAME induspilot-http-common-tests COMMAND $<TARGET_FILE:induspilot-http-common-tests>)
  endif()
endif()
