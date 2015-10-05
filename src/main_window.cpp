#include "main_window.h"
#include "trace_model.h"
#include "trace_painter.h"
#include "canvas.h"
#include "tools/tool.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QWidgetAction>
#include <QDockWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QWhatsThis>
#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopWidget>
#include <QMenu>

/*
inline void initMyResource()
{
    Q_INIT_RESOURCE(vis3);
}
*/

namespace vis4 {

using common::Time;

MainWindow::MainWindow()
: QMainWindow(0), currentTool(0),
  modeActions(new QActionGroup(this))

{
    //initMyResource();
}

void MainWindow::initialize(Trace_model::Ptr & model)
{
    model = restore_model(model);
    canvas = new Canvas(this);

    if (char *e = getenv("VIS3_PORTABLE_DRAWING"))
    {
        char *v = strstr(e, "=");
        if (!v || strcmp(v, "1") == 0)
            canvas->setPortableDrawing(true);
    }

    // Prevent trace drawing until window geometry restore.
    canvas->setVisible(false);

    canvas->setModel(model);
    setCentralWidget(canvas);

    // Restore window geometry
    QRect r = restore_geometry();
    if (r.isValid())
        setGeometry(r);
    else
    {
        r = QApplication::desktop()->screenGeometry();
        resize(r.width(), r.height()*80/100);
    }

    toolbar = new QToolBar(tr("Toolbar"), this);
    addToolBar(toolbar);

    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->toggleViewAction()->setEnabled(false);
    toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolbar, SIGNAL( customContextMenuRequested(const QPoint &) ),
        this, SLOT( toolbarContextMenu(const QPoint &) ));

    sidebar = new QDockWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, sidebar);
    sidebar->setWindowTitle(tr("Browse trace"));
    sidebar->setFeatures(QDockWidget::DockWidgetClosable);
    sidebar->installEventFilter(this);

    sidebarContents = new QStackedWidget(sidebar);
    sidebar->setWidget(sidebarContents);

    QAction * sidebarTrigger = new QAction(this);
    sidebarTrigger->setShortcut(Qt::Key_F9);
    sidebarTrigger->setShortcutContext(Qt::WindowShortcut);
    addAction(sidebarTrigger);

    connect(sidebarTrigger, SIGNAL( triggered(bool) ),
            this, SLOT( sidebarShowHide() ));

    xinitialize(sidebarContents, canvas);

    toolbar->addActions(modeActions->actions());

    Q_ASSERT(browser);
    browser->addToolbarActions(toolbar);

    QAction * resetView = new QAction(this);
    resetView->setShortcut(Qt::Key_Escape);
    resetView->setShortcutContext(Qt::WindowShortcut);
    addAction(resetView);

    connect(resetView, SIGNAL( triggered(bool) ),
            this, SLOT( resetView() ));

    toolbar->addSeparator();

    // Restore time unit and format
    QSettings settings;
    QString time_unit = settings.value("time_unit", Time::unit_name(0)).toString();
    for(int i = 0; i < Time::units().size(); ++i)
    {
        if (Time::unit_name(i) == time_unit)
        {
            Time::setUnit(i);
            break;
        }
    }
    if (settings.value("time_format", "separated") == "separated")
        Time::setFormat(Time::Advanced);

    // Adding freestanding tools
    foreach (QAction * action, freestandingTools)
        toolbar->addAction(action);

    actPrint = new QAction(tr("Print"), this);
    actPrint->setIcon(QIcon(":/printer.png"));
    actPrint->setShortcut(Qt::Key_P);
    actPrint->setToolTip(tr("Print"));
    actPrint->setWhatsThis(
        tr("<b>Print</b>"
           "<p>Prints the time diagram or any portion of it."));
    toolbar->addAction(actPrint);
    connect(actPrint, SIGNAL(triggered()), this, SLOT(actionPrint()));

    toolbar->addAction(QWhatsThis::createAction(this));


    addShortcuts(toolbar);

    // Restore toolbar's settings
    QString iconSize = settings.value("toolbar_icon_size", "big").toString();
    if (iconSize == "small")
        toolbar->setIconSize(QSize(16, 16));
    if (iconSize == "big")
        toolbar->setIconSize(QSize(24, 24));

    QString textPosition = settings.value("toolbar_text_position", "text beside icon").toString();
    if (textPosition == "icon only")
        toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    if (textPosition == "text only")
        toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    if (textPosition == "text beside icon")
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    if (textPosition == "text under icon")
        toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Restore active tool
    prepare_settings(settings, model);

    QString current_tool = settings.value("current_tool").toString();
    bool tool_set = false;

    if (!current_tool.isEmpty())
    {
        if (current_tool == "none")
        {
            sidebar->setVisible(false);
        }
        else if (Tool* tool = findChild<Tool*>(current_tool))
        {
            tool->action()->setChecked(true);
            activateTool(tool);
            tool_set = true;
        }
    }

    if (!tool_set)
        activateTool(browser);

    // Ugly way to move focus to canvas.
    //canvas->setModel(canvas->model());

    connect(canvas, SIGNAL(mouseEvent(QEvent*, Canvas::clickTarget,
                                        int,
                                        State_model*, const common::Time&, bool)),
            this, SLOT(mouseEvent(QEvent*, Canvas::clickTarget,
                                        int,
                                        State_model*, const common::Time&, bool)));

    connect(canvas, SIGNAL(mouseMoveEvent(QMouseEvent*,
                                            Canvas::clickTarget,
                                            int, const common::Time&)),
            this, SLOT(mouseMoveEvent(QMouseEvent*,
                                      Canvas::clickTarget,
                                      int, const common::Time&)));

    connect(canvas, SIGNAL(modelChanged(Trace_model::Ptr &)),
            this, SLOT(modelChanged(Trace_model::Ptr &)));

    // Now we can draw the trace
    canvas->setVisible(true);
}

