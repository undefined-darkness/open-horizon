//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QShortcut>
#include <QInputDialog>
#include <QMessageBox>
#include "scene_view.h"
#include "game/locations_list.h"
#include "game/objects.h"

//------------------------------------------------------------

inline void alert(std::string message)
{
    auto m = new QMessageBox;
    m->setText(message.c_str());
    m->exec();
}

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

    auto objects_tree = new QTreeWidget;
    navigator->insertTab(scene_view::mode_add, objects_tree, "Add");
    auto &obj_list = game::get_objects_list();
    for (auto &o: obj_list)
    {
        auto item = new QTreeWidgetItem;
        item->setText(0, o.id.c_str());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (o.group.empty())
        {
            objects_tree->addTopLevelItem(item);
            continue;
        }

        QList<QTreeWidgetItem*> items = objects_tree->findItems(o.group.c_str(), Qt::MatchExactly, 0);
        if (!items.empty())
        {
            items[0]->addChild(item);
            continue;
        }

        auto group = new QTreeWidgetItem;
        group->setText(0, o.group.c_str());
        group->setFlags(Qt::ItemIsEnabled);
        objects_tree->addTopLevelItem(group);
        group->addChild(item);
    }
    connect(objects_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(on_tree_selected(QTreeWidgetItem*, int)));

    m_edit_layout = new QFormLayout;
    QWidget *edit_widget = new QWidget;
    navigator->insertTab(scene_view::mode_edit, edit_widget, "Edit");
    edit_widget->setLayout(m_edit_layout);

    QWidget *path_widget = new QWidget;
    navigator->insertTab(scene_view::mode_path, path_widget, "Path");

    QWidget *zone_widget = new QWidget;
    navigator->insertTab(scene_view::mode_zone, zone_widget, "Zone");

    QVBoxLayout *script_layout = new QVBoxLayout;
    QWidget *script_widget = new QWidget;
    navigator->insertTab(scene_view::mode_other, script_widget, "Script");
    script_widget->setLayout(script_layout);

    auto info_layout = new QFormLayout;
    QWidget *info_widget = new QWidget;
    navigator->insertTab(scene_view::mode_other + 1, info_widget, "Info");
    info_widget->setLayout(info_layout);

    QSignalMapper *m = new QSignalMapper(this);
    for (int i = 0; i < scene_view::mode_other + 2; ++i)
    {
        QShortcut *s = new QShortcut(QKeySequence(("Ctrl+" + std::to_string(i+1)).c_str()), this);
        connect(s, SIGNAL(activated()), m, SLOT(map()));
        m->setMapping(s, i);
    }
    connect(m, SIGNAL(mapped(int)), navigator, SLOT(setCurrentIndex(int)));
    connect(navigator, SIGNAL(currentChanged(int)), this, SLOT(on_mode_changed(int)));

    m_scene_view->set_mode(scene_view::mode_add);

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

    QAction *save_as_mission = new QAction("Save as mission", this);
    save_as_mission->setShortcut(QKeySequence::SaveAs);
    this->addAction(save_as_mission);
    file_menu->addAction(save_as_mission);
    connect(save_as_mission, SIGNAL(triggered()), this, SLOT(on_save_as_mission()));
}

//------------------------------------------------------------

void main_window::on_new_mission()
{
    QStringList items;
    auto &list = game::get_locations_list();
    for (auto &l: list)
    {
        auto str = QString::fromWCharArray(l.second.c_str(), l.second.size());
        str.append((" [" + l.first + "]").c_str());
        items << str;
    }

    bool ok = false;
    QString item = QInputDialog::getItem(this, "Select location", "Location:", items, 0, false, &ok);
    if (!ok && item.isEmpty())
        return;

    const int idx = items.indexOf(item);
    if (idx < 0 || idx >= (int)list.size())
        return;

    m_scene_view->load_location(list[idx].first);
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

void main_window::on_save_as_mission()
{
}

//------------------------------------------------------------

void main_window::on_tree_selected(QTreeWidgetItem* item, int)
{
    if (item && item->parent())
        m_scene_view->set_selected_add(item->text(0).toUtf8().constData());
    else
        m_scene_view->set_selected_add("");
}

//------------------------------------------------------------

void main_window::on_mode_changed(int idx)
{
    if (idx >= scene_view::mode_other)
        m_scene_view->set_mode(scene_view::mode_other);
    else
        m_scene_view->set_mode(scene_view::mode(idx));
}

//------------------------------------------------------------
