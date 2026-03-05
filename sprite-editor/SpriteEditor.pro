QT       += core gui widgets

TARGET = SpriteEditor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    RomFile.cpp \
    GameDefinition.cpp \
    TileDecoder.cpp \
    PaletteWidget.cpp \
    TileCanvasWidget.cpp \
    SpriteSheetWidget.cpp \
    RawTileBrowserWidget.cpp \
    CompressionHandler.cpp \
    NoneHandler.cpp \
    NemesisDecompressor.cpp \
    KosinskiDecompressor.cpp \
    SpriteReplaceDialog.cpp \
    TileMapWidget.cpp \
    GenesisColorDialog.cpp \
    SpriteCollectionWidget.cpp

HEADERS += \
    MainWindow.h \
    RomFile.h \
    GameDefinition.h \
    TileDecoder.h \
    PaletteWidget.h \
    TileCanvasWidget.h \
    SpriteSheetWidget.h \
    RawTileBrowserWidget.h \
    CompressionHandler.h \
    NoneHandler.h \
    NemesisDecompressor.h \
    KosinskiDecompressor.h \
    SpriteReplaceDialog.h \
    TileMapWidget.h \
    GenesisColorDialog.h \
    SpriteCollectionWidget.h

FORMS += \
    MainWindow.ui \
    SpriteReplaceDialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
