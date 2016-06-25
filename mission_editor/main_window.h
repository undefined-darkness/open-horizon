//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QtWidgets>

//------------------------------------------------------------

class scene_view;

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
    void on_active_changed(int state);
    void on_align_changed(int state);
    void on_init_changed(const QString &s);
    void on_destroy_changed(const QString &s);
    void on_zone_enter_changed(const QString &s);
    void on_zone_leave_changed(const QString &s);
    void on_script_changed();
    void on_compile_script();

private:
    void update_objects_tree();
    void set_selection(std::string, int idx);
    void clear_mission();

private:
    scene_view *m_scene_view;
    QTabWidget *m_navigator;
    QStackedWidget *m_edit;
    QLineEdit *m_edit_obj_name, *m_edit_path_name, *m_edit_zone_name;
    QCheckBox *m_edit_obj_active, *m_edit_zone_active;
    QComboBox *m_edit_obj_align;
    QLineEdit *m_edit_obj_init, *m_edit_obj_destroy;
    QLineEdit *m_edit_zone_enter, *m_edit_zone_leave;
    QTreeWidget *m_objects_tree;
    QTextEdit *m_script_edit, *m_script_errors;
    QLineEdit *m_mission_title, *m_mission_author, *m_mission_email;
    QTextEdit *m_mission_description;
    QTimer *m_compile_timer;
    std::string m_location;
    std::string m_filename;
};

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
