#include "event_list.h"

#include "trace_model.h"
#include "event_model.h"

#include <QHeaderView>
#include <QScrollBar>

namespace vis4 {

using common::Time;

Event_list::Event_list(QWidget* parent) : QTreeView(parent), model_(0)
{
    //setSelectionBehaviour(SelectRows);
    QTreeView::setRootIsDecorated(false);
    setEditTriggers(NoEditTriggers);
    header()->hide();
}

void Event_list::showEvents(Trace_model::Ptr & model, const Time & time)
{
    delete model_;
    events.clear();

    model_ = new QStandardItemModel(this);
    model_->insertColumns(0, 2);
    QTreeView::setModel(model_);
    connect(this->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(eventListRowChanged(const QModelIndex &)));

    int nearest_event = -1; Time min_distance; int row = 0;
    for(;;)
    {
        std::auto_ptr<Event_model> e = model->next_event();

        if (!e.get())
            break;

        model_->insertRow(row);

        if (nearest_event == -1 || distance(time, e->time) < min_distance)
        {
            nearest_event = row;
            min_distance = distance(time, e->time);
        }

        QModelIndex index = model_->index(row, 0, QModelIndex());
        model_->setData(index, e->time.toString());
        index = model_->index(row, 1, QModelIndex());
        model_->setData(index, e->shortDescription());
        model_->setData(index, e->shortDescription(), Qt::ToolTipRole);
        events.push_back(e.release());
        ++row;
    }

    setAlternatingRowColors(true);

    resizeColumnToContents(0);
    resizeColumnToContents(1);

    if (nearest_event != -1) {
        selectionModel()->setCurrentIndex(this->model()->index(nearest_event, 0),
            QItemSelectionModel::Select);
        selectionModel()->select(this->model()->index(nearest_event, 0),
            QItemSelectionModel::Select);
        selectionModel()->select(this->model()->index(nearest_event, 1),
        QItemSelectionModel::Select);
    }
}

void Event_list::updateTime()
{
    for(int i = 0; i < events.size(); ++i)
    {
        QModelIndex index = model_->index(i, 0, QModelIndex());
        model_->setData(index, events[i]->time.toString());
    }
    resizeColumnToContents(0);
}

Event_model * Event_list::currentEvent()
{
    int row = currentIndex().row();
    if (row != -1)
    {
        Q_ASSERT(row < events.size());

        return events[row];
    }

    return 0;
}

void Event_list::setCurrentEvent(Event_model * event)
{
    for(int i = 0; i < events.size(); ++i)
    {
        if (*(events[i]) == *event)
        {
            QModelIndex index = model_->index(i, 0, QModelIndex());
            setCurrentIndex(index); scrollTo(index);
            return;
        }
    }

    qFatal("Can't find given event in the list");
}

void Event_list::eventListRowChanged(const QModelIndex& index)
{
    Event_model * event = currentEvent();
    if (!event) return;

    emit currentEventChanged(event);
}

void Event_list::scrollTo(const QModelIndex & index, ScrollHint hint)
{
    // Save state of horizontal bar while vertical scrolling
    int hscrollbar_state = horizontalScrollBar()->value();
    QTreeView::scrollTo(index, hint);
    horizontalScrollBar()->setValue(hscrollbar_state);
}

QSize Event_list::sizeHint() const
{
    return QSize(100, 192);
}

}
