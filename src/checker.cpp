
#include "checker.h"

namespace vis4 {
QList<Checker*> Checker::checkers;

void Checker::registerChecker(Checker * checker)
{
    checkers << checker;
}

QStringList Checker::availableCheckers()
{
    QStringList result;
    for (int i = 0; i < checkers.size(); ++i)
        result << checkers[i]->name();
    return result;
}

pChecker Checker::createChecker(const QString & checker_name)
{
    for (int i = 0; i < checkers.size(); ++i)
    {
        if (checkers[i]->name() == checker_name)
            return pChecker(checkers[i]->clone());
    }

    qWarning("Checker::createChecker: Checker \"%s\" is unsupported",
        (const char *)checker_name.toLocal8Bit());
    return pChecker();
}

}
