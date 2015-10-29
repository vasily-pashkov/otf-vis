#include "tool.h"

#include "trace_model.h"
#include "event_model.h"
#include "state_model.h"
#include "canvas.h"
#include "event_list.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QStackedWidget>
#include <QAction>
#include <QMouseEvent>
#include <QStack>
#include <QShortcut>
#include <QKeySequence>
#include <QCoreApplication>
#include <QSettings>

namespace vis4 {

using common::Time;
using common::Selection;

class Browser_trace_info : public QGroupBox
{
    Q_OBJECT
public:
    Browser_trace_info(QWidget* parent)
    : QGroupBox(tr("Trace information"), parent)
    {
        QVBoxLayout* infoLayout = new QVBoxLayout(this);
        QPalette grayLinePalette = QLineEdit().palette();
        grayLinePalette.setColor(QPalette::Active, QPalette::Base,  Qt::transparent);
        grayLinePalette.setColor(QPalette::Disabled, QPalette::Base,  Qt::transparent);
        grayLinePalette.setColor(QPalette::Inactive, QPalette::Base,  Qt::transparent);
        infoLayout->addWidget(new QLabel(tr("Start:"), this));
        startTimeLabel = new QLineEdit(this);
        startTimeLabel->setPalette(grayLinePalette);
        startTimeLabel->setReadOnly(true);
        startTimeLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(startTimeLabel);

        infoLayout->addWidget(new QLabel(tr("End:"), this));
        endTimeLabel = new QLineEdit(this);
        endTimeLabel->setPalette(grayLinePalette);
        endTimeLabel->setReadOnly(true);
        endTimeLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(endTimeLabel);

        infoLayout->addWidget(new QLabel(tr("Components:"), this));
        componentsLabel = new QLineEdit(this);
        componentsLabel->setPalette(grayLinePalette);
        componentsLabel->setReadOnly(true);
        componentsLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(componentsLabel);

        infoLayout->addWidget(new QLabel(tr("Event types:"), this));
        eventsLabel = new QLineEdit(this);
        eventsLabel->setPalette(grayLinePalette);
        eventsLabel->setReadOnly(true);
        eventsLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(eventsLabel);

        infoLayout->addStretch();

#if 0
        QToolButton* copy = new QToolButton(this);
        copy->setIconSize(QSize(32, 32));
        copy->setToolTip("Copy trace info to clipboard");
        copy->setIcon(QIcon(":/editcopy.png"));
        infoLayout->addWidget(copy, 4, 0);
#endif

    }

    void update(Canvas* canvas)
    {
        Trace_model::Ptr model = canvas->model();

        startTimeLabel->setText(model->min_time().toString(true));
        endTimeLabel->setText(model->max_time().toString(true));

        const Selection & comp_selection = model->components();
        int parent_component = model->parent_component();

        int visible_components = comp_selection.enabledCount(parent_component);
        int hidden_components = comp_selection.itemsCount(parent_component) - visible_components;

        QString components = QString::number(visible_components);
        if (hidden_components)
        {
            components += " " + tr("(%1 more hidden)").arg(hidden_components);
        }

        componentsLabel->setText(components);

        int visible_events = model->events().enabledCount();
        int hidden_events = model->events().itemsCount() - visible_events;

        QString events = QString::number(visible_events);
        if (hidden_events)
        {
            events += " " + tr("(%1 more hidden)").arg(hidden_events);
        }
        eventsLabel->setText(events);
    }



private:
    QLineEdit * startTimeLabel;
    QLineEdit * endTimeLabel;
    QLineEdit * componentsLabel;
    QLineEdit * eventsLabel;

};


class Browser_event_info : public QGroupBox
{
    Q_OBJECT
public:
    Browser_event_info(QWidget* parent)
    : QGroupBox(tr("Event information"), parent),
      nameLabel(0), using_default_details(false)
    {
        mainLayout = new QVBoxLayout(this);
        splitter = new QSplitter(Qt::Vertical ,this);

        eventList = new Event_list(this);
        /* Qt 4.2 seem to have a bug -- if details widget has
           QGridLayout and one row if it is expanding, then
           size policies on event list and details widget don't work
           -- both widgets get equal size.  */
        splitter->addWidget(eventList);
        connect(eventList, SIGNAL(currentEventChanged(Event_model*)),
                this, SLOT(currentEventChanged(Event_model*)));

        details = new QWidget(this);
        splitter->addWidget(details);

        mainLayout->addWidget(splitter);
        connect(splitter, SIGNAL( splitterMoved(int, int) ),
            this, SLOT( slotSplitterMoved() ));

        QList<int> s = restore_splitter_state();
        if (!s.isEmpty())
            splitter->setSizes(s);
    }

