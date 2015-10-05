#ifndef SELECTION_HPP
#define SELECTION_HPP

#include <QVector>
#include <QList>
#include <QVariant>
#include <QHash>

namespace vis4 {
    namespace common {

/** Class implements support for containing and filtering
   a set of items. Items may be placed in hierarchy. */
class Selection {

public: /* static constants */

    static const int ROOT = -1;

public: /* methods */

    bool hasSubitems() const;

/** @defgroup operations Methods for operation on items. */
/// @{

    int totalItemsCount() const;

    int addItem(const QString & title, int parent = ROOT);
    const QString & item(int link) const;

    bool hasChildren(int parent) const;
    int itemsCount(int parent = ROOT) const;
    const QList<int> & items(int parent = ROOT) const;

    int enabledCount(int parent = ROOT) const;
    const QList<int> enabledItems(int parent = ROOT) const;

    int itemLink(int index, int parent = ROOT) const;
    int itemLink(const QString & title, int parent = ROOT) const;

    int itemParent(int link) const;
    int itemIndex(int link) const;

    QVariant itemProperty(int link, const QString & property) const;
    void setItemProperty(int link, const QString & property, const QVariant & value);

    bool isEnabled(int link) const;
    bool isEnabled(int index, int parent) const;

    void setEnabled(int link, bool enabled);
    void setEnabled(int index, int parent, bool enabled);

    Selection & enableAll(int parent = ROOT, bool recursive = false);
    Selection & disableAll(int parent = ROOT, bool recursive = false);

    void clear();

/// @}

/** @defgroup comparison Comparison operators overload. */
/// @{

    bool operator==(const Selection & other) const;
    bool operator!=(const Selection & other) const;

/// @}

    Selection operator&(const Selection & other) const;

private: /* members */

    QVector<QString> items_;
    QVector<bool> filter_;

    QVector< QHash<QString, QVariant> > properties_;

    QVector< QList<int> > links_;
    QVector<int> parents_;
    QList<int> topLevelItems_;

};

}} // namespaces

#endif
