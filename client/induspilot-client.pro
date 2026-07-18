QT += widgets network
CONFIG += c++17
win32-msvc:QMAKE_CXXFLAGS += /utf-8 /EHsc

TARGET = induspilot-client
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/main_window.cpp \
    src/api_client.cpp

HEADERS += \
    src/main_window.hpp \
    src/api_client.hpp