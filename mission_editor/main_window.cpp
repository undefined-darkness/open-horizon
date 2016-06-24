//
// open horizon -- undefined_darkness@outlook.com
//

#include "main_window.h"
#include "scene_view.h"
#include "game/locations_list.h"
#include "game/objects.h"
#include "game/script.h"
#include "zip.h"
#include "extensions/zip_resources_provider.h"

//------------------------------------------------------------

namespace
{
enum modes
{
    mode_add = scene_view::mode_add,
    mode_edit = scene_view::mode_edit,
    mode_path = scene_view::mode_path,
    mode_zone = scene_view::mode_zone,
    mode_script = scene_view::mode_other,
    mode_info,

    modes_count
};

enum edit_modes
{
    edit_none,
    edit_multiple,
    edit_object,
    edit_path,
    edit_zone
};

}

inline void alert(std::string message)
{
    auto m = new QMessageBox;
    m->setText(message.c_str());
    m->exec();
}

inline const char *to_str(const QString &s) { return s.toUtf8().constData(); }

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
    m_objects_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_objects_tree, SIGNAL(itemSelectionChanged()), this, SLOT(on_obj_selected()));
    connect(m_objects_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(on_obj_focus(QTreeWidgetItem*, int)));

    main_splitter->addWidget(m_objects_tree);

    m_scene_view = new scene_view(this);
    m_scene_view->update_objects_tree = std::bind(&main_window::update_objects_tree, this);
    m_scene_view->set_selection = std::bind(&main_window::set_selection, this, std::placeholders::_1, std::placeholders::_2);
    main_splitter->addWidget(m_scene_view);

    m_navigator = new QTabWidget;
    main_splitter->addWidget(m_navigator);

    main_splitter->setSizes(QList<int>() << 200 << 1000 << 450);

    auto add_objects_tree = new QTreeWidget;
    add_objects_tree->setHeaderLabel("Objects");
    m_navigator->insertTab(mode_add, add_objects_tree, "Add");
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

    m_edit = new QStackedWidget;
    m_navigator->insertTab(mode_edit, m_edit, "Edit");

    auto edit_none = new QLabel;
    edit_none->setText("\nNo editable object selected");
    edit_none->setAlignment(Qt::AlignTop);
    m_edit->addWidget(edit_none);
    auto edit_multiple = new QLabel;
    edit_multiple->setText("\nMultiple objects selected");
    edit_multiple->setAlignment(Qt::AlignTop);
    m_edit->addWidget(edit_multiple);

    auto edit_obj_l = new QFormLayout;
    m_edit_obj_name = new QLineEdit;
    connect(m_edit_obj_name, SIGNAL(textChanged(const QString &)), this, SLOT(on_name_changed(const QString &)));
    edit_obj_l->addRow("Name:", m_edit_obj_name);
    m_edit_obj_active = new QCheckBox;
    connect(m_edit_obj_active, SIGNAL(stateChanged(int)), this, SLOT(on_active_changed(int)));
    edit_obj_l->addRow("Active:", m_edit_obj_active);
    auto edit_obj_w = new QWidget;
    edit_obj_w->setLayout(edit_obj_l);
    m_edit->addWidget(edit_obj_w);

    auto edit_path_l = new QFormLayout;
    m_edit_path_name = new QLineEdit;
    connect(m_edit_path_name, SIGNAL(textChanged(const QString &)), this, SLOT(on_name_changed(const QString &)));
    edit_path_l->addRow("Name:", m_edit_path_name);
    m_edit_path_active = new QCheckBox;
    connect(m_edit_path_active, SIGNAL(stateChanged(int)), this, SLOT(on_active_changed(int)));
    edit_path_l->addRow("Active:", m_edit_path_active);
    auto edit_path_w = new QWidget;
    edit_path_w->setLayout(edit_path_l);
    m_edit->addWidget(edit_path_w);

    auto edit_zone_l = new QFormLayout;
    m_edit_zone_name = new QLineEdit;
    connect(m_edit_zone_name, SIGNAL(textChanged(const QString &)), this, SLOT(on_name_changed(const QString &)));
    edit_zone_l->addRow("Name:", m_edit_zone_name);
    m_edit_zone_active = new QCheckBox;
    connect(m_edit_zone_active, SIGNAL(stateChanged(int)), this, SLOT(on_active_changed(int)));
    edit_zone_l->addRow("Active:", m_edit_zone_active);
    auto edit_zone_w = new QWidget;
    edit_zone_w->setLayout(edit_zone_l);
    m_edit->addWidget(edit_zone_w);

    QWidget *path_widget = new QWidget;
    m_navigator->insertTab(mode_path, path_widget, "Path");

    QWidget *zone_widget = new QWidget;
    m_navigator->insertTab(mode_zone, zone_widget, "Zone");

    auto *script_layout = new QSplitter(Qt::Vertical);
    m_script_edit = new QTextEdit;
    connect(m_script_edit, SIGNAL(textChanged()), this, SLOT(on_script_changed()));
    new highlight_lua(m_script_edit->document());
    script_layout->addWidget(m_script_edit);
    m_script_errors = new QTextEdit;
    m_script_errors->setReadOnly(true);
    m_script_errors->setTextColor(Qt::red);
    script_layout->addWidget(m_script_errors);
    m_compile_timer = new QTimer(this);
    connect(m_compile_timer, SIGNAL(timeout()), this, SLOT(on_compile_script()));
    script_layout->setSizes(QList<int>() << 1000 << 100);
    m_navigator->insertTab(mode_script, script_layout, "Script");

    auto info_layout = new QFormLayout;
    QWidget *info_widget = new QWidget;
    m_navigator->insertTab(mode_info, info_widget, "Info");
    info_widget->setLayout(info_layout);
    info_layout->addRow("Title:", m_mission_title = new QLineEdit);
    info_layout->addRow("Author:", m_mission_author = new QLineEdit);
    info_layout->addRow("Email:", m_mission_email = new QLineEdit);
    info_layout->addRow("Desc:", m_mission_description = new QTextEdit);

    QSignalMapper *m = new QSignalMapper(this);
    for (int i = 0; i < modes_count; ++i)
    {
        QShortcut *s = new QShortcut(QKeySequence(("Ctrl+" + std::to_string(i+1)).c_str()), this);
        connect(s, SIGNAL(activated()), m, SLOT(map()));
        m->setMapping(s, i);
    }
    connect(m, SIGNAL(mapped(int)), m_navigator, SLOT(setCurrentIndex(int)));
    connect(m_navigator, SIGNAL(currentChanged(int)), this, SLOT(on_mode_changed(int)));

    m_scene_view->set_mode(scene_view::mode_other);
    setup_menu();
}

