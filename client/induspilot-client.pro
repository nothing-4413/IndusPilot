QT += widgets network
CONFIG += c++17

TARGET = induspilot-client
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/main_window.cpp \
    src/api_client.cpp

HEADERS += \
    src/main_window.hpp \
    src/api_client.hpp