void MainWindow::installTool(Tool* controllingWidget)
{
    QAction* action = controllingWidget->action();

    action->setCheckable(true);

    tools_list.push_back(controllingWidget);

    sidebarContents->addWidget(controllingWidget);
    modeActions->addAction(action);

    connect(action, SIGNAL(triggered(bool)),
            this, SLOT(actionTriggered()));

    connect(controllingWidget, SIGNAL(activateMe()),
            this, SLOT(activateTool()));

    connect(controllingWidget, SIGNAL(showEvent(Event_model*)),
            this, SLOT(showEvent(Event_model*)));

    connect(controllingWidget, SIGNAL(showState(State_model*)),
            this, SLOT(showState(State_model*)));


    connect(controllingWidget, SIGNAL(browse()),
            this, SLOT(browse()));

    action->setWhatsThis(controllingWidget->whatsThis());
}

void MainWindow::installBrowser(Browser* browser)
{
    installTool(browser);
    this->browser = browser;
    browse_action = browser->action();
    browse_action->setToolTip(browse_action->toolTip() + " " +
        tr("(Shortcut: <b>%1</b>)").arg("Esc"));
}

void MainWindow::installFreestandingTool(QAction * action)
{
    freestandingTools.push_back(action);
}

void MainWindow::activateTool(Tool * tool)
{
    bool first_time = (currentTool == 0);

    if (tool == 0)
        tool = static_cast<Tool*>(sender());

    if (currentTool)
        currentTool->deactivate();

    sidebarContents->setCurrentWidget(tool);
    sidebar->setWindowTitle(tool->windowTitle());

    currentTool = tool;
    tool->activate();

    if (sidebar->isVisible())
        tool->action()->setChecked(true);

    // Save current tool in settings

    QSettings settings;
    prepare_settings(settings, model());

    QString toolName = (sidebar->isVisible() || first_time) ?
        tool->objectName() : "none";
    settings.setValue("current_tool", toolName);
}

void MainWindow::addShortcuts(QWidget* w)
{
    foreach(QAction* action, w->actions())
    {
        QString tooltip = action->toolTip();
        if (!tooltip.isEmpty())
        {
            if (!action->shortcut().isEmpty())
            {
                action->setToolTip(tooltip + " " +
                                   tr("(Shortcut: <b>%1</b>)")
                                   .arg(action->shortcut().toString()));
            }
        }
    }
}

void MainWindow::prepare_settings(QSettings& settings,
                                  const Trace_model::Ptr & model) const
{
    // No per-trace settings, everything is global.
    settings.beginGroup("settings");
}

void MainWindow::save_geometry(const QRect& rect) const
{
    QSettings settings;
    prepare_settings(settings, model());
    settings.setValue("window_geometry", rect);
}

QRect MainWindow::restore_geometry() const
{
    QSettings settings;
    prepare_settings(settings, model());
    return settings.value("window_geometry").toRect();
}

void MainWindow::sidebarShowHide()
{
    sidebar->setVisible(sidebar->isHidden());
}