//------------------------------------------------------------

void main_window::setup_menu()
{
    QMenu *file_menu = menuBar()->addMenu("File");

    QAction *new_mission = new QAction("New mission", this);
    new_mission->setShortcut(QKeySequence::New);
    file_menu->addAction(new_mission);
    connect(new_mission, SIGNAL(triggered()), this, SLOT(on_new_mission()));

    QAction *load_mission = new QAction("Load mission", this);
    load_mission->setShortcut(QKeySequence::Open);
    file_menu->addAction(load_mission);
    connect(load_mission, SIGNAL(triggered()), this, SLOT(on_load_mission()));

    QAction *save_mission = new QAction("Save mission", this);
    save_mission->setShortcut(QKeySequence::Save);
    file_menu->addAction(save_mission);
    connect(save_mission, SIGNAL(triggered()), this, SLOT(on_save_mission()));

    QAction *save_as_mission = new QAction("Save as mission", this);
    save_as_mission->setShortcut(QKeySequence::SaveAs);
    file_menu->addAction(save_as_mission);
    connect(save_as_mission, SIGNAL(triggered()), this, SLOT(on_save_as_mission()));

    QMenu *edit_menu = menuBar()->addMenu("Edit");

    QAction *select_all = new QAction("Select all", this);
    select_all->setShortcut(QKeySequence::SelectAll);
    edit_menu->addAction(select_all);
    connect(select_all, SIGNAL(triggered()), m_objects_tree, SLOT(selectAll()));

    QAction *select_none = new QAction("Select none", this);
    select_none->setShortcut(QKeySequence("Ctrl+D")); //QKeySequence::Deselect
    edit_menu->addAction(select_none);
    connect(select_none, SIGNAL(triggered()), m_objects_tree, SLOT(clearSelection()));

    QAction *delete_selected = new QAction("Delete selected", this);
#ifdef Q_OS_MAC
    delete_selected->setShortcut(QKeySequence("Backspace"));
#else
    delete_selected->setShortcut(QKeySequence::Delete);
#endif
    edit_menu->addAction(delete_selected);
    connect(delete_selected, SIGNAL(triggered()), this, SLOT(on_delete()));
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
    scene_view::object p;
    p.pos.y = m_scene_view->get_height(p.pos.x, p.pos.z);
    p.y = 100.0f;
    m_scene_view->set_player(p);
    update_objects_tree();

    m_script_edit->setText("--Open-Horizon mission script\n\n"
                           "function init()\n"
                           "    --do init here\n"
                           "end\n");

    m_mission_title->setText("Mission");

    m_navigator->setCurrentIndex(mode_info);
}

