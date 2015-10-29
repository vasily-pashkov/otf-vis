#ifndef _FIND_TABS_HPP_
#define _FIND_TABS_HPP_

#include "checker.h"
#include "tool.h"

#include <selection.h>

#include <QWidget>
#include <QSettings>

#include <boost/shared_ptr.hpp>

class QComboBox;
class QStackedLayout;

namespace vis4 {

class Trace_model;
class Event_model;
class State_model;

namespace common {
    class SelectionWidget;
}

/** Base class for find tabs. */
class FindTab : public QWidget {

    Q_OBJECT

public: /* methods */

    FindTab(Tool * find_tool)
        : QWidget(find_tool), find_tool_(find_tool) {}

    virtual QString name() = 0;

    virtual void reset() = 0;

    virtual bool findNext() = 0;

    virtual void setModel(Trace_model::Ptr &) = 0;

    virtual bool isSearchAllowed() = 0;

    virtual void saveState(QSettings & settings) = 0;

    virtual void restoreState(QSettings & settings) = 0;

signals:

    void stateChanged();

protected:

    Tool * find_tool_;

};

/** Class for events find tab. */
class FindEventsTab : public FindTab {

    Q_OBJECT

public: /* methods */

    FindEventsTab(Tool * find_tool);

    QString name()
        { return tr("Events"); }

    void reset();

    bool findNext();

    void setModel(Trace_model::Ptr & model);

    bool isSearchAllowed();

    void saveState(QSettings & settings);

    void restoreState(QSettings & settings);

signals:

    void showEvent(Event_model *);

private slots:

    void selectionChanged(const vis4::common::Selection & selection);

private: /* widgets */

    common::SelectionWidget * selector_;
    Trace_model::Ptr model_;
    Trace_model::Ptr filtered_model_;

};

/** Class for states find tab. */
class FindStatesTab : public FindTab {

    Q_OBJECT

public: /* methods */

    FindStatesTab(Tool * find_tool);

    QString name()
        { return tr("States"); }

    void reset();

    bool findNext();

    void setModel(Trace_model::Ptr & model);

    bool isSearchAllowed();

    void saveState(QSettings & settings);

    void restoreState(QSettings & settings);

signals:

    void showState(State_model *);

private slots:

    void selectionChanged(const vis4::common::Selection & selection);

private: /* widgets */

    common::SelectionWidget * selector_;
    Trace_model::Ptr model_;
    Trace_model::Ptr filtered_model_;

};

/** Class for checkers find tab. */
class FindQueryTab : public FindTab {

    Q_OBJECT

public: /* methods */

    FindQueryTab(Tool * find_tool);

    QString name()
        { return tr("Query"); }

    void reset();

    bool findNext();

    void setModel(Trace_model::Ptr & model);

    bool isSearchAllowed();

    void saveState(QSettings & settings);

    void restoreState(QSettings & settings);

private: /* methods */

    /** Creates checkers and ininialize event/subevent lists. */
    void initializeCheckers();

signals:

    void showEvent(Event_model *);

private slots:

    void updateChecker();

    void activateCheckerEvents();

    void checkerStateChanged();

private: /* widgets */

    Trace_model::Ptr model_;
    Trace_model::Ptr model_with_checker;

    QList<pChecker> checkers;
    Checker * active_checker;
    bool active_checker_is_ready;

    QComboBox * checkerCombo;
    QStackedLayout * checkerWidgetContainer;
    QLayout * checkerSettings_layout;

};

}
#endif
