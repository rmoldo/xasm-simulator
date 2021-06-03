QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    arch-window/cpuwindow.cpp \
    assembler/XASMGenerator.cpp \
    assembler/lexer.cpp \
    assembler/parser.cpp \
    assembler/verifier.cpp \
    cgb/cgb.cpp \
    cpu/cpu.cpp \
    editor/codeeditor.cpp \
    editor/linenumberarea.cpp \
    editor/xasmhighlighter.cpp \
    main.cpp \
    mainwindow.cpp \
    memory-viewer/memoryviewer.cpp \
    memory-viewer/memoryviewerdialog.cpp

HEADERS += \
    arch-window/cpuwindow.h \
    assembler/XASMGenerator.h \
    assembler/defs.h \
    assembler/encoding.h \
    assembler/lexer.h \
    assembler/parser.h \
    assembler/token.h \
    assembler/verifier.h \
    cgb/cgb.h \
    cpu/cpu.h \
    editor/codeeditor.h \
    editor/linenumberarea.h \
    editor/xasmhighlighter.h \
    mainwindow.h \
    memory-viewer/memoryviewer.h \
    memory-viewer/memoryviewerdialog.h

FORMS += \
    Forms/cpuwindow.ui \
    Forms/mainwindow.ui \
    Forms/memoryviewerdialog.ui \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