//------------------------------------------------------------

void main_window::on_load_mission()
{
    auto filename = QFileDialog::getOpenFileName(this, "Load mission", "missions", "*.zip");
    if (!filename.length())
        return;

    std::string filename_str = to_str(filename);

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

    auto p = root.child("player");
    scene_view::object plr;
    plr.yaw = p.attribute("yaw").as_float();
    plr.pos.set(p.attribute("x").as_float(), p.attribute("y").as_float(), p.attribute("z").as_float());
    plr.y = p.attribute("editor_y").as_float();
    plr.pos.y -= plr.y;
    m_scene_view->set_player(plr);

    for (auto o = root.child("object"); o; o = o.next_sibling("object"))
    {
        scene_view::object obj;
        obj.name = o.attribute("name").as_string();
        obj.id = o.attribute("id").as_string();
        obj.active = o.attribute("active").as_bool();
        obj.yaw = o.attribute("yaw").as_float();
        obj.pos.set(o.attribute("x").as_float(), o.attribute("y").as_float(), o.attribute("z").as_float());
        obj.y = o.attribute("editor_y").as_float();
        obj.pos.y -= obj.y;
        m_scene_view->add_object(obj);
    }

    for (auto z = root.child("zone"); z; z = z.next_sibling("zone"))
    {
        scene_view::zone zn;
        zn.name = z.attribute("name").as_string();
        zn.active = z.attribute("active").as_bool();
        zn.radius = z.attribute("radius").as_float();
        zn.pos.set(z.attribute("x").as_float(), z.attribute("y").as_float(), z.attribute("z").as_float());
        m_scene_view->add_zone(zn);
    }

    for (auto p = root.child("path"); p; p = p.next_sibling("path"))
    {
        scene_view::path pth;
        pth.name = p.attribute("name").as_string();
        pth.active = p.attribute("active").as_bool();
        for (auto p0 = p.child("point"); p0; p0 = p0.next_sibling("point"))
        {
            nya_math::vec4 p;
            p.xyz().set(p0.attribute("x").as_float(), p0.attribute("y").as_float(), p0.attribute("z").as_float());
            p.w = p0.attribute("editor_y").as_float();
            p.y -= p.w;
            pth.points.push_back(p);
        }
        m_scene_view->get_paths().push_back(pth);
    }

    auto script_res = zprov.access("script.lua");
    if (script_res)
    {
        std::string script;
        script.resize(script_res->get_size());
        if (!script.empty())
            script_res->read_all(&script[0]);
        m_script_edit->setText(script.c_str());
        script_res->release();
    }

    if (load_xml(zprov.access("info.xml"), doc))
    {
        auto root = doc.first_child();

        m_mission_title->setText(root.attribute("name").as_string());
        m_mission_description->setText(root.child("description").first_child().value());

        auto author = root.child("author");
        m_mission_author->setText(author.attribute("name").as_string());
        m_mission_email->setText(author.attribute("email").as_string());
    }

    update_objects_tree();
    m_navigator->setCurrentIndex(mode_info);
}

