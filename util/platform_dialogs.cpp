//
// open horizon -- undefined_darkness@outlook.com
//

#include "platform.h"
#include "string.h"

#ifdef QT_GUI_LIB
    #include <QMessageBox.h>
    #include <QFileDialog.h>
#elif _WIN32
    #undef APIENTRY
    #include <windows.h>
    #include <shlobj.h>
#elif __APPLE__
    #include <Cocoa/Cocoa.h>
#endif

//------------------------------------------------------------

std::string platform::open_folder_dialog()
{
#ifdef QT_GUI_LIB
    const std::string folder = QFileDialog::getExistingDirectory(Q_NULLPTR, "Specify path").toUtf8().constData();
    return folder.empty() ? folder : folder + "/";
#elif _WIN32
    BROWSEINFO bi = {0};
    const LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)
        return "";

    char path[MAX_PATH] = "";
    if (SHGetPathFromIDListA(pidl, path))
        strcat(path, "\\");
    CoTaskMemFree(pidl);
    return path;
#elif __APPLE__
    NSArray *fileTypes = [NSArray arrayWithObjects:nil];
    NSOpenPanel * panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseDirectories:YES];
    [panel setCanChooseFiles:NO];
    [panel setFloatingPanel:YES];
    NSInteger result = [panel runModalForDirectory:NSHomeDirectory() file:nil types:fileTypes] == NSOKButton;
    if(result == NSOKButton)
    {
        NSArray *urls = [panel URLs];
        if ([urls count] < 1)
            return "";

        NSURL *url = [urls objectAtIndex:0];
        return std::string([url path].UTF8String) + "/";
    }
    return "";
#else
    char path[1024];
    FILE *f = popen("zenity --file-selection --directory", "r");
    if(!f)
        return "";
    fgets(path, sizeof(path), f);
    fclose(f);
    path[strlen(path)-1] = '/';
    return path;
#endif
}

//------------------------------------------------------------

bool platform::show_msgbox(std::string message)
{
#ifdef QT_GUI_LIB
    auto m = new QMessageBox;
    m->setText(message.c_str());
    return m->exec() != QMessageBox::Cancel;
#elif _WIN32
    return MessageBoxA(0, message.c_str(), "Open Horizon", MB_OK | MB_ICONINFORMATION) == IDOK;
#elif __APPLE__
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText: [NSString stringWithUTF8String:message.c_str()]];
    [alert addButtonWithTitle: @"OK"];
    [alert addButtonWithTitle: @"Cancel"];
    return [alert runModal] == NSAlertFirstButtonReturn;
#else
    FILE *f = popen(("zenity --warning --text=\"" + message + "\"").c_str(), "r");
    if(!f)
        return false;
    fclose(f);
    return true;
#endif
}

//------------------------------------------------------------
