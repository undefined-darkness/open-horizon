//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game/script.h"
#include <QtWidgets>

//------------------------------------------------------------

class scene_view;
class function_edit;
class objects_tree;

//------------------------------------------------------------

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = 0);

private:
    void setup_menu();

private slots:
    void on_new_mission();
    void on_load_mission();
    void on_save_mission();
    void on_save_as_mission();
    void on_mode_changed(int idx);
    void on_delete();
    void on_obj_selected();
    void on_obj_focus(QTreeWidgetItem *, int);
    void on_add_tree_selected(QTreeWidgetItem*, int);
    void on_name_changed(const QString &s);
    void on_obj_follow_changed(const QString &s);
    void on_obj_target_changed(const QString &s);
    void on_obj_path_changed(const QString &s);
    void on_active_changed(int state);
    void on_align_changed(int state);
    void on_zone_display_changed(int state);
    void on_path_loop_changed(int state);
    void on_script_changed();
    void on_compile_script();

private:
    void update_objects_tree();
    void set_selection(std::string, int idx);
    void clear_mission();

public:
    bool has_script_function(std::string function);
    void update_attribute(std::string id, std::string value);
    void goto_script_function(std::string function);
    void reorder_objects(std::string group, std::vector<int> from, int to);

private:
    scene_view *m_scene_view;
    QTabWidget *m_navigator;
    QStackedWidget *m_edit;
    QLineEdit *m_edit_obj_name, *m_edit_path_name, *m_edit_zone_name;
    QCheckBox *m_edit_obj_active, *m_edit_path_loop, *m_edit_zone_active;
    QComboBox *m_edit_obj_align;
    QLineEdit *m_edit_obj_follow, *m_edit_obj_target, *m_edit_obj_path;
    function_edit *m_edit_obj_init, *m_edit_obj_destroy;
    function_edit *m_edit_zone_enter, *m_edit_zone_leave;
    QComboBox *m_edit_zone_display;
    objects_tree *m_objects_tree;
    QTextEdit *m_script_edit, *m_script_errors;
    QLineEdit *m_mission_title, *m_mission_author, *m_mission_email;
    QTextEdit *m_mission_description;
    QTimer *m_compile_timer;
    std::string m_location;
    std::string m_filename;
    game::script m_script;
};

//------------------------------------------------------------

inline std::string to_str(const QString &s) { return s.toUtf8().constData(); }

//------------------------------------------------------------

class highlight_lua : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit highlight_lua(QTextDocument *parent);

private:
    void highlightBlock(const QString &text) override;
    std::vector<std::pair<QRegExp, QColor> > m_rules;
    QColor m_comment_color;
    QRegExp m_comment_start, m_comment_end;
};

//------------------------------------------------------------

class function_edit: public QLineEdit
{
    Q_OBJECT

public:
    explicit function_edit(main_window *m, std::string id);

private:
    void keyPressEvent(QKeyEvent *event);
    main_window *m_window;
    std::string m_id;

private slots:
    void on_changed(const QString &s);
};

//------------------------------------------------------------

class objects_tree: public QTreeWidget
{
public:
    objects_tree(main_window *parent): m_parent(parent) {}

private:
  virtual void dropEvent(QDropEvent * event);

private:
    main_window *m_parent;
};

//------------------------------------------------------------