//------------------------------------------------------------

inline std::string to_string(bool b) { return b ? "true" : "false"; }

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

    auto &p = m_scene_view->get_player();
    str += "\t<player ";
    str += "x=\"" + std::to_string(p.pos.x) + "\" ";
    str += "y=\"" + std::to_string(p.pos.y + p.y) + "\" ";
    str += "z=\"" + std::to_string(p.pos.z) + "\" ";
    str += "yaw=\"" + std::to_string(p.yaw.get_deg()) + "\" ";
    str += "editor_y=\"" + std::to_string(p.y) + "\" ";
    str += "/>\n";

    for (auto &o: m_scene_view->get_objects())
    {
        str += "\t<object ";
        str += "name=\"" + o.name + "\" ";
        str += "id=\"" + o.id + "\" ";
        str += "active=\"" + to_string(o.active) + "\" ";
        str += "x=\"" + std::to_string(o.pos.x) + "\" ";
        str += "y=\"" + std::to_string(o.pos.y + o.y) + "\" ";
        str += "z=\"" + std::to_string(o.pos.z) + "\" ";
        str += "yaw=\"" + std::to_string(o.yaw.get_deg()) + "\" ";
        str += "editor_y=\"" + std::to_string(o.y) + "\" ";
        str += "/>\n";
    }

    for (auto &z: m_scene_view->get_zones())
    {
        str += "\t<zone ";
        str += "name=\"" + z.name + "\" ";
        str += "active=\"" + to_string(z.active) + "\" ";
        str += "x=\"" + std::to_string(z.pos.x) + "\" ";
        str += "y=\"" + std::to_string(z.pos.y) + "\" ";
        str += "z=\"" + std::to_string(z.pos.z) + "\" ";
        str += "radius=\"" + std::to_string(z.radius) + "\" ";
        str += "/>\n";
    }

    for (auto &pth: m_scene_view->get_paths())
    {
        str += "\t<path ";
        str += "name=\"" + pth.name + "\" ";
        str += "active=\"" + to_string(pth.active) + "\" ";
        str += ">\n";

        for (auto &p: pth.points)
        {
            str += "\t\t<point ";
            str += "x=\"" + std::to_string(p.x) + "\" ";
            str += "y=\"" + std::to_string(p.y + p.w) + "\" ";
            str += "z=\"" + std::to_string(p.z) + "\" ";
            str += "editor_y=\"" + std::to_string(p.w) + "\" ";
            str += "/>\n";
        }

        str += "\t</path>\n";
    }

    str += "</mission>\n";

    zip_entry_open(zip, "objects.xml");
    zip_entry_write(zip, str.c_str(), str.length());
    zip_entry_close(zip);

    std::string script = to_str(m_script_edit->toPlainText());
    zip_entry_open(zip, "script.lua");
    zip_entry_write(zip, script.c_str(), script.size());
    zip_entry_close(zip);

    std::string info = "<!--Open Horizon mission info-->\n";
    info += "<info ";
    info += "name=\"" + std::string(to_str(m_mission_title->text())) + "\">\n";
    info += "\t<author name=\"" + std::string(to_str(m_mission_author->text())) + "\" ";
    info += "email=\"" + std::string(to_str(m_mission_email->text())) + "\"/>\n";
    info += "\t<description>";
    info += to_str(m_mission_description->toPlainText());
    info += "</description>\n";
    info += "</info>\n";
    zip_entry_open(zip, "info.xml");
    zip_entry_write(zip, info.c_str(), info.size());
    zip_entry_close(zip);

    zip_close(zip);
}

//------------------------------------------------------------

void main_window::on_save_as_mission()
{
    if (m_location.empty())
        return;

    auto filename = QFileDialog::getSaveFileName(this, "Save mission", "missions", "*.zip");
    if (!filename.length())
        return;

    m_filename.assign(to_str(filename));
    on_save_mission();
}

