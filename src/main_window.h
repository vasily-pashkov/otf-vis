#ifndef MAIN_WINDOW_HPP_VP_20060719
#define MAIN_WINDOW_HPP_VP_20060719

#include "trace_model.h"
#include "canvas.h"

#include <QMap>
#include <QMainWindow>
#include <QSettings>

class QAction;
class QWidget;
class QMoveEvent;
class QResizeEvent;
class QActionGroup;
class QToolBar;
class QDocWidget;
class QStackedWidget;

namespace vis4 {

class Trace_model;
class Event_model;
class Tool;
class Browser;


/** Основное окно приложения.
    Содержит панель инструментов, изображение временной диаграммы, и набор
    пользовательских инструментов. Инструменты деляться на "управляемые" и
    независимые.

    Управляемые элементы показываются в правом sidebar, и в каждый
    момент показывается только один инструмент. На панели инструментов есть набор
    кнопок, переключающих между управляемыми инструментами.

    Независимые инструмент -- это кнопка в отдельной области панели инструментов. Обработка
    нажатия на это кнопку задается автором инструмента. Например, может появляться дополнительное
    окно с графиками.
*/
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /** Creates useless instance. Call to initialize must be made
        to bring this object to life. */
    MainWindow();

    /** Performs the real initialization. We want to allow derived
        classes to change the set of tools by overriding 'initialize'
        but we can't call virtual methods from constructor. So,
        this method does real initialization, and will be called from
        'show', below. */
    void initialize(Trace_model::Ptr & model);

    /** Preparing current path in QSettings to store settings per trace. */
    virtual void prepare_settings(QSettings& settings,
                                  const Trace_model::Ptr & model) const;

protected:
    /** Добавляет controllingWindget к списку инструментов, доступных пользователю
    в sidebar. */
    void installTool(Tool* tool);

    /** Добавляет инструмент навигации к списку инструментов, доступных пользователю
    в sidebar.
    @sa Browser
    */
    void installBrowser(Browser* browser);

    /** Добавляет QAction для независимого инструмента к панели инструментов. Поведение
    при активации QAction определяется вызывающей стороной. */
    void installFreestandingTool(QAction*);

    /** Event filter is used to catch sidebar events. */
    bool eventFilter(QObject * watched, QEvent * event);

private:

    /** Данная функция создает набор используемых инструментов с
    помощью вызовов функций installTool, installBrowser и
    installFreestandingTool. */
    virtual void xinitialize(QWidget* toolContainer, Canvas* canvas) = 0;

    void addShortcuts(QWidget* w);
    void moveEvent(QMoveEvent*);
    void resizeEvent(QResizeEvent*);
    void closeEvent(QCloseEvent *);

    virtual void save_model(Trace_model::Ptr & m);
    virtual Trace_model::Ptr &
    restore_model(Trace_model::Ptr & m);

    virtual void save_geometry(const QRect& rect) const;
    virtual QRect restore_geometry() const;

private slots:

    void activateTool(Tool * tool = 0);

    void actionTriggered();

    void sidebarShowHide();

    void modelChanged(Trace_model::Ptr &);

    void showEvent(Event_model* event);

    void showState(State_model* state);

    void browse();

    /** Print trace on printer. */
    void actionPrint();

    void mouseEvent(QEvent* event,
                    Canvas::clickTarget target,
                    int component,
                    State_model* state,
                    const common::Time& time,
                    bool events_near);

    void mouseMoveEvent(QMouseEvent* event, Canvas::clickTarget target,
                        int component, const common::Time& time);

    // Shows context menu with toolbar's settings
    void toolbarContextMenu(const QPoint & pos);

    // Handles context menu with toolbar's settings
    void toolbarSettingsChanged(QAction * action);

    /** Resets all tools, activate browser and set focus to trace. */
    void resetView();

protected:

    Trace_model::Ptr model() const;

private:

    Canvas* canvas;
    Tool* currentTool;
    QActionGroup* modeActions;
    QAction* browse_action;
    Browser* browser;
    QToolBar* toolbar;
    QDockWidget* sidebar;
    QStackedWidget* sidebarContents;

    QAction * actPrint;

    QList<QAction*> freestandingTools;
    QVector<Tool*> tools_list;
};

}
#endif

