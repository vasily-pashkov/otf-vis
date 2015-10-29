
#ifndef TIME_HPP_1116418cda127f283494b6feded63d1e
#define TIME_HPP_1116418cda127f283494b6feded63d1e

#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>

#include <QStringList>

#include <math.h>
#include <typeinfo>
#include <assert.h>
#include <boost/any.hpp>

namespace vis4 { namespace common {

class Time_implementation
{
public:

    virtual Time_implementation * fromString(const QString & time) const = 0;

    virtual QString toString() const = 0;

    virtual Time_implementation* add(const Time_implementation* another) = 0;

    virtual Time_implementation* sub(const Time_implementation* another) = 0;

    virtual Time_implementation* mul(double a) = 0;

    virtual double div(const Time_implementation* another) = 0;

    virtual bool lessThan(const Time_implementation* another) = 0;

    virtual boost::any raw() const = 0;

    virtual Time_implementation* setRaw(const boost::any& any) const = 0;

    virtual ~Time_implementation() {}
};

/** Class-wrapper for various time representation. */
class Time : public boost::less_than_comparable<Time>
{

public: /* static methods */

    enum UnitType { us, ms, sec, min, hour };
    enum Format { Plain, Advanced };

    static const QStringList & units()  { return units_; }

    static int unit()  { return unit_; }
    static void setUnit(int unit) { unit_ = unit; }

    static Format format() { return format_; }
    static void setFormat(Format format) { format_ = format; }

    static QString unit_name(int unit = -1)
    {
        if (unit == -1) unit = unit_;
        return units_[unit];
    }

    static long long unit_scale(int unit = -1)
    {
        if (unit == -1) unit = unit_;
        return scales_[unit];
    }

public: /* static members */

    static QStringList units_;
    static QList<long long> scales_;

    static int unit_;
    static Format format_;

public:
    // Created uninitialized time. Using such instance in any way
    // is a programmer's error.
    // NOTE: most usages will assert thanks to shared_ptr.
    Time();

    Time(const boost::shared_ptr<Time_implementation>& imp) : pimp(imp) {}

    Time(const Time& another) : pimp(another.pimp) {}

    Time& operator=(const Time& another)
    {
        pimp = another.pimp;
        return *this;
    }

    bool isNull() const
    {
        return pimp.get() == 0;
    }

    Time operator+(const Time& another) const
    {
        assert(typeid(*pimp.get()) == typeid(*another.pimp.get()));
        boost::shared_ptr<Time_implementation> n(
            pimp->add(another.pimp.get()));
        return Time(n);
    }

    Time operator-(const Time& another) const
    {
        assert(typeid(*pimp.get()) == typeid(*another.pimp.get()));
        boost::shared_ptr<Time_implementation> n(
            pimp->sub(another.pimp.get()));
        return Time(n);
    }

    Time operator*(double a) const
    {
        boost::shared_ptr<Time_implementation> n(
            pimp->mul(a));
        return Time(n);
    }

    Time operator/(double a) const
    {
        boost::shared_ptr<Time_implementation> n(
            pimp->mul(1/a));
        return Time(n);
    }

    double operator/(const Time& another) const
    {
        assert(typeid(*pimp.get()) == typeid(*another.pimp.get()));
        return pimp->div(another.pimp.get());
    }

    bool operator<(const Time& another) const
    {
        assert(pimp.get());
        assert(another.pimp.get());
        assert(typeid(*pimp.get()) == typeid(*another.pimp.get()));
        return pimp->lessThan(another.pimp.get());
    }

    bool operator==(const Time& another) const
    {
        if (pimp.get() == 0) return another.isNull();
        if (another.isNull()) return false;

        /* We might add another method to Time_implementation to
           compare this directly, but time equality comparison
           is not done often, so we don't care about performance.  */
        return !(*this < another) && !(another < *this);
    }

    bool operator!=(const Time& another) const
    {
        return !(*this == another);
    }

    bool sameType(const Time& another) const
    {
        return typeid(*pimp.get()) == typeid(*another.pimp.get());
    }

    Time fromString(const QString & time) const
    {
        Q_ASSERT(pimp.get() != 0);
        boost::shared_ptr<Time_implementation> p(pimp->fromString(time));
        return Time(p);
    }

    QString toString(bool also_unit = false) const
    {
        QString time_str = pimp->toString();
        if (also_unit && format() == Plain)
            time_str += " " + unit_name(unit());

        return time_str;
    }

    boost::any raw() const
    {
        return pimp->raw();
    }

    Time setRaw(const boost::any& any) const
    {
        boost::shared_ptr<Time_implementation> p(pimp->setRaw(any));
        return Time(p);
    }

    static Time scale(const Time& point1, const Time& point2, double pos)
    {
        return point1 + (point2-point1)*pos;
    }

private:

    boost::shared_ptr<Time_implementation> pimp;

};

template<class T>
class Scalar_time_implementation_base : public Time_implementation
{
public:
    Scalar_time_implementation_base(T time) : time(time)
    {}

    virtual Time_implementation* add(const Time_implementation* xanother)
    {
        const Scalar_time_implementation_base* another =
            static_cast<const Scalar_time_implementation_base*>(xanother);
        return construct(time + another->time);
    }

    virtual Time_implementation* sub(const Time_implementation* xanother)
    {
        const Scalar_time_implementation_base* another =
            static_cast<const Scalar_time_implementation_base*>(xanother);
        return construct(time - another->time);
    }

    virtual Time_implementation* mul(double a)
    {
        return construct(T(floor(time*a+0.5)));
    }

    virtual double div(const Time_implementation* xanother)
    {
        const Scalar_time_implementation_base* another =
            static_cast<const Scalar_time_implementation_base*>(xanother);
        return double(time)/another->time;
    }

    virtual bool lessThan(const Time_implementation* xanother)
    {
        const Scalar_time_implementation_base* another =
            static_cast<const Scalar_time_implementation_base*>(xanother);
        return time < another->time;
    }

    boost::any raw() const
    {
        return time;
    }

    Scalar_time_implementation_base* setRaw(const boost::any& any) const
    {
        return construct(boost::any_cast<T>(any));
    }

    virtual Scalar_time_implementation_base* construct(T time) const = 0;

protected:
    T time;
};

template<class T>
class Scalar_time_implementation : public Scalar_time_implementation_base<T>
{
public:
    Scalar_time_implementation(T time)
    : Scalar_time_implementation_base<T>(time)
    {}

    virtual Time_implementation * fromString(const QString & time) const
    {
        return new Scalar_time_implementation(*this);
    }

    virtual QString toString() const
    {
        return QString::number(
            Scalar_time_implementation_base<T>::time/Time::unit_scale());
    }

    virtual Scalar_time_implementation* construct(T time) const
    {
        return new Scalar_time_implementation(time);
    }
};

template<class T>
Time scalar_time(T t)
{
    boost::shared_ptr<Time_implementation> p(
        new Scalar_time_implementation<T>(t));
    return Time(p);
}

inline Time distance(const Time & t1, const Time t2)
{
    return (t1 > t2) ? t1-t2 : t2-t1;
}

// возвращает время в микросекундах или -1, если не ясно на основе какого типа построено время
unsigned long long getUs(const Time & t);

}} // namespaces

#endif
