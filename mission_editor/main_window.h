//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QMainWindow>
#include <QTreeView>

//------------------------------------------------------------

class scene_view;

//------------------------------------------------------------

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = 0);

private:
    scene_view *m_scene_view;
    QTreeView *m_objects_tree;
};

//------------------------------------------------------------
