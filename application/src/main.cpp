#include "controlpanel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ControlPanel w;
    w.show();
    
    return a.exec();
}
