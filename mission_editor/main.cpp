//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QApplication>
#include <QStyleFactory>

//------------------------------------------------------------

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));

    main_window w;
    w.setWindowTitle("Open-Horizon mission editor");
    w.resize(1200, 700);
    w.show();

    return a.exec();
}
//------------------------------------------------------------
