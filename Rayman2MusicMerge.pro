QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = Rayman2MusicMerge
TEMPLATE = app

INCLUDEPATH += .
DEPENDPATH  += .

SOURCES += MainWindow.cpp APMFile.cpp Merge.cpp AudioOutput.cpp ListWidget.cpp main.cpp
HEADERS += MainWindow.hpp APMFile.hpp Merge.hpp AudioOutput.hpp ListWidget.hpp
FORMS   += MainWindow.ui  Merge.ui

RESOURCES += Imgs.qrc
win32: RC_FILE = Windows/Icon.rc

OBJECTS_DIR = build/obj
MOC_DIR     = build/moc
RCC_DIR     = build/rcc
UI_DIR      = build/ui

win32 {
	QMAKE_LFLAGS += -static-libgcc
	LIBS += -Wl,-Bstatic -lportaudio -Wl,-Bdynamic -lwinmm -luuid
}
else: LIBS += -lportaudio
