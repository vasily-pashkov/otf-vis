#ifndef OTF_MAIN_WINDOW_H
#define OTF_MAIN_WINDOW_H

#include "tools/tool.h"
#include "main_window.h"

namespace vis4 {

    using namespace common;

    class OTF_main_window : public MainWindow
    {
    public:
        OTF_main_window()
        {
            setWindowTitle("Vis4");
        }

    private:
        void xinitialize(QWidget* toolContainer, Canvas* canvas)
        {

            Browser* browser = createBrowser(toolContainer, canvas);
            installBrowser(browser);
/*
            installTool(createGoto(toolContainer, canvas));
            installTool(createMeasure(toolContainer, canvas));
            installTool(createFilter(toolContainer, canvas));
            Tool* find = createFind(toolContainer, canvas);
            installTool(find);
            connect(find, SIGNAL(extraHelp(const QString&)),
                    browser, SLOT(extraHelp(const QString&)));
                    */
        }
    };

}

#endif // OTF_MAIN_WINDOW_H
