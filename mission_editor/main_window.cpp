//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QSplitter>
#include <QTabWidget>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QShortcut>
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

    main_splitter->setSizes(QList<int>() << 1000 << 400);

    m_objects_tree = new QTreeView;
    navigator->insertTab(0, m_objects_tree, "Add");

    m_edit_layout = new QFormLayout;
    QWidget *edit_widget = new QWidget;
    navigator->insertTab(1, edit_widget, "Edit");
    edit_widget->setLayout(m_edit_layout);

    QVBoxLayout *script_layout = new QVBoxLayout;
    QWidget *script_widget = new QWidget;
    navigator->insertTab(2, script_widget, "Script");
    script_widget->setLayout(script_layout);

    QFormLayout *info_layout = new QFormLayout;
    QWidget *info_widget = new QWidget;
    navigator->insertTab(3, info_widget, "Info");
    info_widget->setLayout(info_layout);

    QSignalMapper *m = new QSignalMapper(this);
    for (int i = 0; i < 4; ++i)
    {
        QShortcut *s = new QShortcut(QKeySequence(("Ctrl+" + std::to_string(i+1)).c_str()), this);
        connect(s, SIGNAL(activated()), m, SLOT(map()));
        m->setMapping(s, i);
    }
    connect(m, SIGNAL(mapped(int)), navigator, SLOT(setCurrentIndex(int)));

    setup_menu();
}

//------------------------------------------------------------

void main_window::setup_menu()
{
    QMenu *file_menu = menuBar()->addMenu("File");

    QAction *new_mission = new QAction("New mission", this);
    new_mission->setShortcut(QKeySequence::New);
    this->addAction(new_mission);
    file_menu->addAction(new_mission);
    connect(new_mission, SIGNAL(triggered()), this, SLOT(on_new_mission()));

    QAction *load_mission = new QAction("Load mission", this);
    load_mission->setShortcut(QKeySequence::Open);
    this->addAction(load_mission);
    file_menu->addAction(load_mission);
    connect(load_mission, SIGNAL(triggered()), this, SLOT(on_load_mission()));

    QAction *save_mission = new QAction("Save mission", this);
    save_mission->setShortcut(QKeySequence::Save);
    this->addAction(save_mission);
    file_menu->addAction(save_mission);
    connect(save_mission, SIGNAL(triggered()), this, SLOT(on_save_mission()));
}

//------------------------------------------------------------

void main_window::on_new_mission()
{
}

//------------------------------------------------------------

void main_window::on_load_mission()
{
}

//------------------------------------------------------------

void main_window::on_save_mission()
{
}

//------------------------------------------------------------
