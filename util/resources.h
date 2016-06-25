//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "containers/qdf_provider.h"
#include "containers/dpl_provider.h"

#include "util/config.h"
#include "util/platform.h"

#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"

#include "system/system.h"

#ifndef _WIN32
    #include <unistd.h>
#endif

#ifdef QT_GUI_LIB
    #include <QMessageBox.h>
    #include <QFileDialog.h>
#endif

//------------------------------------------------------------

static bool setup_resources()
{
#ifndef _WIN32
    chdir(nya_system::get_app_path());
#endif

    config::register_var("acah_path", "");

    static qdf_resources_provider qdfp;
    if (!qdfp.open_archive((config::get_var("acah_path") + "datafile.qdf").c_str()))
    {
        static const char message[] = "You are running Open Horizon outside of the Assault Horizon folder.\n"
                                      "Please specify the path to the Assault Horizon folder.\n"
                                      "It will be saved automatically.";
#ifdef QT_GUI_LIB
        auto m = new QMessageBox;
        m->setText(message);
        if (m->exec() != QMessageBox::Cancel)
#else
        if (platform::show_msgbox(message))
#endif
        {
#ifdef QT_GUI_LIB
            std::string folder = QFileDialog::getExistingDirectory(Q_NULLPTR, "Specify path").toUtf8().constData();
            folder.append("/");
#else
            std::string folder = platform::open_folder_dialog();
#endif
            if (!folder.length())
                return false;

            config::set_var("acah_path", folder);

            if (!qdfp.open_archive((config::get_var("acah_path") + "datafile.qdf").c_str()))
                return false;
        }
        else
            return false;
    }

    class target_resource_provider: public nya_resources::resources_provider
    {
        nya_resources::resources_provider &m_provider;
        dpl_resources_provider m_dlc_provider;
        nya_resources::file_resources_provider m_fprov;
        nya_resources::file_resources_provider m_fprov2;

    public:
        target_resource_provider(nya_resources::resources_provider &provider): m_provider(provider)
        {
            m_fprov.set_folder(nya_system::get_app_path());
            m_fprov2.set_folder(config::get_var("acah_path").c_str());

            nya_resources::composite_resources_provider cprov;
            cprov.add_provider(&m_fprov);
            cprov.add_provider(&m_fprov2);
            cprov.add_provider(&provider);
            nya_resources::set_resources_provider(&cprov);

            m_dlc_provider.open_archive("target/DATA.PAC", "DATA.PAC.xml");
        }

    private:
        nya_resources::resource_data *access(const char *resource_name)
        {
            if (!resource_name)
                return 0;

            if (m_dlc_provider.has(resource_name))
                return m_dlc_provider.access(resource_name);

            const std::string str(resource_name);
            if (m_provider.has(("target/" + str).c_str()))
                return m_provider.access(("target/" + str).c_str());

            if (m_provider.has(("common/" + str).c_str()))
                return m_provider.access(("common/" + str).c_str());

            if (m_provider.has(resource_name))
                return m_provider.access(resource_name);

            if (m_fprov.has(resource_name))
                return m_fprov.access(resource_name);

            return m_fprov2.access(resource_name);
        }

        bool has(const char *resource_name)
        {
            if (!resource_name)
                return false;

            const std::string str(resource_name);
            return m_provider.has(("target/" + str).c_str()) || m_provider.has(("common/" + str).c_str()) || m_provider.has(resource_name);
        }
        
    } static trp(qdfp);

    nya_resources::set_resources_provider(&trp);
    return true;
}

//------------------------------------------------------------
