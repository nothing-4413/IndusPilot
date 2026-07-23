find_package(Qt6 QUIET COMPONENTS Widgets Network)
if(Qt6_FOUND)
  set(INDUSPILOT_QT_PACKAGE Qt6)
else()
  find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
  set(INDUSPILOT_QT_PACKAGE Qt5)
endif()

set(CMAKE_AUTOMOC ON)

add_executable(induspilot-client
  src/main.cpp
  src/main_window.cpp
  src/api_client.cpp
)

target_include_directories(induspilot-client PRIVATE src)
target_link_libraries(induspilot-client PRIVATE ${INDUSPILOT_QT_PACKAGE}::Widgets ${INDUSPILOT_QT_PACKAGE}::Network)
target_compile_options(induspilot-client PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /EHsc>
)
