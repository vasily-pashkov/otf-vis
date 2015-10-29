#ifndef TIMEEDIT_HPP
#define TIMEEDIT_HPP

#include "time_vis3.h"

#include <QAbstractSpinBox>
#include <QLineEdit>

namespace vis4 { namespace common {

class TimeEdit : public QAbstractSpinBox {

    Q_OBJECT

public: /* methods */

    TimeEdit(QWidget * parent = 0);

    StepEnabled stepEnabled () const;
    void stepBy(int steps);

    Time time() const;
    /** Setting time to edit. Emits a signal timeChanged(). */
    void setTime(const Time & time);

    void setMaximum(const Time & max_time);
    void setMinimum(const Time & min_time);

public slots:

    void onTimeSettingsChanged();

protected:

    void focusInEvent(QFocusEvent* event);
    void showEvent(QShowEvent* event);

    bool validate(const Time & t) const;

signals:

    void focusIn();
    void timeChanged(Time);
    void textChanged();

private slots:

    void valueChanged(const QString & value);
    void onEditingFinished();

private: /* variables */

    bool long_long_time;
    bool unsigned_long_long_time;

    Time max_time_;
    Time min_time_;
    Time cur_time_;

    QRegExp mValidatorRegExp;
    QValidator* mValidator;
    bool mNoError;
    bool mEdited;
    QPalette mNormalPalette;
    QPalette mErrorPalette;
};


inline void TimeEdit::setMaximum(const Time & max_time)
{
    max_time_ = max_time;
}

inline void TimeEdit::setMinimum(const Time & min_time)
{
    min_time_ = min_time;
}

inline void TimeEdit::focusInEvent(QFocusEvent* event)
{
    QAbstractSpinBox::focusInEvent(event);
    emit focusIn();
}

inline void TimeEdit::showEvent(QShowEvent* event)
{
    // bypass QAbstractSpinBox::showEvent()
    QWidget::showEvent(event);
}

}} // namespaces

#endif
