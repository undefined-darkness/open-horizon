//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QApplication>
#include <QStyleFactory>
#include "util/resources.h"

//------------------------------------------------------------

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));

    setup_resources();

    main_window w;
    w.setWindowTitle("Open-Horizon mission editor");
    w.resize(1300, 700);
    w.show();

    return a.exec();
}
//------------------------------------------------------------
