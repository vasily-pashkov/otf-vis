#include "canvas.h"
#include "trace_model.h"
#include "timeline.h"
#include "canvas_item.h"
#include "trace_painter.h"
#include "state_model.h"

#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>
#include <QKeyEvent>
#include <QPicture>
#include <QWhatsThis>
#include <QVector>
#include <QList>
#include <QToolTip>
#include <QCursor>
#include <QApplication>
#include <QSettings>
#include <QTimer>

#include <math.h>

namespace vis4 {

using namespace std;
using common::Time;

const int arrowhead_length = 16;

Canvas::Canvas(QWidget* parent)
    : QScrollArea(parent), timeline_(0), updateTimer(0)
{
    setWhatsThis(tr("<b>Time diagram</b>"
                 "<p>Time diagram shows the components of the distributed system"
                 ", events in those compoments, and states those components are in."
                 "<p>Compoments are green boxes on the left. Composite compoments "
                 "are shown as stack."));

    contents_ = new Contents_widget(this);
    setWidget(contents_);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWidgetResizable(true);

    timeline_ = new Timeline(this, contents_->trace_painter.get());
    setViewportMargins(0, 0, 0, timeline_->sizeHint().height());
    connect(timeline_, SIGNAL( timeSettingsChanged() ),
        this, SLOT( timeSettingsChanged() ) );
}

void Canvas::setModel(Trace_model::Ptr model)
{
    /* Optimization: don't do anything if the model
       is the same. Can happen, for example, when trying
       to move to left when we can't move any lefter.  */
    if (model.get() != contents_->model().get())
    {
        contents_->setModel(model);
        // Emit signal after changing the model, so that receivers can operate
        // on the new canvas.
        emit modelChanged(contents_->model_);

        timeline_->update();
    }
}

Trace_model::Ptr & Canvas::model() const
{
    return contents_->model_;
}

void Canvas::timeSettingsChanged()
{
    timeline_->update();

    emit modelChanged(model());

    QSettings settings;
    settings.setValue("time_unit", Time::unit_name(Time::unit()));
    if (Time::format() == Time::Advanced)
        settings.setValue("time_format", "separated");
    else
        settings.setValue("time_format", "plain");
}

void Canvas::setCursor(const QCursor& c)
{
    contents_->setCursor(c);
}

int Canvas::nearest_lifeline(int y)
{
    int best_l = -1;
    int best_distance = 10000000;
    for(unsigned i = 0; i < contents_->trace_painter->lifeline_position.size(); ++i)
    {
        int distance = abs((int)(contents_->trace_painter->lifeline_position[i] - y));
        if (distance < best_distance)
        {
            best_distance = distance;
            best_l = i;
        }
    }
    return best_l;
}

QPoint Canvas::lifeline_point(int component, const Time& time)
{
    return QPoint(contents_->trace_painter->pixelPositionForTime(time),
                  contents_->trace_painter->lifeline_position[component]);
}

QRect Canvas::boundingRect(int component,
                           const Time& min_time, const Time& max_time)
{
    return contents_->boundingRect(component, min_time, max_time);
}

QPair<Time, Time> Canvas::nearby_range(const Time& time)
{
    Trace_painter *tp = contents_->trace_painter.get();
    int pixel = tp->pixelPositionForTime(time);

    return qMakePair(tp->timeForPixel(pixel-20),
                     tp->timeForPixel(pixel+20));
}

void Canvas::scrollContentsBy(int dx, int dy)
{
    contents_->scrolledBy(dx, dy);
    QScrollArea::scrollContentsBy(dx, dy);
}

void Canvas::resizeEvent(QResizeEvent* event)
{
    // Permit redrawing if only height was changed
    if (updateTimer || event->size().width() != event->oldSize().width())
    {
        // Start a timer which will repaint canvas after 300 msec
        // since last resize event. Thus we prevents repainting
        // while resizing is not finished.
        if (updateTimer) killTimer(updateTimer);
        updateTimer = startTimer(300);
    }

    QScrollArea::resizeEvent(event);

    if (!timeline_) return;
    timeline_->move(viewport()->x(), viewport()->y() + viewport()->height());
}

void Canvas::timerEvent(QTimerEvent * event)
{
    // If user holds mouse button, resizing is not finished.
    if (QApplication::mouseButtons() & Qt::LeftButton)
        return;

    killTimer(updateTimer); updateTimer = 0;

    // Repaint timeline.
    if (timeline_) {
        unsigned height = timeline_->sizeHint().height();
        timeline_->resize(viewport()->width(), height);
        timeline_->repaint();
    }

    // Redraw current model
    if (model().get()) {
        contents_->setModel(model(), true);
        emit modelChanged(model());
    }
}

void Canvas::closeEvent(QCloseEvent *event)
{
    // Stop background drawing
    contents_->trace_painter->setState(Trace_painter::Canceled);
}

void Canvas::addItem(class CanvasItem* item)
{
    contents_->items.push_back(item);
    item->setParent(contents_);
}

void Canvas::setPortableDrawing(bool p)
{
    contents_->portable_drawing = p;
}

Contents_widget::Contents_widget(Canvas* parent)
: QWidget(parent), parent_(parent), paintBuffer(0),
  portable_drawing(false), visir_position((unsigned)-1)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    setFocusPolicy(Qt::NoFocus);

