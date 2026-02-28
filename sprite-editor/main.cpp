#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // Usage: SpriteEditor [ROM_FILE] [DEFINITION.json]
    // File type is detected by extension: .json = definition, anything else = ROM
    if (a.arguments().size() > 1)
        w.loadFromCommandLine(a.arguments());

    w.show();
    return a.exec();
}
