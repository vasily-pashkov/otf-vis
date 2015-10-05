#include "timeline.h"
#include "trace_painter.h"
#include <timeunit_control.h>

namespace vis4 {

Timeline::Timeline(QWidget* parent, Trace_painter * painter)
    : QWidget(parent), tp(painter)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    timeUnitControl = new common::TimeUnitControl(this);
    connect(timeUnitControl, SIGNAL( timeSettingsChanged() ),
        this, SIGNAL( timeSettingsChanged() ));
}

QSize Timeline::sizeHint() const
{
    return QSize(-1, Trace_painter::timeline_text_top + QFontMetrics(font()).height());
}

QSize Timeline::minimumSizeHint() const
{
    return sizeHint();
}

void Timeline::paintEvent(QPaintEvent* e)
{
    if (!tp) return;

    // Draw time notches and labels.
    QPainter p(this);
    tp->drawTimeline(&p, 0, 0);

    // Draw time unit control.
    QSize size = timeUnitControl->sizeHint();
    timeUnitControl->resize(size);
    timeUnitControl->move(3, (height()-size.height()) / 2);

    QWidget::paintEvent(e);
}
}