    trace_painter.reset(new Trace_painter());

    painter_timer = new QTimer(this);
    connect(painter_timer, SIGNAL( timeout() ),
        this, SLOT( timerTick() ) );

    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::white);
    p.setColor(QPalette::Foreground, Qt::black);
    setPalette(p);

    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);

    setMouseTracking(true);
}

void Contents_widget::setModel(Trace_model::Ptr model, bool force)
{
    Q_ASSERT(model.get() != 0);

    bool need_redraw = true;
    bool start_in_background = false;
    /* At the moment, 'delta' returns 0 is the time
       unit changed, which is just for our purposed here. */
    if (!force && model_)
    {
        int delta = vis4::delta(*model_, *model);
        need_redraw = (delta != 0);
        start_in_background = !(delta & Trace_model_delta::time_range);
    }

    model_ = model;
    trace_painter->setModel(model_);
    if (!need_redraw) return;

    if (trace_painter->state() == Trace_painter::Canceled)
        return;

    // Stop current drawing
    if (trace_painter->state() == Trace_painter::Active ||
        trace_painter->state() == Trace_painter::Background)
    {
        trace_painter->setState(Trace_painter::Canceled);
        pendingRedraw = true; return;
    }

    // Don't draw trace util canvas is visible
    if (!isVisible()) return;

    // Draw trace
    QApplication::setOverrideCursor(Qt::BusyCursor);
    for (;;) {
        pendingRedraw = false;
        doDrawing(start_in_background);
        if (!pendingRedraw) break;
    }
    QApplication::restoreOverrideCursor();
}

Trace_model::Ptr Contents_widget::model() const
{
    return model_;
}

bool Contents_widget::event(QEvent *event)
{
    // This funnction handles only ToolTip events;
    if (event->type() != QEvent::ToolTip)
        return QWidget::event(event);

    // Prevents mouse events while trace is not painted
    if (trace_geometry.get() == 0) return true;

    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    if (helpEvent->pos().x() < trace_painter->left_margin) {

        // If mouse cursor on component name label,
        // display tip with full componet name.
        int component = trace_geometry->componentLabelAtPos(helpEvent->pos());
        if (component != -1)
        {
            QToolTip::showText(helpEvent->globalPos(),
                model_->component_name(component));
        }
        else
        {
            QToolTip::hideText();
        }

    } else {

        // If mouse cursor on state, display state info tip.
        Canvas::clickTarget target = Canvas::nothingClicked;
        int component = -1;
        State_model* state = 0;
        bool events_near = false;

        targetUnderCursor(helpEvent->pos(), &target, &component, &state, &events_near);
        if (target == Canvas::stateClicked && state && component > -1) {
            QString stateTip;
            stateTip  = tr("State: ")+model_->states().item(state->type)+"\n";
            stateTip += tr("Component: ")+model_->component_name(component)+"\n";
            stateTip += tr("Start: ")+state->begin.toString()+"\n";
            stateTip += tr("End: ")+state->end.toString()+"\n";
            stateTip += tr("Duration:")+(state->end - state->begin).toString();

            QToolTip::showText(helpEvent->globalPos(), stateTip);
        } else {
            QToolTip::hideText();
        }
    }

    return true;
}

