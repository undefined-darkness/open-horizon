//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QSplitter>
#include <QTabWidget>
#include "scene_view.h"

//------------------------------------------------------------

main_window::main_window(QWidget *parent): QMainWindow(parent)
{
    QSplitter *main_splitter = new QSplitter(this);
    setCentralWidget(main_splitter);

    m_scene_view = new scene_view(this);
    main_splitter->addWidget(m_scene_view);

    QTabWidget *navigator = new QTabWidget;
    main_splitter->addWidget(navigator);

    m_objects_tree = new QTreeView;
    navigator->insertTab(0, m_objects_tree, "add");

    main_splitter->setSizes(QList<int>() << 1000 << 400);
}

//------------------------------------------------------------
