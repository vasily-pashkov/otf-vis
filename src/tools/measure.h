#include "tool.h"

#include "canvas_item.h"
#include "event_list.h"
#include "trace_model.h"
#include "canvas.h"
#include "event_model.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QAction>

namespace vis4 {

using common::Time;
using common::Selection;

class MeasureRibbon : public CanvasItem
{
public:
    MeasureRibbon() : shown_(true) {}

    void setA(const QPoint& p)
    {
        A = p;
        refresh();
    }

    void setB(const QPoint& p)
    {
        B = p;
        refresh();
    }

    void setShown(bool shown)
    {
        shown_ = shown;
        new_geomerty(pdraw(0));
    }

private: // CanvasItem override
    QRect xdraw(QPainter& painter)
    {
        if (shown_)
            return pdraw(&painter);
        else
            return pdraw(0);
    }

    QRect pdraw(QPainter* painter)
    {
        QRect total;

        if (painter)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(QPen(Qt::black, 3));
        }


        total |= drawCross(A, painter);
        total |= drawCross(B, painter);

        if (painter)
        {
            if (!A.isNull() && !B.isNull())
            {
                if (A.y() == B.y())
                {
                    painter->setPen(Qt::white);
                    painter->drawLine(A, B);
                }

                QPen pen(Qt::blue);
                QVector<qreal> dashes;
                dashes.push_back(15);
                dashes.push_back(15);
                pen.setDashPattern(dashes);
                painter->setPen(pen);
                painter->drawLine(A, B);
            }
        }

        if (painter)
        {
            painter->restore();
        }

        return total;
    }

    QRect drawCross(const QPoint& A, QPainter* painter)
    {
        const int cross_size = 5;

        QRect b;
        if (!A.isNull())
        {
            b = QRect(A - (QPoint(cross_size, cross_size)),
                           QSize(cross_size*2, cross_size*2));

            if (painter)
            {
                painter->drawLine(A.x()-cross_size, A.y(),
                                  A.x()+cross_size, A.y());

                painter->drawLine(A.x(), A.y()-cross_size,
                                  A.x(), A.y()+cross_size);
            }
        }
        b.adjust(-3, -3, 3, 3);
        return b;
    }


    void refresh()
    {
        new_geomerty(pdraw(0));
    }


private:
    QPoint A, B;
    bool shown_;
    friend class Measure;

public:
    using CanvasItem::new_geomerty;
};

class Measure_point_display : public QGroupBox
{
    Q_OBJECT
public:
    Measure_point_display(const QString& name, QWidget* parent)
    : QGroupBox(name, parent), currentlySelecting(false)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        timeLabel = new QLabel("", this);
        timeLabel->setWordWrap(true);
        mainLayout->addWidget(timeLabel);

        selectEventLabel = new QLabel(tr("Snap to event:"), this);
        mainLayout->addWidget(selectEventLabel);

        events = new Event_list(this);
        mainLayout->addWidget(events);

        mainLayout->addStretch();

        connect(events, SIGNAL(currentEventChanged(Event_model*)),
                this, SLOT(currentEventChanged(Event_model*)));
    }

    void setCurrentlySelecting(bool s)
    {
        currentlySelecting = s;
        if (time.isNull())
            showNoTime();
    }

    void showNoTime()
    {
        QString s = tr("Not selected yet.");
        if (currentlySelecting)
            s += " " + tr("<b>Click to select</b>");
        timeLabel->setText(s);
        events->hide();
        selectEventLabel->hide();
    }

    void setPoint(int component, const Time &time,
                  Trace_model::Ptr model,
                  Canvas* canvas)
    {
        this->time = time;
        if (time.isNull() || component == -1)
        {
            this->time = Time();
            showNoTime();
            return;
        }

        selectedPoint(time);

        Selection component_filter = model->components();
        component_filter.disableAll(model->parent_component());
        component_filter.setEnabled(component, true);

        QPair<Time, Time> nearby = canvas->nearby_range(time);

        Trace_model::Ptr filtered_ =
            model->set_range(nearby.first, nearby.second);
        filtered_ = filtered_->filter_components(component_filter);

        filtered_->rewind();

        events->showEvents(filtered_, time);

        // FIXME: auto-snap if there's one event.
        if (events->model()->rowCount() == 0)
        {
            events->hide();
            selectEventLabel->hide();
        }
        else if (events->model()->rowCount() == 1)
        {
            // Just one event, snap to it.
            events->hide();
            selectEventLabel->hide();
            events->setCurrentIndex(events->model()->index(0, 0));
        }
        else
        {
            events->show();
            selectEventLabel->show();
            events->setFocus(Qt::OtherFocusReason);

        }
    }

    void selectedPoint(const Time& time)
    {
        timeLabel->setText(tr("Time: %1").arg(time.toString(true)));
    }

