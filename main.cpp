#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    //show at center
    w.move(QApplication::desktop()->screen()->rect().center() - w.rect().center());

    w.showMaximized();

    return a.exec();
}
