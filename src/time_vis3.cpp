#include "time_vis3.h"
#include <QApplication>

namespace vis4 {
    namespace common {

QStringList Time::units_;
Time::Format Time::format_;
QList<long long> Time::scales_;

int Time::unit_ = -1;

namespace
{
    QString tr(const char* s)
    {
        //В файле перевода vis'а контекст перевода единиц времени связан с vis4::Time.
        //Возможно, его стоит обновить, потому что сейчас Time находится в namespace vis4::common
	//Пока такой хак делаем.
	QString origin = QString(s);
        QString translate = qApp->translate("vis4::Time", s);

	if (origin == translate)
	{
            translate = qApp->translate("vis4::common::Time", s);
	}

	return translate;
    }
}

Time::Time()
{
    if (unit_ != -1) return;

    // If we create time object  for the first time,
    // we have to initialize units.

    units_ << tr("us"); scales_ << 1;
    units_ << tr("ms"); scales_ << 1000;
    units_ << tr("s"); scales_ << 1000000;
    units_ << tr("m"); scales_ << 60ll*1000000;
    units_ << tr("h"); scales_ << 60ll*60ll*1000000;

    unit_ = 0; format_ = Plain;
}

unsigned long long getUs(const Time & t)
{
    try
    {
	unsigned long long us = boost::any_cast<unsigned long long>(t.raw());
	return us;
    }
    catch (boost::bad_any_cast &)
    {
	try
	{
	    long long us = boost::any_cast<long long>(t.raw());
	    return (unsigned long long)us;
	}
	catch (boost::bad_any_cast &)
	{
	    try
	    {
	       unsigned long us = boost::any_cast<unsigned long>(t.raw());
	       return (unsigned long long)us;
	    }
	    catch (boost::bad_any_cast &)
	    {
		try
		{
		    long us = boost::any_cast<long>(t.raw());
		    return (unsigned long long)us;
		}
		catch (boost::bad_any_cast &)
		{
		    try
		    {
			unsigned int us = boost::any_cast<unsigned int>(t.raw());
			return (unsigned long long)us;
		    }
		    catch (boost::bad_any_cast &)
		    {
			try
			{
			    int us = boost::any_cast<int>(t.raw());
			    return (unsigned long long)us;
			}
			catch (boost::bad_any_cast &)
			{
			    qWarning("Warning! Unknown time implementation.");
			    return (unsigned long long)-1;
			}
		    }
		}
	    }
	}
    }
}

}} // namespaces
