//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QMainWindow>
#include <QTreeView>
#include <QFormLayout>

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

private:
    scene_view *m_scene_view;
    QTreeView *m_objects_tree;
    QFormLayout *m_edit_layout;
};

//------------------------------------------------------------
