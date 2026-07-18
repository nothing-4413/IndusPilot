add_library(induspilot_backend_core
  src/app/application.cpp
  src/app/config.cpp
  src/app/logger.cpp
  src/api/api_types.cpp
  src/api/router.cpp
  src/data/data_connectors.cpp
  src/modules/ai_service.cpp
  src/modules/alert_service.cpp
  src/modules/asset_service.cpp
  src/modules/identity_service.cpp
  src/modules/maintenance_service.cpp
  src/modules/monitoring_service.cpp
)

target_include_directories(induspilot_backend_core PUBLIC include)

add_executable(induspilot-backend src/main.cpp)
target_link_libraries(induspilot-backend PRIVATE induspilot_backend_core)

if(INDUSPILOT_BUILD_TESTS)
  add_executable(induspilot-backend-tests tests/backend_foundation_tests.cpp)
  target_link_libraries(induspilot-backend-tests PRIVATE induspilot_backend_core)
  add_test(NAME induspilot-backend-tests COMMAND induspilot-backend-tests)
endif()