bool MainWindow::eventFilter(QObject * watched, QEvent * event)
{
    if (watched != sidebar)
        return QMainWindow::eventFilter(watched, event);

    if (!currentTool) return false;

    if (event->type() == QEvent::Close || event->type() == QEvent::Hide)
        currentTool->action()->setChecked(false);

    if (event->type() == QEvent::Show)
        currentTool->action()->setChecked(true);

    return false;
}

void MainWindow::moveEvent(QMoveEvent*)
{
    save_geometry(geometry());
}

void MainWindow::resizeEvent(QResizeEvent*)
{
    save_geometry(geometry());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Pass close event to canvas in order to stop background drawing
    canvas->closeEvent(event);
}

void MainWindow::actionTriggered()
{
    QAction * action = static_cast<QAction*>(sender());
    Tool * tool = qobject_cast<Tool*>(action->parent());

    sidebar->show();
    activateTool(tool);
}

void MainWindow::modelChanged(Trace_model::Ptr & m)
{
    save_model(m);

    // Disable/enable printing button
    actPrint->setEnabled(m->visible_components().size() != 0);
}

void MainWindow::showEvent(Event_model* event)
{
    browser->doShowEvent(event);
    activateTool(browser);
}

void MainWindow::showState(State_model* state)
{
    browser->doShowState(state);
    activateTool(browser);
}


void MainWindow::browse()
{
    activateTool(browser);
}