    void showEvent(Canvas * canvas, Event_model* event)
    {
        // We block signals to permit flicker effect
        eventList->blockSignals(true);
        update(canvas, event->component, event->time);
        eventList->setCurrentEvent(event);
        eventList->blockSignals(false);

        // Since eventList was blocked we have to update event details manually
        currentEventChanged(event);
    }

    void update(Canvas * canvas, int component, const Time & time)
    {
        trace_ = canvas->model();
        int parent_component = trace_->parent_component();

        Selection component_filter = trace_->components();
        component = trace_->visible_components()[trace_->lifeline(component)];
        component_filter.disableAll(parent_component);
        component_filter.setEnabled(component, true);

        QPair<Time, Time> nearby = canvas->nearby_range(time);
        filtered_ = trace_->set_range(nearby.first, nearby.second);
        filtered_ = filtered_->filter_components(component_filter);

        filtered_->rewind();

        eventList->showEvents(filtered_, time);

        if (eventList->model()->rowCount() == 1)
            eventList->hide();
        else {
            eventList->show();
            eventList->setFocus(Qt::OtherFocusReason);
        }

    }

    void updateTime()
    {
        eventList->updateTime();
        currentEventChanged(eventList->currentEvent());
    }

private slots:

    void currentEventChanged(Event_model* event)
    {
        if (!event)
            { details->hide(); return; }

        if (event->customDetailsWidget())
        {
            if (using_default_details)
            {
                deleteAllChildren(details);
                nameLabel = 0;
            }
            event->createDetailsWidget(details);
            using_default_details = false;
        }
        else
        {
            if (!using_default_details)
            {
                deleteAllChildren(details);
                nameLabel = 0;
            }

            if (!nameLabel)
            {
                QGridLayout* infoLayout = new QGridLayout(details);
                infoLayout->setMargin(0);

                infoLayout->addWidget(new QLabel(tr("Type:"), details), 0, 0);
                nameLabel = new QLabel("", details);
                infoLayout->addWidget(nameLabel, 0, 1);

                infoLayout->addWidget(new QLabel(tr("Time:"), details), 1, 0);
                timeLabel = new QLabel("", details);
                infoLayout->addWidget(timeLabel, 1, 1);

                infoLayout->setRowStretch(2, 1);
            }

            nameLabel->setText(event->kind);
            timeLabel->setText(event->time.toString(true));
            using_default_details = true;
        }

        /* Expand the minimum size of details widget to the current
           size hint, so that it never shrinks below that size.
           This is needed to prevent constant shrink and grow
           when traversing the event list.  */
        details->updateGeometry();
        eventList->updateGeometry();
        QCoreApplication::sendPostedEvents(0, 0);
        QSize ds = details->sizeHint();
        details->setMinimumSize(ds.expandedTo(details->minimumSize()));
        details->show();
    }

    void slotSplitterMoved()
    {
        save_splitter_state(splitter->sizes());
    }

private:

    void deleteAllChildren(QWidget* w)
    {
        const QObjectList& obj = w->children();
        for (int i = 0; i < obj.size(); ++i)
            delete obj[i];
    }

