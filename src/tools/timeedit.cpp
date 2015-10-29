#include "timeedit.h"
#include <boost/any.hpp>

namespace vis4 { namespace common {

TimeEdit::TimeEdit(QWidget * parent) : QAbstractSpinBox(parent),
	long_long_time(false), unsigned_long_long_time(false), mNoError(true), mEdited(false)
{
    connect(lineEdit(), SIGNAL( textEdited(const QString &) ),
        this, SLOT( valueChanged(const QString &) ));

    connect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

    mValidator = 0;
    mValidatorRegExp.setPattern("(\\d{1,2}:){1,2}(\\d{1,2}\\.)?\\d+");
    onTimeSettingsChanged();

    mErrorPalette.setColor(QPalette::Base, Qt::red);
    mNormalPalette = palette();

    // пока не задано время, редактирование запрещено
    setEnabled(false);
}

void TimeEdit::stepBy(int steps)
{
    if (cur_time_.isNull())
    {
	return;
    }

    Time t = cur_time_.fromString(lineEdit()->text());
    int u = Time::unit();

    long long newUs = (long long)getUs(t);
    newUs += Time::unit_scale(u)*steps;
    if (newUs < 0)
    {
	// проверим, чтобы не уйти в минус
	return;
    }

    if (long_long_time)
    {
	t = t.setRaw(newUs);
    }
    else if (unsigned_long_long_time)
    {
	t = t.setRaw((unsigned long long)newUs);
    }
    else
    {
        t = t.setRaw((int)newUs);
    }

    if (!validate(t))
    {
	return;
    }

    setTime(t);

    emit editingFinished();
    emit timeChanged(t);
}

Time TimeEdit::time() const
{
    if (cur_time_.isNull()) return Time();

    return cur_time_;
}

void TimeEdit::setTime(const Time & time)
{
    setEnabled(false);

    Q_ASSERT(!time.isNull());

    if (cur_time_.isNull())
    {
	cur_time_ = time;
	boost::any raw = time.raw();
	if (boost::any_cast<long long>(&raw))
	    long_long_time = true;
	else if (boost::any_cast<unsigned long long>(&raw))
	    unsigned_long_long_time = true;
	else if (!boost::any_cast<int>(&raw))
	    qFatal("Error: Invalid underlying kind of time");
    }
    else
    {
	cur_time_ = time;
    }
    lineEdit()->setText(time.toString());
    if (!mNoError)
    {
	setPalette(mNormalPalette);
	mNoError = true;
    }
    mEdited = false;

    setEnabled(true);
}

void TimeEdit::valueChanged(const QString & value)
{
    if (!(Time::format() != Time::Advanced || mValidatorRegExp.exactMatch(value)) ||
	!validate(cur_time_.fromString(value)))
    {
	setPalette(mErrorPalette);
	mNoError = false;
    }
    else
    {
	setPalette(mNormalPalette);
	mNoError = true;
	cur_time_ = cur_time_.fromString(value);
    }
    mEdited = true;

    emit textChanged();
}

bool TimeEdit::validate(const Time & t) const
{
    if (t.isNull()) return false;
    if (!min_time_.isNull() && min_time_ > t) return false;
    if (!max_time_.isNull() && max_time_ < t) return false;

    return true;
}

void TimeEdit::onEditingFinished()
{
    if (cur_time_.isNull() || !mEdited)
    {
	return;
    }
    mEdited = false;

    QString value = lineEdit()->text();

    Time t = cur_time_.fromString(value);

    if (!validate(t) || !mNoError)
    {
	// возвращаемся в корректное состояние
	setTime(cur_time_);
	return;
    }
    cur_time_ = t;

    emit timeChanged(t);
}

void TimeEdit::onTimeSettingsChanged()
{
	if (mValidator) delete mValidator;
	mValidator = 0;
	switch (Time::format())
	{
		case Time::Plain:
		{
			unsigned long long maximum = 24 * Time::unit_scale(Time::hour);
			maximum /= Time::unit_scale();
			mValidator = new QIntValidator(0, maximum, this);
			break;
		}
		case Time::Advanced:
			mValidator = new QRegExpValidator(mValidatorRegExp, this);
			break;
		default:;
	}
	lineEdit()->setValidator(mValidator);
}

QAbstractSpinBox::StepEnabled TimeEdit::stepEnabled () const
{
    if (!cur_time_.isNull() && getUs(cur_time_) < (unsigned long long)Time::unit_scale())
    {
	return StepUpEnabled;
    }
    return StepUpEnabled | StepDownEnabled;
}

}} // namespaces
