QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           inventory_safety_tests.cpp \
           main.cpp \
           ../motorcontrol.cpp \
           ../GeometryUtils.cpp
INCLUDEPATH += ..
HEADERS += ../motorcontrol.h \
           placement_tests.h \
           inventory_safety_tests.h

QMAKE_CXXFLAGS += -Oz -ffunction-sections -fdata-sections
QMAKE_LFLAGS += -Wl,--gc-sections