    void save_splitter_state(const QList<int> & sizes) const
    {
        QList<QVariant> s;
        QListIterator<int> it(sizes);
        while (it.hasNext())
            s << it.next();

        QSettings settings;
        settings.beginGroup("browser_tool");
        settings.setValue("event_info_geometry", s);
    }

    QList<int> restore_splitter_state() const
    {
        QSettings settings;
        settings.beginGroup("browser_tool");

        QList<int> sizes;
        QListIterator<QVariant> it(settings.value("event_info_geometry").toList());
        while (it.hasNext())
            sizes << it.next().toInt();

        return sizes;
    }

    QVBoxLayout* mainLayout;
    QSplitter* splitter;

    QLabel* nameLabel;
    QLabel* timeLabel;
    Event_list* eventList;
    QWidget* details;
    bool using_default_details;

    Trace_model::Ptr trace_;
    Trace_model::Ptr filtered_;
};



class Browser_state_info : public QGroupBox
{
    Q_OBJECT
public:
    Browser_state_info(QWidget* parent)
    : QGroupBox(tr("State information"), parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout* infoLayout = new QVBoxLayout(this);

        QPalette grayLinePalette = QLineEdit().palette();
        grayLinePalette.setColor(QPalette::Active, QPalette::Base,  Qt::transparent);
        grayLinePalette.setColor(QPalette::Inactive, QPalette::Base,  Qt::transparent);
        grayLinePalette.setColor(QPalette::Disabled, QPalette::Base,  Qt::transparent);

        infoLayout->addWidget(new QLabel(tr("Name:"), this));
        nameLabel = new QLineEdit(this);
        nameLabel->setPalette(grayLinePalette);
        nameLabel->setReadOnly(true);
        nameLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(nameLabel);


        infoLayout->addWidget(new QLabel(tr("Start:"), this));
        startTimeLabel = new QLineEdit(this);
        startTimeLabel->setPalette(grayLinePalette);
        startTimeLabel->setReadOnly(true);
        startTimeLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(startTimeLabel);

        infoLayout->addWidget(new QLabel(tr("End:"), this));
        endTimeLabel = new QLineEdit(this);
        endTimeLabel->setPalette(grayLinePalette);
        endTimeLabel->setReadOnly(true);
        endTimeLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(endTimeLabel);

        infoLayout->addWidget(new QLabel(tr("Duration:"), this));
        durationLabel = new QLineEdit(this);
        durationLabel->setPalette(grayLinePalette);
        durationLabel->setReadOnly(true);
        durationLabel->setFocusPolicy(Qt::NoFocus);
        infoLayout->addWidget(durationLabel);

        infoLayout->addStretch();
    }

    void update(Trace_model::Ptr model, State_model* state)
    {
        startTimeLabel->setText(state->begin.toString());
        endTimeLabel->setText(state->end.toString());
        nameLabel->setText(model->states().item(state->type));

        durationLabel->setText((state->end - state->begin).toString());

        state_begin = state->begin;
        state_end = state->end;
    }

    void updateTime()
    {
        startTimeLabel->setText(state_begin.toString(true));
        endTimeLabel->setText(state_end.toString(true));
        durationLabel->setText((state_end - state_begin).toString(true));
    }

private:
    QLineEdit * nameLabel;
    QLineEdit * startTimeLabel;
    QLineEdit * endTimeLabel;
    QLineEdit * durationLabel;