void MainWindow::actionPrint()
{
    QPrinter printer;
    printer.setOrientation(QPrinter::Landscape);
//    printer.setOutputFormat(QPrinter::PostScriptFormat);

    QPrintDialog printDialog(&printer, this->parentWidget());

    // Set default parameters
    printDialog.addEnabledOption(QAbstractPrintDialog::PrintPageRange);
    printDialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
    printDialog.setPrintRange(QAbstractPrintDialog::Selection);

    // Convert max_time from Time to int
    Trace_model::Ptr model = canvas->model();
    boost::any max_time_raw = model->root()->max_time().raw();
    long long * max_time_ll = boost::any_cast<long long>(&max_time_raw);
    if (max_time_ll)
        printDialog.setMinMax(0, (int)(*max_time_ll));
    else {
        int * max_time = boost::any_cast<int>(&max_time_raw);
        printDialog.setMinMax(0, *max_time);
    }

    // Show dialog
    if (printDialog.exec() == QDialog::Accepted)
    {
        Time min_time, max_time;
        Time timePerPage = model->max_time() - model->min_time();

        if (printDialog.printRange() == QAbstractPrintDialog::Selection) {
            min_time = model->min_time();
            max_time = model->max_time();
        } else if (printDialog.printRange() == QAbstractPrintDialog::AllPages) {
            min_time = model->root()->min_time();
            max_time = model->root()->max_time();
        } else { /* printDialog->printRange() == QAbstractPrintDialog:::PageRange */

            // Convert time interval from int to Time
            min_time = max_time = model->min_time();
            if (max_time_ll) {
                min_time = Time(min_time.setRaw(boost::any((long long)printDialog.fromPage())));
                max_time = Time(max_time.setRaw(boost::any((long long)printDialog.toPage())));
            } else {
                min_time = Time(min_time.setRaw(boost::any((int)printDialog.fromPage())));
                max_time = Time(max_time.setRaw(boost::any((int)printDialog.toPage())));
            }
        }

        model = model->set_range(min_time, max_time);

        //qDebug() << "Trace model for print:" << "min time:" << min_time.toString() << "max time:" << max_time.toString();

        Trace_painter tp;
        tp.setModel(model);
        tp.setPaintDevice(&printer);

        QApplication::setOverrideCursor(Qt::WaitCursor);
        tp.drawTrace(timePerPage, true /* start in background */);
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::save_model(Trace_model::Ptr & m)
{
    QSettings settings;
    prepare_settings(settings, m);
    settings.setValue("trace_state", m->save());
}

Trace_model::Ptr & MainWindow::restore_model(Trace_model::Ptr & model)
{
    QSettings settings;
    prepare_settings(settings, model);
    QString model_desc = settings.value("trace_state").toString();
    if (!model_desc.isEmpty())
    {
        model->restore(model_desc);
    }
    return model;
}

Trace_model::Ptr MainWindow::model() const
{
    return canvas->model();
}


void MainWindow::mouseEvent(QEvent* event,
                    Canvas::clickTarget target,
                    int component,
                    State_model* state,
                    const Time& time,
                    bool events_near)
{
    bool done = currentTool->mouseEvent(event, target, component, state, time, events_near);
    if (!done)
    {
        for (int i = 0; i < tools_list.size(); ++i)
        {
            Tool* t = tools_list[i];
            if (t != currentTool)
                t->mouseEvent(event, target, component, state, time, events_near);
        }
    }
}

 void MainWindow::mouseMoveEvent(QMouseEvent* event, Canvas::clickTarget target,
                        int component, const Time& time)
{
    bool done = currentTool->mouseMoveEvent(event, target, component, time);
    if (!done)
    {
        for (int i = 0; i < tools_list.size(); ++i)
        {
            Tool* t = tools_list[i];
            if (t != currentTool)
                t->mouseMoveEvent(event, target, component, time);
        }
    }
}

void MainWindow::toolbarContextMenu(const QPoint & pos)
{
    QMenu * menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(menu, SIGNAL( triggered(QAction *) ),
        this, SLOT( toolbarSettingsChanged(QAction *) ));

    // Adding nice menu title
    QLabel * title = new QLabel(tr("Toolbar menu"), menu);
    title->setFrameStyle(QFrame::Panel);
    title->setFrameShadow(QFrame::Raised);
    title->setMargin(4);
    title->setAlignment(Qt::AlignCenter);
    QFont boldFont(title->font()); boldFont.setBold(true);
    title->setFont(boldFont);

    QWidgetAction * titleAction = new QWidgetAction(menu);
    titleAction->setDefaultWidget(title);
    titleAction->setDisabled(true);
    menu->addAction(titleAction);

    QAction * action;

    // Adding icon size menu
    QMenu * iconsMenu = menu->addMenu(tr("Size of icons"));

    action = iconsMenu->addAction(tr("Small icons"));
    action->setObjectName("act_smallIcons");
    action->setCheckable(true);
    action->setChecked(toolbar->iconSize().width() == 16);

    action = iconsMenu->addAction(tr("Big icons"));
    action->setObjectName("act_bigIcons");
    action->setCheckable(true);
    action->setChecked(toolbar->iconSize().width() == 24);

    // Adding text position menu
    QMenu * textposMenu = menu->addMenu(tr("Position of text"));

    action = textposMenu->addAction(tr("Icon only"));
    action->setObjectName("act_iconOnly");
    action->setCheckable(true);
    action->setChecked(toolbar->toolButtonStyle() == Qt::ToolButtonIconOnly);

    action = textposMenu->addAction(tr("Text only"));
    action->setObjectName("act_textOnly");
    action->setCheckable(true);
    action->setChecked(toolbar->toolButtonStyle() == Qt::ToolButtonTextOnly);

    action = textposMenu->addAction(tr("Text beside icon"));
    action->setObjectName("act_textBeside");
    action->setCheckable(true);
    action->setChecked(toolbar->toolButtonStyle() == Qt::ToolButtonTextBesideIcon);

    action = textposMenu->addAction(tr("Text under icon"));
    action->setObjectName("act_textUnder");
    action->setCheckable(true);
    action->setChecked(toolbar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);

    menu->popup(toolbar->mapToGlobal(pos));
}

void MainWindow::toolbarSettingsChanged(QAction * action)
{
    QSettings settings;

    if (action->objectName() == "act_smallIcons") {
        toolbar->setIconSize(QSize(16, 16));
        settings.setValue("toolbar_icon_size", "small");
    }
    if (action->objectName() == "act_bigIcons") {
        toolbar->setIconSize(QSize(24, 24));
        settings.setValue("toolbar_icon_size", "big");
    }

    if (action->objectName() == "act_iconOnly") {
        toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        settings.setValue("toolbar_text_position", "icon only");
    }
    if (action->objectName() == "act_textOnly") {
        toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
        settings.setValue("toolbar_text_position", "text only");
    }
    if (action->objectName() == "act_textBeside") {
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        settings.setValue("toolbar_text_position", "text beside icon");
    }
    if (action->objectName() == "act_textUnder") {
        toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        settings.setValue("toolbar_text_position", "text under icon");
    }
}

void MainWindow::resetView()
{
    for (int i = 0; i < tools_list.size(); ++i)
        tools_list[i]->reset();
    activateTool(browser);
    canvas->setFocus(Qt::OtherFocusReason);
}

}
