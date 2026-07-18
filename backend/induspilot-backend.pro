TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle

TARGET = induspilot-backend
INCLUDEPATH += include
win32-msvc:QMAKE_CXXFLAGS += /utf-8 /EHsc

SOURCES += \
    src/main.cpp \
    src/app/application.cpp \
    src/app/config.cpp \
    src/app/logger.cpp \
    src/api/api_types.cpp \
    src/api/router.cpp \
    src/data/data_connectors.cpp \
    src/modules/ai_service.cpp \
    src/modules/alert_service.cpp \
    src/modules/asset_service.cpp \
    src/modules/identity_service.cpp \
    src/modules/maintenance_service.cpp \
    src/modules/monitoring_service.cpp