void Contents_widget::paintEvent(QPaintEvent* event)
{
    // Draw white background while vis loading the trace
    if (!paintBuffer)
    {
        QPainter painter(this);
        painter.fillRect(0, 0, width(), height(), Qt::white);
        return;
    }

    if (!model_) return;
    if (trace_painter->state() == Trace_painter::Active ||
        trace_painter->state() == Trace_painter::Canceled)
    {
        return;
    }

    QPainter painter(this);

    // Draw paint buffer at the canvas
    if (portable_drawing)
        painter.drawImage(0, 0, *static_cast<QImage*>(paintBuffer));
    else
        painter.drawPixmap(0, 0, *static_cast<QPixmap*>(paintBuffer));

    // Draw outside of pixmap
    int image_height = paintBuffer->height();
    int image_width = paintBuffer->width();
    painter.fillRect(image_width, 0, width(),
                     image_height, Qt::white);
    painter.fillRect(0, image_height, width(),
                     height()-image_height, Qt::white);

    QLinearGradient g(0, 0, trace_painter->left_margin, 0);
    g.setColorAt(0, QColor(150, 150, 150));
    g.setColorAt(1, Qt::white);
    painter.setBrush(g);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, image_height, trace_painter->left_margin, height()-image_height);

    // Draw visir line and baloon tip
    if (visir_position != -1)
    {
        painter.setPen(Qt::red);
        painter.drawLine(visir_position, 0, visir_position, height());

        QColor c("#10e205");
        painter.setBrush(c);
        painter.setPen(Qt::black);

        ballon = drawBaloon(&painter);
    }

    // Draw additional canvas items.
    painter.setClipRect(QRect(trace_painter->left_margin, 0,
        image_width-trace_painter->left_margin, image_height));
    for(unsigned i = 0; i < items.size(); ++i)
    {
        items[i]->draw(painter);
    }
}

void Contents_widget::mouseMoveEvent(QMouseEvent* ev)
{
    // Prevents mouse events while trace is not painted
    if (trace_geometry.get() == 0) return;

    if (ev->x() >= trace_painter->left_margin)
    {
        if (ev->x() != visir_position)
        {
            // Mouse is not on component names area, move visir.
            unsigned old_position = visir_position;
            visir_position = ev->x();

            update(QRect(old_position, 0, 1, height()) |
                   QRect(visir_position, 0, 1, height()));

            update(ballon);
            update(drawBaloon(0));
        }
    }

    Canvas::clickTarget target = Canvas::nothingClicked;
    int component = -1;
    State_model* state = 0;
    bool events_near = false;

    targetUnderCursor(ev->pos(), &target, &component, &state, &events_near);

    emit parent_->mouseMoveEvent(ev, target,
                                 trace_geometry->componentAtPosition(ev->pos()),
                                 trace_painter->timeForPixel(ev->x()));
}

void Contents_widget::wheelEvent(QWheelEvent * event)
{
    if (event->modifiers() != Qt::ShiftModifier)
        return QWidget::wheelEvent(event);

    emit parent_->mouseEvent(event, Canvas::nothingClicked,
        0, 0, model_->min_resolution(), false);
}

void Contents_widget::scrolledBy(int dx, int dy)
{
    update(ballon);
    ballon.adjust(0, -dy, 0, -dy);
    update(ballon);
    update();
}

QSize Contents_widget::minimumSizeHint() const
{
    if (!model_)
        return QSize(300, 200);
    else
    {
        int components_count = model_->visible_components().size();
        return QSize(300,
                     trace_painter->lifeline_stepping
                     * (components_count+1));
    }
}


QSize Contents_widget::sizeHint() const
{
    return minimumSizeHint();
}

QRect Contents_widget::drawBaloon(QPainter* painter)
{
    QString s = trace_painter->timeForPixel(visir_position).toString();
    int x = visir_position + 10;
    int y = parent_->verticalScrollBar()->value() + 15;

    QRect try_on_right = Trace_painter::drawTextBox(s, 0, x, y, -1, trace_painter->text_elements_height);
    if (try_on_right.right() > width()-trace_painter->right_margin)
    {
        // Draw on left.
        return Trace_painter::drawTextBox(s, painter,
                           visir_position - 10 - try_on_right.width(),
                           y, -1, trace_painter->text_elements_height);
    }
    else
    {
        if (painter)
            Trace_painter::drawTextBox(s, painter, x, y, -1, trace_painter->text_elements_height);
        return try_on_right;
    }
}

