//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include <QSplitter>
#include <QTabWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QAction>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QShortcut>
#include <QInputDialog>
#include <QMessageBox>
#include "scene_view.h"
#include "game/locations_list.h"
#include "game/objects.h"
#include "zip.h"
#include "extensions/zip_resources_provider.h"

//------------------------------------------------------------

inline void alert(std::string message)
{
    auto m = new QMessageBox;
    m->setText(message.c_str());
    m->exec();
}

//------------------------------------------------------------

inline QTreeWidgetItem *new_tree_group(std::string name)
{
    auto group = new QTreeWidgetItem;
    group->setText(0, name.c_str());
    group->setFlags(Qt::ItemIsEnabled);
    return group;
}

//------------------------------------------------------------

inline QTreeWidgetItem *new_tree_item(std::string name)
{
    auto item = new QTreeWidgetItem;
    item->setText(0, name.c_str());
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
}

//------------------------------------------------------------

main_window::main_window(QWidget *parent): QMainWindow(parent)
{
    QSplitter *main_splitter = new QSplitter(this);
    setCentralWidget(main_splitter);

    m_objects_tree = new QTreeWidget;
    m_objects_tree->setHeaderLabel("Objects selection");
    main_splitter->addWidget(m_objects_tree);

    m_scene_view = new scene_view(this);
    m_scene_view->update_objects_tree = std::bind(&main_window::update_objects_tree, this);
    main_splitter->addWidget(m_scene_view);

    QTabWidget *navigator = new QTabWidget;
    main_splitter->addWidget(navigator);

    main_splitter->setSizes(QList<int>() << 200 << 1000 << 400);

    auto add_objects_tree = new QTreeWidget;
    add_objects_tree->setHeaderLabel("Objects");
    navigator->insertTab(scene_view::mode_add, add_objects_tree, "Add");
    auto &obj_list = game::get_objects_list();
    for (auto &o: obj_list)
    {
        auto item = new_tree_item(o.id);
        if (o.group.empty())
        {
            add_objects_tree->addTopLevelItem(item);
            continue;
        }

        QList<QTreeWidgetItem*> items = add_objects_tree->findItems(o.group.c_str(), Qt::MatchExactly, 0);
        if (!items.empty())
        {
            items[0]->addChild(item);
            continue;
        }

        auto group = new_tree_group(o.group);
        add_objects_tree->addTopLevelItem(group);
        group->addChild(item);
    }
    connect(add_objects_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(on_add_tree_selected(QTreeWidgetItem*, int)));

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
    if (!ok || item.isEmpty())
        return;

    const int idx = items.indexOf(item);
    if (idx < 0 || idx >= (int)list.size())
        return;

    m_filename.clear();
    clear_mission();
    m_location = list[idx].first;
    m_scene_view->load_location(m_location);
    update_objects_tree();
}

//------------------------------------------------------------

void main_window::on_load_mission()
{
    auto filename = QFileDialog::getOpenFileName(this, "Load mission", "missions", "*.zip");
    if (!filename.length())
        return;

    std::string filename_str = filename.toUtf8().constData();

    auto prov = &nya_resources::get_resources_provider();
    nya_resources::file_resources_provider fprov;
    nya_resources::set_resources_provider(&fprov);
    nya_resources::zip_resources_provider zprov;
    bool result = zprov.open_archive(filename_str.c_str());
    nya_resources::set_resources_provider(prov);
    if (!result)
    {
        alert("Unable to load location " + filename_str);
        return;
    }

    m_filename.assign(filename_str);

    pugi::xml_document doc;
    if (!load_xml(zprov.access("objects.xml"), doc))
        return;

    auto root = doc.first_child();
    std::string loc = root.attribute("location").as_string();
    if (loc.empty())
        return;

    clear_mission();

    m_location = loc;
    m_scene_view->load_location(loc);

    for (pugi::xml_node o = root.child("object"); o; o = o.next_sibling("object"))
    {
        scene_view::object obj;
        obj.name = o.attribute("name").as_string();
        obj.id = o.attribute("id").as_string();
        obj.yaw = o.attribute("yaw").as_float();
        obj.pos.set(o.attribute("x").as_float(), o.attribute("y").as_float(), o.attribute("z").as_float());
        obj.y = o.attribute("editor_y").as_float();
        obj.pos.y -= obj.y;
        m_scene_view->add_object(obj);
    }

    update_objects_tree();
}

//------------------------------------------------------------

void main_window::on_save_mission()
{
    if (m_filename.empty())
    {
        on_save_as_mission();
        return;
    }

    zip_t *zip = zip_open(m_filename.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 0);
    if (!zip)
    {
        alert("Unable to save mission " + m_filename);
        return;
    }

    std::string str = "<!--Open Horizon mission-->\n";
    str += "<mission location=\"" + m_location + "\">\n";
    for (auto &o: m_scene_view->get_objects())
    {
        str += "\t<object ";
        str += "name=\"" + o.name + "\" ";
        str += "id=\"" + o.id + "\" ";
        str += "x=\"" + std::to_string(o.pos.x) + "\" ";
        str += "y=\"" + std::to_string(o.pos.y + o.y) + "\" ";
        str += "z=\"" + std::to_string(o.pos.z) + "\" ";
        str += "yaw=\"" + std::to_string(o.yaw.get_deg()) + "\" ";
        str += "editor_y=\"" + std::to_string(o.y) + "\" ";
        str += "/>\n";
    }

    str += "</mission>\n";

    zip_entry_open(zip, "objects.xml");
    zip_entry_write(zip, str.c_str(), str.length());
    zip_entry_close(zip);

    zip_close(zip);
}

//------------------------------------------------------------

void main_window::on_save_as_mission()
{
    auto filename = QFileDialog::getSaveFileName(this, "Save mission", "missions", "*.zip");
    if (!filename.length())
        return;

    m_filename.assign(filename.toUtf8().constData());
    on_save_mission();
}

//------------------------------------------------------------

void main_window::on_tree_selected(QTreeWidgetItem* item, int)
{
}

//------------------------------------------------------------

void main_window::on_add_tree_selected(QTreeWidgetItem* item, int)
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

void main_window::update_objects_tree()
{
    m_objects_tree->clear();

    auto objects = new_tree_group("objects");
    m_objects_tree->addTopLevelItem(objects);

    for (auto &o: m_scene_view->get_objects())
        objects->addChild(new_tree_item(o.name + " (" + o.id + ")"));

    m_objects_tree->addTopLevelItem(new_tree_item("player spawn"));
    m_objects_tree->expandAll();
}

//------------------------------------------------------------

void main_window::clear_mission()
{
    m_scene_view->clear_objects();
}

//------------------------------------------------------------