    Time state_begin;
    Time state_end;
};


Browser::Browser(QWidget* parent, Canvas* c)
: Tool(parent, c)
{}


class BrowserReal : public Browser
{
    Q_OBJECT
public:
    BrowserReal(QWidget* parent, Canvas* c)
    : Browser(parent, c), active_(false)
    {
        setObjectName("browser");
        setWindowTitle(tr("Browse trace"));

        setWhatsThis(tr("<b>Trace browser</b>"
                     "<p>The trace browser is used to navigate the trace "
                     "and get basic information about events and states."));

        zoom_in = new QPixmap(":/viewmag+.png", 0);
        if (!zoom_in->isNull())
            zoom_in_cursor = new QCursor(*zoom_in);
        zoom_out = new QPixmap(":/viewmag+.png", 0);


        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        QGroupBox* help_group = new QGroupBox(tr("Help"), this);
        mainLayout->addWidget(help_group);

        QVBoxLayout* helpLayout = new QVBoxLayout(help_group);
        help_group->setLayout(helpLayout);

        stock_help =
            tr("Object details: <b>Click</b><br>"
            "Zoom in: <b>Shift+Click</b><br>"
            "Zoom out: <b>Shift+Right Click</b>");

        helpLabel = new QLabel(stock_help, help_group);
        helpLabel->setAlignment(Qt::AlignTop);
        helpLabel->setWordWrap(true);

        helpLayout->addWidget(helpLabel);

        infoStack = new QStackedWidget(this);
        mainLayout->addWidget(infoStack);

        trace_info_ = new Browser_trace_info(infoStack);
        infoStack->addWidget(trace_info_);

        event_info_ = new Browser_event_info(infoStack);
        infoStack->addWidget(event_info_);

        state_info_ = new Browser_state_info(infoStack);
        infoStack->addWidget(state_info_);

        createToolbarActions();

        connect(canvas(), SIGNAL(modelChanged(Trace_model::Ptr &)),
                this, SLOT(modelChanged(Trace_model::Ptr &)));
    }

    ~BrowserReal()
    {
        delete zoom_in;
        delete zoom_out;
        delete zoom_in_cursor;
    }

    void createToolbarActions()
    {
        startAction = new QAction(QIcon(":/start.png"), "", this);
        startAction->setToolTip(tr("Start"));
        startAction->setShortcut(Qt::Key_Home);
        connect(startAction, SIGNAL(triggered(bool)), this, SLOT(goHome()));

        prevAction = new QAction(QIcon(":/previous.png"), "", this);
        prevAction->setToolTip(tr("Previous"));
        prevAction->setShortcut(Qt::Key_Left);
        connect(prevAction, SIGNAL(triggered(bool)), this, SLOT(goLeft()));

        zoominAction = new QAction(QIcon(":/viewmag+.png"), "", this);
        zoominAction->setToolTip(tr("Zoom in"));
        zoominAction->setShortcut(Qt::Key_Plus);
        connect(zoominAction, SIGNAL(triggered(bool)), this, SLOT(zoomInCenter()));

        fitAction = new QAction(QIcon(":/fit.png"), "", this);
        fitAction->setToolTip(tr("Fit"));
        connect(fitAction, SIGNAL(triggered(bool)), this, SLOT(fit()));

        zoomoutAction = new QAction(QIcon(":/viewmag-.png"), "", this);
        zoomoutAction->setToolTip(tr("Zoom out"));
        zoomoutAction->setShortcut(Qt::Key_Minus);
        connect(zoomoutAction, SIGNAL(triggered(bool)), this, SLOT(zoomOutCenter()));

        nextAction = new QAction(QIcon(":/next.png"), "", this);
        nextAction->setToolTip(tr("Next"));
        nextAction->setShortcut(Qt::Key_Right);
        connect(nextAction, SIGNAL(triggered(bool)), this, SLOT(goRight()));

        finishAction = new QAction(QIcon(":/finish.png"), "", this);
        finishAction->setToolTip(tr("End"));
        finishAction->setShortcut(Qt::Key_End);
        connect(finishAction, SIGNAL(triggered(bool)), this, SLOT(goEnd()));
    }

    void addToolbarActions(QToolBar* toolbar)
    {
        toolbar->addSeparator();
        toolbar->addAction(startAction);
        toolbar->addAction(prevAction);
        toolbar->addAction(zoominAction);
        toolbar->addAction(fitAction);
        toolbar->addAction(zoomoutAction);
        toolbar->addAction(nextAction);
        toolbar->addAction(finishAction);

        checkActions();
    }

