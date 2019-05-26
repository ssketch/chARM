#---------------------------------------------#
#                                             #
#  Project file for chARM Exoskeleton device  #
#                                             #
#---------------------------------------------#

QT      += core gui widgets opengl
TEMPLATE = app

# specify targets for files created during compilation
TARGET      = chARMconsole
DESTDIR     = ./bin
MOC_DIR     = ./moc
OBJECTS_DIR = ./obj
RCC_DIR     = ./rcc
UI_DIR      = ./ui

# add paths to libraries
LIBS += -luser32
LIBS += -lole32
LIBS += -lshell32
LIBS += -L$$PWD/external/s826_3.3.9/api/x32/ -ls826
LIBS += -L$$PWD/external/gl_32/lib -lOPENGL32
LIBS += -L$$PWD/external/gl_32/lib -lGLU32
CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/external/chai3d-3.1.1/extras/freeglut/lib/Debug/Win32/ -lfreeglut
    LIBS += -L$$PWD/external/chai3d-3.1.1/lib/Debug/Win32/ -lchai3d
} else {
    LIBS += -L$$PWD/external/chai3d-3.1.1/extras/freeglut/lib/Release/Win32/ -lfreeglut
    LIBS += -L$$PWD/external/chai3d-3.1.1/lib/Release/Win32/ -lchai3d
}

# add paths to files associated with libraries
INCLUDEPATH += $$PWD/external/s826_3.3.9/api
INCLUDEPATH += $$PWD/external/chai3d-3.1.1/src
INCLUDEPATH += $$PWD/external/gl_32/include/GL
INCLUDEPATH += $$PWD/external/chai3d-3.1.1/external/glew/include
INCLUDEPATH += $$PWD/external/chai3d-3.1.1/extras/freeglut/include
INCLUDEPATH += $$PWD/external/chai3d-3.1.1/extras/freeglut/src
INCLUDEPATH += $$PWD/external/chai3d-3.1.1/external/Eigen

# point to source, header, and form files created to project
SOURCES += $$PWD/main.cpp \
           $$PWD/mainwindow.cpp \
           $$PWD/expwindow.cpp \
           $$PWD/dialog_setup.cpp \
           $$PWD/dialog_gaintuning.cpp \
           $$PWD/dialog_exp.cpp \
           $$PWD/charmwidget.cpp \
           $$PWD/expwidget.cpp \
           $$PWD/motorcontrol.cpp \
           $$PWD/exo.cpp \
           $$PWD/subject.cpp

HEADERS += $$PWD/mainwindow.h \
           $$PWD/expwindow.h \
           $$PWD/dialog_setup.h \
           $$PWD/dialog_gaintuning.h \
           $$PWD/dialog_exp.h \
           $$PWD/charmwidget.h \
           $$PWD/expwidget.h \
           $$PWD/motorcontrol.h \
           $$PWD/exo.h \
           $$PWD/subject.h

FORMS += $$PWD/mainwindow.ui \
         $$PWD/expwindow.ui \
         $$PWD/dialog_setup.ui \
         $$PWD/dialog_gaintuning.ui \
         $$PWD/dialog_exp.ui
