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
    void on_tree_selected();
    void on_add_tree_selected(QTreeWidgetItem*, int);
    void on_script_changed();
    void on_compile_script();

private:
    void update_objects_tree();
    void clear_mission();

private:
    scene_view *m_scene_view;
    QTabWidget *m_navigator;
    QFormLayout *m_edit_layout;
    QTreeWidget *m_objects_tree;
    QTextEdit *m_script_edit, *m_script_errors;
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
