#include <QtWidgets/QApplication>
#include "screenwidget.h"
#include <QDesktopWidget>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ScreenWidget w;
    w.repaint();

    // w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    // w.resize(QApplication::desktop()->size());
    //w.showFullScreen();
    // w.showMaximized();
    // w.show();
    // w.grabDesktop();

    return a.exec();
}