    QAction* createAction()
    {
        return new QAction(QIcon(":/point.png"), tr("&Browse"), this);
    }

    void activate()
    {
        trace_info_->update(canvas());
        active_ = true;
    }

    void deactivate()
    {
        active_ = false;
    }

    // 'do' is to avoid naming conflict with the signal.
    void doShowEvent(Event_model* event)
    {
        infoStack->setCurrentWidget(event_info_);
        event_info_->showEvent(canvas(), event);
    }

    void doShowState(State_model* state)
    {
        infoStack->setCurrentWidget(state_info_);
        state_info_->update(model(), state);
    }


private:

    bool event(QEvent * event)
    {
        if (event->type() == QEvent::ShortcutOverride)
            return false;
        return Browser::event(event);
    }

    bool mouseEvent(QEvent* event,
                          Canvas::clickTarget target,
                          int component,
                          State_model* state,
                          const Time& time,
                          bool events_near)
    {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent * wEvent = static_cast<QWheelEvent*>(event);
            if (wEvent->delta() < 0) goRight(); else goLeft();
            return true;
        }

        QMouseEvent * mEvent = static_cast<QMouseEvent*>(event);
        bool result = false;
        if (target == Canvas::componentClicked)
        {
            if (model()->has_children(component) && mEvent->button() == Qt::LeftButton)
            {
                Trace_model::Ptr new_model
                    = model()->set_parent_component(component);
                canvas()->setModel(new_model);

                result = true;
            }
        }
        else if (time >= model()->min_time()) /* handle clicks only in the lifelines area  */
        {
            if (mEvent->modifiers() & Qt::ShiftModifier)
            {
                if (mEvent->button() == Qt::LeftButton)
                {
                    zoomInAt(time);
                }
                else if (mEvent->button() == Qt::RightButton)
                {
                    zoomOutAt(time);
                }
                result = true;
            }
            else if (mEvent->button() == Qt::LeftButton)
            {
                if(events_near)
                {
                    event_info_->update(canvas(), component, time);
                    infoStack->setCurrentWidget(event_info_);
                    emit activateMe();
                }
                else if (state)
                {
                    state_info_->update(model(), state);
                    infoStack->setCurrentWidget(state_info_);
                    emit activateMe();
                }
                else
                {
                    infoStack->setCurrentWidget(trace_info_);
                }
                result = true;
            }
        }
        return result;
    }

    bool mouseMoveEvent(QMouseEvent* ev, Canvas::clickTarget target,
                        int component, const Time& time)
    {
        if (target == Canvas::componentClicked)
        {
            canvas()->setCursor(Qt::PointingHandCursor);
        }
        else if (target == Canvas::stateClicked && active_)
        {
            canvas()->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            canvas()->setCursor(Qt::ArrowCursor);
        }
        return true;
    }

private slots:

    void reset()
    {
        infoStack->setCurrentWidget(trace_info_);
    }

    void goHome()
    {
        Trace_model::Ptr m = model();
        Time delta = m->max_time() - m->min_time();

        Time new_min = m->root()->min_time();

        canvas()->setModel(model()->set_range(new_min, new_min + delta));
    }

    void goEnd()
    {
        Trace_model::Ptr m = model();
        Time delta = m->max_time() - m->min_time();

        Time new_max = m->root()->max_time();

        canvas()->setModel(m->set_range(new_max - delta, new_max));
    }

    void fit()
    {
        Trace_model::Ptr root = model()->root();

        canvas()->setModel(
            model()->set_range(root->min_time(), root->max_time()));
    }

    void goLeft()
    {
        Trace_model::Ptr m = model();
        Time delta = m->max_time() - m->min_time();

        Time new_min = m->min_time()-delta/4;
        Time root_min = m->root()->min_time();
        if (new_min < root_min)
            new_min = root_min;

        Time new_max = new_min + delta;

        canvas()->setModel(
            model()->set_range(new_min, new_max));
    }

