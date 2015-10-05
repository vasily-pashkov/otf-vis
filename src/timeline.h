#ifndef TIMELINE_HPP_VP_2006_03_21
#define TIMELINE_HPP_VP_2006_03_21

#include <QWidget>

namespace vis4 {

class Trace_painter;

namespace common {
    class TimeUnitControl;
}

class Timeline : public QWidget
{

    Q_OBJECT

public:

    Timeline(QWidget* parent, Trace_painter * painter);

public: // QWidget overides

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

signals:

    void timeSettingsChanged();

protected:

    void paintEvent(QPaintEvent* e);

private:

    Trace_painter *tp;
    common::TimeUnitControl * timeUnitControl;

};

}
#endif
