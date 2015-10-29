#ifndef TOOL_HPP_VP_2006_04_03
#define TOOL_HPP_VP_2006_04_03

#include "canvas.h"

#include <QSettings>
#include <QWidget>

#include <boost/shared_ptr.hpp>

class QAction;
class QToolBar;

namespace vis4 {

class Trace_model;
class Event_model;
class MainWindow;

/** Базовый класс для инструментов, размещаемых в левом sidebar.

   В каждый момент времени видим только один инструмент. Toolbar
   содержит отдельную область в кнопками, переключающими видимый
   инструмент.

   Класс Tool наследован от QWidget и непосредственно показывается в
   левом sidebar.
*/
class Tool : public QWidget
{
    Q_OBJECT
public:
    Tool(QWidget* parent, Canvas* canvas);

    /** Возвращает объект QAction, который помещается на toolbar. */
    QAction* action();

    /** Данный метод вызывается в момент активизации инструмента,
        когда он становится видимым на sidebar. */
    virtual void activate() = 0;

    /** Данный метод вызывается в момент деактивизации инструмента,
    когда он становится невидимым на sidebar. */
    virtual void deactivate() = 0;

    /** Reset tool to initial state. */
    virtual void reset() {}

    /**
     *  Возвращает канву, на которой данный инструмент может рисовать.
     */
    Canvas* canvas() const { return canvas_; }

    /** Метод вызывается при нажатии кнопок миши на временной диаграммы.
        Сначала метод вызывается у текущего инструмента. Если при этом
        возвращается true, то обработка завершается. В противном случае
        вызываются методы всех имеющихся инструментов. Порядок вызова не
        определен и возвращаемое значение игнорируется.  */
    virtual bool mouseEvent(QEvent* event,
                          Canvas::clickTarget target,
                          int component,
                          State_model* state,
                          const common::Time& time,
                          bool events_near) { return false; }

    /** Метод вызывается при перемещении миши на временной диаграмме.
        Обработка такая же как и у canvasMouseEvent.  */
    virtual bool mouseMoveEvent(QMouseEvent* ev, Canvas::clickTarget target,
                                int component, const common::Time& time) { return false; }

signals:

    /** This signal is emitted when tool wants to be activated.
        This must be the only way to activate itself.
        __DO NOT CALL trigger() METHOD OF SELF ACTION__ */
    void activateMe();

    /**
     * Данный сигнал может использоваться дочерними классами для того,
     * чтобы показать подробную информацию о событии event. Переключение
     * инструментов при этом происходит автоматически.
     */
    void showEvent(Event_model* event);

    void showState(State_model* state);

    /**
     *  Данный сигнал может использоваться дочерними классами для переключания
     *  на иструмент "общего обзора трассы".
     */
    void browse();

protected:

    /** Preparing current path in QSettings to store settings per trace. */
    void prepare_settings(QSettings & settings) const;

    /**
     *  Возвращает трассу, с которой работает данный инструмент.
     */
    Trace_model::Ptr & model() const;

    /** Prevents handle keys like arrows as shortcut.  */
    bool event(QEvent * event);

private:
    /** Функция должна создать объект Action, который будет помещен
        на панель инструментов. Объект должен иметь заголовок  иконку,
        и имя объекта (objectName). Сигналы объекта никуда присоединяться
        не должны. */
    virtual QAction* createAction() = 0;

private:
    Canvas* canvas_;
    MainWindow * mainWnd_;
    QAction* action_;
};

/** Инструмент, предназначенные для навигации по временной диграмме. Один из инструментов
 всегда должен быть унаследовано от этого типа, и добавлен к MainWindow вызовом
метода MainWindow::installBrowser.
Реализует переходы вниз в вверх, движение по времени влево и вправо, изменение масштаба,
и показ списка событий и детальной информации по щелчку.
*/
class Browser : public Tool
{
    Q_OBJECT
public:
    Browser(QWidget* parent, Canvas* c);

    /** Вызывается классом MainWindow. Должен добавить действия, необходимые
    для навигации, в конец toolbar. */
    virtual void addToolbarActions(QToolBar* toolbar) = 0;
    /** При вызове должен показать детальную информацию о событии event. */
    virtual void doShowEvent(Event_model* event) = 0;

    virtual void doShowState(State_model* state) = 0;

};




Browser* createBrowser(QWidget* parent, Canvas* canvas);
Tool* createGoto(QWidget* parent, Canvas* canvas);
Tool* createMeasure(QWidget* parent, Canvas* canvas);
Tool* createFilter(QWidget* parent, Canvas* canvas);
Tool* createFind(QWidget* parent, Canvas* canvas);



}

#endif
