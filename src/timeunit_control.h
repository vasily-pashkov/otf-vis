#ifndef TIMEUNIT_CONTROL_HPP
#define TIMEUNIT_CONTROL_HPP

#include <QWidget>

class QStyleOptionComboBox;
class QMenu;

namespace vis4 {
    namespace common {

/** Class for dislpay and change time unit and format settings. */
class TimeUnitControl : public QWidget
{

    Q_OBJECT

public: /* methods */

    TimeUnitControl(QWidget * parent);

    QSize sizeHint() const;

signals:

    void timeSettingsChanged();

protected: /* methods */

    void paintEvent(QPaintEvent * event);

    void mousePressEvent(QMouseEvent *e);

    void initStyleOption(QStyleOptionComboBox *option) const;

private slots:

    void menuActionTriggered(QAction * action);

private: /* members */

    QMenu * menu;

};

}} // namespaces

#endif
