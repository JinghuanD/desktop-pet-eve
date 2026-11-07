
#include "eve.h"
#include <QApplication>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Eve");
    a.setApplicationDisplayName("Eve");
    // The pet owns independent top-level windows such as settings and dress-up; closing one must not exit the application.
    a.setQuitOnLastWindowClosed(false);
    //qDebug() << "1. QApplication created";

    Eve w;
    //qDebug() << "2. Setup complete";

    w.show();

   // qDebug() << "3. show() complete";

    return a.exec();
}