signals:
    void eventSelected(const Time& time);

private slots:

    void currentEventChanged(Event_model* event)
    {
        emit eventSelected(event->time);
    }


private:
    QLabel* timeLabel;

    QLabel* selectEventLabel;
    Event_list* events;

    Time time;
    bool currentlySelecting;
};


class Measure : public Tool
{
    Q_OBJECT
public:
    Measure(QWidget* parent, Canvas* c)
    : Tool(parent, c), active_(false),
      point_a_fixed(false), point_b_fixed(false),
      component_a(-1), component_b(-1)
    {
        setObjectName("measure");
        setWindowTitle(tr("Measure"));

        setWhatsThis(tr("<b>Measure distances</b>"
                     "<p>Allows to measure distances between two events "
                     "or arbitrary points on the trace. "
                     "<p>Click on the trace to set point A, click again to set "
                     "point B, and read the distance on the buttom of the sidebar."
                     "<p>If you clear near event, the point is automatically "
                     "snapped to the event. If there are several events near, "
                     "you will see a listbox with all events and will be able "
                     "to select one event to snap to."));



        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        pointA = new Measure_point_display(tr("Point A"), this);
        mainLayout->addWidget(pointA);


        pointB = new Measure_point_display(tr("Point B"), this);
        mainLayout->addWidget(pointB);

        QGroupBox* distanceGroup = new QGroupBox(tr("Distance"), this);
        distanceGroup->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Maximum);
        mainLayout->addWidget(distanceGroup);

        // Distance group contents.
        QVBoxLayout* distanceLayout = new QVBoxLayout(distanceGroup);
        distanceLabel = new QLabel("", distanceGroup);
        distanceLayout->addWidget(distanceLabel);

        connect(canvas(), SIGNAL(modelChanged(Trace_model::Ptr &)),
                this, SLOT(modelChanged(Trace_model::Ptr &)));

        connect(pointA, SIGNAL(eventSelected(const Time&)),
                this, SLOT(eventSelected(const Time&)));

        connect(pointB, SIGNAL(eventSelected(const Time&)),
                this, SLOT(eventSelected(const Time&)));

        ribbon = new MeasureRibbon;
        canvas()->addItem(ribbon);

        pointA->setCurrentlySelecting(true);
        model_ = model();
        updateView();
    }

    QAction* createAction()
    {
        QAction* measure_ = new QAction(QIcon(":/kruler.png"),
                                        tr("&Measure"), this);
        measure_->setShortcut(QKeySequence(Qt::Key_M));
        return measure_;
    }

    void activate()
    {
        active_ = true;
        canvas()->setCursor(Qt::CrossCursor);
        ribbon->setShown(true);
    }

    void deactivate()
    {
        active_ = false;
        canvas()->setCursor(Qt::ArrowCursor);
        ribbon->setShown(false);
    }

private:

    void updateView(bool snapped = false)
    {
        if (snapped)
        {
            if (!time_a.isNull())
                pointA->selectedPoint(time_a);
            if (!time_b.isNull())
                pointB->selectedPoint(time_b);
        }
        else
        {
            pointA->setPoint(component_a, time_a, model(), canvas());
            pointB->setPoint(component_b, time_b, model(), canvas());
        }

        if (!time_a.isNull() && !time_b.isNull())
        {
            Time d = distance(time_a, time_b);
            distanceLabel->setText(tr("Distance: %1").arg(d.toString(true)));
        }
        else
        {
            distanceLabel->setText(tr("Points not selected yet"));
        }
    }

