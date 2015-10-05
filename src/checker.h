#ifndef CHECKER_HPP
#define CHECKER_HPP

#include "trace_model.h"

#include <QObject>
#include <QStringList>
#include <set>

namespace vis4 {

class Checker;
typedef boost::shared_ptr<Checker> pChecker;

class Checker : public QObject {

    Q_OBJECT

public: /* static methods */

    static void registerChecker(Checker * checker);

    /** Returns list of all checkers. */
    static QStringList availableCheckers();

    static pChecker createChecker(const QString & checker_name);

private: /* static members */

    static QList<Checker*> checkers;

public: /* methods */

    virtual ~Checker() {}

    virtual QString name() const = 0;

    virtual QString title() const = 0;

    virtual std::set<int> events() const = 0;

    virtual std::set<int> subevents(int event) const = 0;

    /** Creates control widget for the checker. */
    virtual QWidget * widget() = 0;

    virtual bool isReady() const = 0;

    virtual void setModel(const Trace_model::Ptr & model) = 0;

private: /* methods */

    virtual Checker * clone() const = 0;

signals:

    void stateChanged();

};

}
#endif