//------------------------------------------------------------

void main_window::on_obj_selected()
{
    m_scene_view->clear_selection();

    auto items = m_objects_tree->selectedItems();
    if (items.empty())
    {
        m_scene_view->update();
        m_edit->setCurrentIndex(edit_none);
        return;
    }

    for (auto &item: items)
    {
        for (int i = 0; i < m_objects_tree->topLevelItemCount(); ++i)
        {
           QTreeWidgetItem *p = m_objects_tree->topLevelItem(i);
           if (item == p)
           {
               m_scene_view->select(to_str(p->text(0)), 0);
               break;
           }

           const int idx = p->indexOfChild(item);
           if (idx < 0)
               continue;

           m_scene_view->select(to_str(p->text(0)), idx);
        }
    }

    if (items.size() == 1)
    {
        auto *p = items[0]->parent();
        if (p)
        {
            auto &s = m_scene_view->get_selected();

            if (p->text(0) == "objects")
            {
                m_edit_obj_name->setText(s.name.c_str());
                m_edit_obj_active->setChecked(s.active);
                m_edit->setCurrentIndex(edit_object);
            }
            else if (p->text(0) == "paths")
            {
                m_edit_path_name->setText(s.name.c_str());
                m_edit_path_active->setChecked(s.active);
                m_edit->setCurrentIndex(edit_path);
            }
            else if (p->text(0) == "zones")
            {
                m_edit_zone_name->setText(s.name.c_str());
                m_edit_zone_active->setChecked(s.active);
                m_edit->setCurrentIndex(edit_zone);
            }
            else
                m_edit->setCurrentIndex(edit_none);
        }
        else
            m_edit->setCurrentIndex(edit_none);
    }
    else
        m_edit->setCurrentIndex(edit_multiple);

    if (m_navigator->currentIndex() != mode_path)
        m_navigator->setCurrentIndex(mode_edit);

    m_scene_view->update();
}

//------------------------------------------------------------

void main_window::on_obj_focus(QTreeWidgetItem *item, int)
{
    for (int i = 0; i < m_objects_tree->topLevelItemCount(); ++i)
    {
       QTreeWidgetItem *p = m_objects_tree->topLevelItem(i);
       if (item == p)
       {
           m_scene_view->set_focus(to_str(p->text(0)), 0);
           return;
       }

       const int idx = p->indexOfChild(item);
       if (idx < 0)
           continue;

       m_scene_view->set_focus(to_str(p->text(0)), idx);
       return;
    }
}

//------------------------------------------------------------

void main_window::on_add_tree_selected(QTreeWidgetItem* item, int)
{
    if (item && item->parent())
        m_scene_view->set_selected_add(to_str(item->text(0)));
    else
        m_scene_view->set_selected_add("");
}

//------------------------------------------------------------

void main_window::on_delete()
{
    m_scene_view->delete_selected();
}

//------------------------------------------------------------

void main_window::on_mode_changed(int idx)
{
    if (m_location.empty() || idx >= scene_view::mode_other)
        m_scene_view->set_mode(scene_view::mode_other);
    else
        m_scene_view->set_mode(scene_view::mode(idx));

    if (idx != scene_view::mode_edit)
        m_objects_tree->clearSelection();

    m_scene_view->update();
}

//------------------------------------------------------------

void main_window::on_name_changed(const QString &s)
{
    m_scene_view->get_selected().name = to_str(s);
    auto c = m_objects_tree->currentItem();
    if (!c)
        return;

    if (!c->parent() || c->parent()->text(0) != "objects")
    {
        c->setText(0, s);
        return;
    }

    const int idx = c->parent()->indexOfChild(c);
    auto &objs = m_scene_view->get_objects();
    if (idx < objs.size())
        c->setText(0, (std::string(to_str(s)) + " (" + objs[idx].id + ")").c_str());
}

//------------------------------------------------------------

void main_window::on_active_changed(int state)
{
    m_scene_view->get_selected().active = state > 0;
}

//------------------------------------------------------------

