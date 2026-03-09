#include "mainwindow.h"

#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Italic-VariableFont_wdth,wght.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-VariableFont_wdth,wght.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Verdana.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Verdana-Bold.ttf");
    a.setQuitOnLastWindowClosed(false);
    MainWindow w;
    w.show();
    return a.exec();
}

