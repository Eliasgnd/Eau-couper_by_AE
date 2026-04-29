QT += testlib widgets serialport
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           placement_assist_tests.cpp \
           inventory_safety_tests.cpp \
           stm_safety_tests.cpp \
           shape_manager_tests.cpp \
           main.cpp \
           ../infrastructure/hardware/StmUartService.cpp \
           ../domain/geometry/GeometryUtils.cpp \
           ../domain/shapes/ShapeManager.cpp \
           ../ui/canvas/PlacementAssist.cpp
INCLUDEPATH += .. \
               ../ui/canvas \
               ../domain/geometry \
               ../domain/shapes \
               ../domain/interfaces \
               ../infrastructure/hardware \
               ../shared
HEADERS += placement_tests.h \
           placement_assist_tests.h \
           inventory_safety_tests.h \
           stm_safety_tests.h \
           shape_manager_tests.h \
           ../infrastructure/hardware/StmUartService.h \
           ../infrastructure/hardware/StmProtocol.h \
           ../domain/shapes/ShapeManager.h \
           ../ui/canvas/PlacementAssist.h