QRect Contents_widget::boundingRect(int component,
                                    const Time& min_time,
                                    const Time& max_time)
{
    int left = trace_painter->pixelPositionForTime(min_time);
    int right = trace_painter->pixelPositionForTime(max_time);

    int lifeline = model_->lifeline(component); Q_ASSERT(lifeline != -1);
    int y_mid = trace_painter->lifeline_position[lifeline];

    // The event line is text_element_height/2
    // The letter is drawn one pixel above and can take
    // as much as text_height.
    int y_delta = trace_painter->text_elements_height/2 + 1 + trace_painter->text_height;
    left -= trace_painter->text_letter_width;
    right += trace_painter->text_letter_width;

    int y_top = y_mid - y_delta;
    int y_bottom = y_mid + y_delta;

    return QRect(QPoint(left, y_top), QPoint(right, y_bottom));
}

void Contents_widget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
}

void Contents_widget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);

    // Prevents mouse events while trace is not painted
    if (trace_geometry.get() == 0) return;

    Canvas::clickTarget target;
    int component = -1;
    State_model* state = 0;
    bool events_near = false;

    targetUnderCursor(event->pos(), &target, &component, &state, &events_near);

    emit parent_->mouseEvent(event,
                             target,
                             component,
                             state,
                             trace_painter->timeForPixel(event->x()),
                             events_near);
}

void Contents_widget::targetUnderCursor(
    const QPoint& pos, Canvas::clickTarget* target,
    int* component, State_model** state, bool* events_near)
{
    *target = Canvas::nothingClicked;

    if (pos.x() < trace_painter->left_margin)
    {
        if (trace_geometry->clickable_component(pos, *component))
            *target = Canvas::componentClicked;
    }
    else
    {
        *state = trace_geometry->clickable_state(pos);

        if (*state)
        {
            *target = Canvas::stateClicked;
        }
        else
        {
            *target = Canvas::lifelinesClicked;
        }

        *component = trace_geometry->componentAtPosition(pos);
        if (*component != -1)
        {
            int lifeline = model_->lifeline(*component);
            Q_ASSERT(lifeline != -1);

            /* Check if there are events near this point.  */
            // FIXME: this '20' is the area where click on lifeline
            // will be associated with this event. Probably, should
            // be configurable.
            int left = pos.x() - 20;
            if (left < 0)
                left = 0;
            int right = pos.x() + 20;
            int size = trace_geometry->eventsNear[lifeline].size();
            if (right > size)
                right = size;

            for (int i = left; i < right; ++i)
                if (trace_geometry->eventsNear[lifeline][i])
                {
                    *events_near = true;
                    break;
                }
        }
    }
}

void Contents_widget::doDrawing(bool start_in_background)
{
    // Create painter buffer
    int components_count = model_->visible_components().size();
    int height = trace_painter->lifeline_stepping
                            * (components_count+1);

    if (!paintBuffer || paintBuffer->width() != width() || paintBuffer->height() != height)
    {
        if (portable_drawing) {
            QImage * image = new QImage(width(), height, QImage::Format_RGB32);
            trace_painter->setPaintDevice(image);
            delete paintBuffer; paintBuffer = image;
        }
        else {
            QPixmap * pixmap = new QPixmap(width(), height);
            trace_painter->setPaintDevice(pixmap);
            delete paintBuffer; paintBuffer = pixmap;
        }
    }

    // Draw the trace!
    painter_timer->start(1000);
    trace_painter->drawTrace(model_->max_time() - model_->min_time(), start_in_background);
    painter_timer->stop();

    if (trace_painter->state() == Trace_painter::Canceled) return;
    trace_geometry = trace_painter->traceGeometry();

    updateGeometry();
    update();
}

void Contents_widget::timerTick()
{
    if (trace_painter->state() == Trace_painter::Active) {
        trace_painter->setState(Trace_painter::Background);
        painter_timer->start(500);
    }

    if (trace_painter->state() == Trace_painter::Canceled) {
        painter_timer->stop(); return;
    }

    Q_ASSERT(trace_painter->state() == Trace_painter::Background);

    // Repaint canvas with current drawing result.
    repaint();
}

}