    void goRight()
    {
        Trace_model::Ptr m = model();
        Time delta = m->max_time() - m->min_time();

        Time new_max = m->max_time()+delta/4;
        if (new_max > m->root()->max_time())
        {
            new_max = m->root()->max_time();
        }

        Time new_min = new_max - delta;

        canvas()->setModel(
            model()->set_range(new_min, new_max));
    }

    // This is extra hacky and is desired to work only for specific
    // case of switch from search widget to browse widget.
    void extraHelp(const QString& s)
    {
        if (s.isEmpty())
        {
            helpLabel->setText(stock_help);
        }
        else
        {
            helpLabel->setText(s + "<br>" + stock_help);
        }
    }

    void modelChanged(Trace_model::Ptr &)
    {
        trace_info_->update(canvas());
        if (infoStack->currentWidget() == event_info_)
            event_info_->updateTime();
        if (infoStack->currentWidget() == state_info_)
            state_info_->updateTime();

        checkActions();
    }


    void zoomInCenter()
    {
        zoomInAt(Time::scale(
                     model()->min_time(), model()->max_time(), 0.5));
    }

    void zoomOutCenter()
    {
        zoomOutAt(Time::scale(
                      model()->min_time(), model()->max_time(), 0.5));
    }

private:

    void zoomInAt(const Time& time)
    {
        Time new_min = (model()->min_time() + time)/2;
        Time new_max = (model()->max_time() + time)/2;

        if (new_max - new_min < model()->min_resolution())
            return;

        canvas()->setModel(
            model()->set_range(new_min, new_max));
    }

    void zoomOutAt(const Time& time)
    {
        Time new_min = time
            - (time - model()->min_time())*2;
        Time new_max = time
            + (model()->max_time() - time)*2;

        Trace_model::Ptr r = model()->root();
        if ((new_max - new_min) >
            (r->max_time() - r->min_time()))
        {
            new_min = r->min_time();
            new_max = r->max_time();
        }
        else if (new_min < r->min_time())
        {
            new_min = r->min_time();
            new_max = new_max - (new_min - r->min_time());
        }
        if (new_max > r->max_time())
        {
            Time delta = new_max - new_min;
            new_max = r->max_time();
            new_min = new_max - delta;
        }

        canvas()->setModel(
            model()->set_range(new_min, new_max));
    }

    /** Disables unavailable toolbar's actions. */
    void checkActions()
    {
        Trace_model::Ptr m = model();
        Trace_model::Ptr r = model()->root();

        bool min_time = m->min_time() == r->min_time();
        startAction->setEnabled(!min_time);
        prevAction->setEnabled(!min_time);

        bool max_time = m->max_time() == r->max_time();
        nextAction->setEnabled(!max_time);
        finishAction->setEnabled(!max_time);

        bool max_zoom = (m->max_time()-m->min_time())/2 < m->min_resolution();
        zoominAction->setEnabled(!max_zoom);

        bool full_trace = min_time && max_time;
        fitAction->setEnabled(!full_trace);
        zoomoutAction->setEnabled(!full_trace);
    }

private:
    QAction* selfAction;
    QString stock_help;

    QStackedWidget* infoStack;
    QLabel* helpLabel;
    Browser_trace_info* trace_info_;
    Browser_event_info* event_info_;
    Browser_state_info* state_info_;

    // Toolbar's actions.
    QAction* startAction;
    QAction* prevAction;
    QAction* zoominAction;
    QAction* fitAction;
    QAction* zoomoutAction;
    QAction* nextAction;
    QAction* finishAction;


    QPixmap* zoom_in;
    QCursor* zoom_in_cursor;
    QPixmap* zoom_out;

    bool active_;
};

Browser* createBrowser(QWidget* parent, Canvas* canvas)
{
    return new BrowserReal(parent, canvas);
}

}