private slots:

    bool mouseEvent(QEvent* event, Canvas::clickTarget target,
                    int component,  State_model*,
                    const Time& time, bool eventsNear)
    {
        if (!active_)
            return false;

        if (event->type() != QEvent::MouseButtonRelease) return false;
        QMouseEvent * mEvent = static_cast<QMouseEvent*>(event);

        if (mEvent->button() == Qt::LeftButton
            && (target == Canvas::lifelinesClicked
                || target == Canvas::stateClicked
                || target == Canvas::nothingClicked))
        {
            if (!point_a_fixed && component_a != -1)
            {
                ribbon->setA(canvas()->lifeline_point(component_a, time));
                time_a = time;
                point_a_fixed = true;;
            }
            else if (!point_b_fixed && component_b != -1)
            {
                ribbon->setB(
                    canvas()->lifeline_point(component_b, time));
                time_b = time;
                point_b_fixed = true;
            }
            else if (point_a_fixed && point_b_fixed)
            {
                // Both points set. Reset point A and B;
                component_a = canvas()->nearest_lifeline(mEvent->y());
                ribbon->setA(canvas()->lifeline_point(component_a, time));
                time_a = Time();
                ribbon->setB(QPoint());
                time_b = Time();
                component_b = -1;
                point_a_fixed = point_b_fixed = false;
            }

            updateView();
            pointA->setCurrentlySelecting(!point_a_fixed);
            pointB->setCurrentlySelecting(point_a_fixed && !point_b_fixed);
            return true;
        }

        return false;
    }

    bool mouseMoveEvent(QMouseEvent* ev, Canvas::clickTarget target,
                        int component, const Time& time)
    {
        if (!active_)
            return false;

        if (target != Canvas::lifelinesClicked
            && target != Canvas::stateClicked)
            return false;

        if (point_a_fixed && point_b_fixed)
            return false;

        int nearest_lifeline = canvas()->nearest_lifeline(ev->y());
        QPoint p = canvas()->lifeline_point(nearest_lifeline, time);

        if (!point_a_fixed)
        {
            component_a = nearest_lifeline;
            ribbon->setA(p);
        }
        else
        {
            component_b = nearest_lifeline;
            ribbon->setB(p);
        }

	return false;
    }

    void eventSelected(const Time& time)
    {
        if (sender() == pointA)
        {
            time_a = time;
            ribbon->setA(canvas()->lifeline_point(component_a, time));
        }
        else
        {
            time_b = time;
            ribbon->setB(canvas()->lifeline_point(component_b, time));
        }

        updateView(true /* snapped */);
    }

    void modelChanged(Trace_model::Ptr & model)
    {
        int d = delta(*model.get(), *model_.get());
        if (d & Trace_model_delta::component_position ||
            d & Trace_model_delta::components)
        {
            point_a_fixed = point_b_fixed = false;
            component_a = component_b = -1;
            time_a = time_b = Time();

            ribbon->setA(QPoint());
            ribbon->setB(QPoint());
        }

        if (d & Trace_model_delta::time_range)
        {
            if (point_a_fixed)
                ribbon->setA(canvas()->lifeline_point(component_a, time_a));
            if (point_b_fixed)
                ribbon->setB(canvas()->lifeline_point(component_b, time_b));
        }

        model_ = model;
        updateView();
    }

private:
    bool active_;

    MeasureRibbon* ribbon;
    Measure_point_display* pointA;
    Measure_point_display* pointB;
    QLabel* distanceLabel;

    bool point_a_fixed;
    bool point_b_fixed;
    int component_a;
    int component_b;
    Time time_a;
    Time time_b;

    Trace_model::Ptr model_;
};


Tool* createMeasure(QWidget* parent, Canvas* canvas)
{
    return new Measure(parent, canvas);
}

}

