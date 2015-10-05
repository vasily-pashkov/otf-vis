#ifndef EVENT_LIST_HPP_VP_2006_04_03
#define EVENT_LIST_HPP_VP_2006_04_03

#include "trace_model.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QVector>

namespace vis4 {

class Event_model;

class Event_list : public QTreeView
{
    Q_OBJECT
public:
    Event_list(QWidget* parent);

    void showEvents(Trace_model::Ptr & model, const common::Time & time);

    void updateTime();

    QSize sizeHint() const;

    void scrollTo(const QModelIndex & index, ScrollHint hint = EnsureVisible);

    Event_model * currentEvent();
    void setCurrentEvent(Event_model * event);

signals:
    void currentEventChanged(Event_model*);

private slots:
    void eventListRowChanged(const QModelIndex& index);

private:
    QStandardItemModel* model_;
    QVector<Event_model*> events;
};


}
#endif