void main_window::on_script_changed()
{
    m_compile_timer->start(1000);
}

//------------------------------------------------------------

void main_window::on_compile_script()
{
    m_compile_timer->stop();

    auto t = m_script_edit->toPlainText();
    game::script s;
    s.load(to_str(t));
    m_script_errors->setText(s.get_error().c_str());
}

//------------------------------------------------------------

void main_window::update_objects_tree()
{
    m_objects_tree->clear();

    auto objects = new_tree_group("objects");
    m_objects_tree->addTopLevelItem(objects);
    for (auto &o: m_scene_view->get_objects())
        objects->addChild(new_tree_item(o.name + " (" + o.id + ")"));

    auto zones = new_tree_group("zones");
    m_objects_tree->addTopLevelItem(zones);
    for (auto &z: m_scene_view->get_zones())
        zones->addChild(new_tree_item(z.name));

    auto paths = new_tree_group("paths");
    m_objects_tree->addTopLevelItem(paths);
    for (auto &p: m_scene_view->get_paths())
        paths->addChild(new_tree_item(p.name));

    m_objects_tree->addTopLevelItem(new_tree_item("player spawn"));
    m_objects_tree->expandAll();
}

//------------------------------------------------------------

void main_window::set_selection(std::string group, int idx)
{
    m_objects_tree->clearSelection();
    if (idx < 0)
        return;

    for (int i = 0; i < m_objects_tree->topLevelItemCount(); ++i)
    {
       QTreeWidgetItem *p = m_objects_tree->topLevelItem(i);
       if (p->text(0) == group.c_str())
       {
           if (idx >= p->childCount())
               return;

           m_objects_tree->setCurrentItem(p->child(idx));
           return;
       }
    }
}

//------------------------------------------------------------

void main_window::clear_mission()
{
    m_mission_title->setText("");
    m_mission_author->setText("");
    m_mission_email->setText("");
    m_mission_description->setText("");

    m_scene_view->clear_objects();
    m_scene_view->clear_zones();
    m_scene_view->get_paths().clear();
}

//------------------------------------------------------------

highlight_lua::highlight_lua(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    QColor keyword_color = QColor(0xcc, 0, 0xa1);
    std::string keywords[] = {"and",    "break",  "do",   "else",     "elseif",
                              "end",    "false",  "for",  "function", "if",
                              "in",     "local",  "nil",  "not",      "or",
                              "repeat", "return", "then", "true",     "until", "while"};
    for (auto k: keywords)
        m_rules.push_back({QRegExp(("\\b" + k + "\\b").c_str()), keyword_color});

    m_rules.push_back({QRegExp("[0-9]"), QColor(0x5c, 0, 0xdd)});

    m_rules.push_back({QRegExp("\\b[A-Za-z0-9_]+(?=\\()"), QColor(0x6f, 0, 0x8f)});

    m_rules.push_back({QRegExp("\".*\""), QColor(0xe4, 0, 0x41)});
    m_rules.back().first.setMinimal(true);
    m_rules.push_back({QRegExp("\'.*\'"), QColor(0xe4, 0, 0x41)});
    m_rules.back().first.setMinimal(true);

    m_comment_color = QColor(0, 0x8e, 0);
    m_rules.push_back({QRegExp("--[^\n]*"), m_comment_color});
    m_comment_start = QRegExp("--\\[\\[");
    m_comment_end = QRegExp("\\]\\]");
}

//------------------------------------------------------------

void highlight_lua::highlightBlock(const QString& text)
{
    for (auto &r: m_rules)
    {
        QRegExp expression(r.first);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, r.second);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);

    int start = 0;
    if (previousBlockState() != 1)
        start = m_comment_start.indexIn(text);

    while (start >= 0)
    {
        int end = m_comment_end.indexIn(text, start);
        int len;
        if (end == -1)
        {
            setCurrentBlockState(1);
            len = text.length() - start;
        }
        else
            len = end - start + m_comment_end.matchedLength();

        setFormat(start, len, m_comment_color);
        start = m_comment_start.indexIn(text, start + len);
    }
}

//------------------------------------------------------------
