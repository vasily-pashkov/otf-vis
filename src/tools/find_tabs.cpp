#include "find_tabs.h"
#include "trace_model.h"
#include "event_model.h"
#include "state_model.h"
#include "canvas.h"

#include "selection_widget.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QStackedLayout>

#include <set>

namespace vis4 {

using std::set;
using common::Selection;
using common::SelectionWidget;

//---------------------------------------------------------------------------------------
// FindEventsTab class implementation
//---------------------------------------------------------------------------------------

FindEventsTab::FindEventsTab(Tool * find_tool)
    : FindTab(find_tool)
{
    setObjectName("events");

    selector_ = new SelectionWidget(this);
    selector_->minimizeSize(true);
    connect(selector_, SIGNAL(selectionChanged(const vis4::common::Selection &)),
        this, SLOT(selectionChanged(const vis4::common::Selection &)));

    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(selector_);
}

void FindEventsTab::reset()
{
    filtered_model_.reset();
}

bool FindEventsTab::findNext()
{
    Q_ASSERT(model_.get() != 0);

    if (!filtered_model_.get()) {
        Selection filter = model_->events() & selector_->selection();
        filtered_model_ = model_->filter_events(filter);
        filtered_model_->rewind();
    }

    std::auto_ptr<Event_model> e = filtered_model_->next_event();
    if (e.get())
    {
        model_ = model_->set_range(e->time, model_->max_time());
        emit showEvent(e.get());
        return true;
    }

    return false;
}

void FindEventsTab::setModel(Trace_model::Ptr & model)
{
    if (model_.get())
    {
        if (delta(*model.get(), *model_.get()) &
            Trace_model_delta::event_types)
        {
            selector_->initialize(model->events(),
                selector_->selection());
            reset();
        }
    }
    else
    {
            selector_->initialize(model->events(), model->events());
    }

    model_ = model;
    emit stateChanged();
}

bool FindEventsTab::isSearchAllowed()
{
    return (selector_->selection().enabledCount() != 0);
}

void FindEventsTab::saveState(QSettings & settings)
{
    settings.beginGroup("events");
    selector_->saveState(settings);
    settings.endGroup();
}

void FindEventsTab::restoreState(QSettings & settings)
{
    settings.beginGroup("events");
    selector_->restoreState(settings);
    settings.endGroup();
}

void FindEventsTab::selectionChanged(const Selection & selection)
{
    reset();
    emit stateChanged();
}

//---------------------------------------------------------------------------------------
// FindStatesTab class implementation
//---------------------------------------------------------------------------------------

FindStatesTab::FindStatesTab(Tool * find_tool)
    : FindTab(find_tool)
{
    setObjectName("states");

    selector_ = new SelectionWidget(this);
    selector_->minimizeSize(true);
    connect(selector_, SIGNAL(selectionChanged(const vis4::common::Selection &)),
            this, SLOT(selectionChanged(const vis4::common::Selection &)));

    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(selector_);
}

void FindStatesTab::reset()
{
    filtered_model_.reset();
}

bool FindStatesTab::findNext()
{
    Q_ASSERT(model_.get() != 0);

    if (!filtered_model_.get()) {
        Selection filter = model_->states() & selector_->selection();
        filtered_model_ = model_->filter_states(filter);
        filtered_model_->rewind();

    }

    for (;;) {
        std::auto_ptr<State_model> s = filtered_model_->next_state();
        if (s.get())
        {
            model_ = model_->set_range(s->begin, model_->max_time());
            emit showState(s.get());
            return true;
        }

        return false;
    }
}

void FindStatesTab::setModel(Trace_model::Ptr & model)
{
    if (model_.get())
    {
        if (delta(*model.get(), *model_.get()) &
            Trace_model_delta::state_types)
        {
            selector_->initialize(model->available_states() & model->states(),
                selector_->selection());
            reset();
        }
    }
    else
    {
        Selection states = model->available_states() & model->states();
        selector_->initialize(states, states);
    }

    model_ = model;
    emit stateChanged();
}

bool FindStatesTab::isSearchAllowed()
{
    return (selector_->selection().enabledCount() != 0);
}

void FindStatesTab::saveState(QSettings & settings)
{
    settings.beginGroup("states");
    selector_->saveState(settings);
    settings.endGroup();
}

void FindStatesTab::restoreState(QSettings & settings)
{
    settings.beginGroup("states");
    selector_->restoreState(settings);
    settings.endGroup();
}

void FindStatesTab::selectionChanged(const Selection & selection)
{
    reset();
    emit stateChanged();
}

//---------------------------------------------------------------------------------------
// FindQueryTab class implementation
//---------------------------------------------------------------------------------------

FindQueryTab::FindQueryTab(Tool * find_tool)
    : FindTab(find_tool), active_checker(0),
      active_checker_is_ready(false)
{
    setObjectName("query");

    QVBoxLayout * layout = new QVBoxLayout(this);

    QLabel * l = new QLabel(tr("Condition:"), this);
    layout->addWidget(l);

    checkerCombo = new QComboBox(this);
    checkerCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    layout->addWidget(checkerCombo);

    layout->addSpacing(10);

    // Do we have any checkers in general?
    if (Checker::availableCheckers().isEmpty())
    {
        checkerCombo->addItem(tr("No checkers"));
        checkerCombo->setEnabled(false);
        layout->addStretch(); return;
    }


    // Add nice title "Checker settings"

    checkerSettings_layout = new QHBoxLayout();
    layout->addLayout(checkerSettings_layout);

    QFrame * leftLine = new QFrame(this);
    leftLine->setFrameStyle(QFrame::HLine);
    checkerSettings_layout->addWidget(leftLine);

    checkerSettings_layout->addWidget(new QLabel(tr("Checker settings"), this));

    QFrame * rightLine = new QFrame(this);
    rightLine->setFrameStyle(QFrame::HLine);
    checkerSettings_layout->addWidget(rightLine);

    // Add checker widget container
    checkerWidgetContainer = new QStackedLayout();
    checkerWidgetContainer->setMargin(0);
    layout->addLayout(checkerWidgetContainer);
    layout->addStretch();

    // Create tip for case when not all necessary events is enabled.

    checkerWidgetContainer->addWidget(new QWidget(this));
    QVBoxLayout * infoTab_layout = new QVBoxLayout(
        checkerWidgetContainer->widget(0));

    QLabel * tipLabel = new QLabel(this);
    tipLabel->setText(tr("<b>Some necessary events for this checker is filtered. "
        "Checker is not available.</b>"));
    tipLabel->setWordWrap(true);
    infoTab_layout->addWidget(tipLabel);

    QPushButton * activateEventsBtn = new QPushButton(tr("Activate events"), this);
    connect(activateEventsBtn, SIGNAL( pressed() ),
        this, SLOT( activateCheckerEvents() ));
    infoTab_layout->addWidget(activateEventsBtn);
    infoTab_layout->addStretch();

    initializeCheckers();

    connect(checkerCombo, SIGNAL( currentIndexChanged(int) ),
        this, SLOT( checkerStateChanged() ));
}

void FindQueryTab::reset()
{
    model_with_checker.reset();
}

bool FindQueryTab::findNext()
{
    Q_ASSERT(model_.get() != 0);

    if (!model_with_checker.get()) {
        model_with_checker = model_->install_checker(active_checker);
        model_with_checker->rewind();
    }

    std::auto_ptr<Event_model> e =
        model_with_checker->next_event();

    if (e.get())
    {
        model_ = model_->set_range(e->time, model_->max_time());
        emit showEvent(e.get());
        return true;
    }

    return false;
}

void FindQueryTab::setModel(Trace_model::Ptr & model)
{
    if (checkers.isEmpty()) return;

    model_ = model;
    updateChecker();

    emit stateChanged();
}

bool FindQueryTab::isSearchAllowed()
{
    return active_checker_is_ready;
}

void FindQueryTab::saveState(QSettings & settings)
{
    if (checkers.isEmpty()) return;
    settings.setValue("checker_name", checkerCombo->currentText());
}

void FindQueryTab::restoreState(QSettings & settings)
{
    if (checkers.isEmpty()) return;

    // Restore state of checker box
    QString checker_name = settings.value("checker_name").toString();
    if (!checker_name.isEmpty()) {
        int index = checkerCombo->findText(checker_name);
        if (index != -1)
            checkerCombo->setCurrentIndex(index);
    }
}

void FindQueryTab::initializeCheckers()
{
    QStringList names = Checker::availableCheckers();
    for (int i = 0; i < names.size(); ++i) {
        pChecker checker = Checker::createChecker(names[i]);
        checkerCombo->addItem(checker->title());

        QWidget * widget = checker->widget();
        widget->layout()->setMargin(0);
        checkerWidgetContainer->addWidget(widget);

        connect(checker.get(), SIGNAL( stateChanged() ),
            this, SLOT( checkerStateChanged() ));

        checkers << checker;
    }
}

void FindQueryTab::updateChecker()
{
    Q_ASSERT(model_ != 0);

    int checker_no = checkerCombo->currentIndex();
    if (active_checker != checkers[checker_no].get())
    {
        active_checker = checkers[checker_no].get();
        reset();
    }
    active_checker->setModel(model_);

    // Check events necessary for checker

    bool checker_events_enabled = true;
    const Selection & events = model_->events();

    // Process checker's events
    set<int> checker_events = active_checker->events();
    for (set<int>::iterator it = checker_events.begin();
        (it != checker_events.end()) && checker_events_enabled; it++)
    {
        int eventLink = events.itemLink(*it);
        if (!events.isEnabled(eventLink))
            { checker_events_enabled = false; break; }

        // Process checker's subevents
        set<int> checker_subevents = active_checker->subevents(*it);
        for (set<int>::iterator it2 = checker_subevents.begin();
            it2 != checker_subevents.end(); it2++)
        {
            if (events.itemsCount(eventLink) >= *it2) continue;
            if (!events.isEnabled(*it2, eventLink))
                { checker_events_enabled = false; break; }
        }
    }

    // Activate widget for current checker
    if (checker_events_enabled)
        checkerWidgetContainer->setCurrentIndex(checker_no+1);
    else
        // Some necessary events filtered,
        // so we must tell user about it.
        checkerWidgetContainer->setCurrentIndex(0);

    // Show/hide title 'Checker settings'
    for (int i = 0; i < checkerSettings_layout->count(); ++i)
    {
        checkerSettings_layout->itemAt(i)->widget()->
            setVisible(checker_events_enabled);
    }

    // Disable "Find" button if checker haven't correct settings.
    active_checker_is_ready = checker_events_enabled && active_checker->isReady();
}

void FindQueryTab::activateCheckerEvents()
{
    Selection events = model_->events();

    // Process checker's events
    set<int> checker_events = active_checker->events();
    for (set<int>::iterator it = checker_events.begin();
        it != checker_events.end(); it++)
    {
        int eventLink = events.itemLink(*it);
        events.setEnabled(eventLink, true);

        // Process checker's subevents
        set<int> checker_subevents = active_checker->subevents(*it);
        for (set<int>::iterator it2 = checker_subevents.begin();
            it2 != checker_subevents.end(); it2++)
        {
            if (events.itemsCount(eventLink) > *it2)
                events.setEnabled(*it2, eventLink, true);
        }
    }

    Canvas * canvas = find_tool_->canvas();
    canvas->setModel(canvas->model()->filter_events(events));
}

void FindQueryTab::checkerStateChanged()
{
    reset();
    updateChecker();
    emit stateChanged();
}

}
