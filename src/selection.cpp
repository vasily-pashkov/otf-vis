#include "selection.h"

namespace vis4 { namespace common {

const int Selection::ROOT;

int Selection::addItem(const QString & title, int parent)
{
    int link = items_.size();
    items_ << title; filter_ << true;
    properties_ << QHash<QString, QVariant>();
    links_ << QList<int>(); parents_ << parent;

    if (parent == ROOT)
    {
        topLevelItems_ << link;
    }
    else
    {
        Q_ASSERT(parent < items_.size());
        links_[parent] << link;
    }

    return link;
}

const QString & Selection::item(int link) const
{
    Q_ASSERT(link < items_.size());
    return items_[link];
}

const QList<int> & Selection::items(int parent) const
{
    Q_ASSERT(parent < items_.size());
    return (parent == ROOT) ? topLevelItems_ : links_[parent];
}

int Selection::itemLink(int index, int parent) const
{
    if (parent == ROOT) {
        Q_ASSERT(index < topLevelItems_.size());
        return topLevelItems_[index];
    }

    Q_ASSERT(parent < items_.size());
    Q_ASSERT(index < links_[parent].size());
    return links_[parent][index];
}

int Selection::itemLink(const QString & title, int parent) const
{
    Q_ASSERT(parent < items_.size());

    foreach (int link, items(parent))
        if (items_[link] == title) return link;

    return ROOT;
}

int Selection::itemParent(int link) const
{
    if (link == Selection::ROOT) return Selection::ROOT;

    Q_ASSERT(link < items_.size());
    return parents_[link];
}

int Selection::itemIndex(int link) const
{
    Q_ASSERT(link < items_.size());
    int parent = itemParent(link);
    if (parent == ROOT)
        return topLevelItems_.indexOf(link);
    else
        return links_[parent].indexOf(link);
}

const QList<int> Selection::enabledItems(int parent) const
{
    QList<int> items;

    if (parent == ROOT) {
        for (int i = 0; i < topLevelItems_.size(); i++)
        {
            if (filter_[topLevelItems_[i]])
                items << itemLink(i);
        }
    }
    else
    {
        Q_ASSERT(parent < items_.size());
        for (int i = 0; i < links_[parent].size(); i++)
        {
            if (filter_[links_[parent][i]])
                items << itemLink(i, parent);
        }
    }

    return items;
}


QVariant Selection::itemProperty(int link, const QString & property) const
{
   Q_ASSERT(link < items_.size());

   if (!properties_[link].contains(property))
        return QVariant();

   return properties_[link][property];
}

void Selection::setItemProperty(int link, const QString & property, const QVariant & value)
{
   Q_ASSERT(link < items_.size());
   properties_[link][property] = value;
}

bool Selection::hasSubitems() const
{
    return items_.size() > topLevelItems_.size();
}

bool Selection::hasChildren(int parent) const
{
    Q_ASSERT(parent < items_.size());
    return items(parent).size();
}

int Selection::itemsCount(int parent) const
{
    Q_ASSERT(parent < items_.size());
    return items(parent).size();
}

int Selection::totalItemsCount() const
{
    return items_.size();
}

bool Selection::isEnabled(int link) const
{
    Q_ASSERT(link < items_.size());
    return filter_[link];
}

bool Selection::isEnabled(int index, int parent) const
{
    return isEnabled(itemLink(index, parent));
}

void Selection::setEnabled(int link, bool enabled)
{
    Q_ASSERT(link < items_.size());
    filter_[link] = enabled;

    int parent = parents_[link];
    if (!enabled && parent != ROOT && filter_[parent])
    {
        for (int i = 0; i < itemsCount(parent); i++)
            if (filter_[itemLink(i, parent)]) return;

        setEnabled(parent, false); return;
    }

    if (enabled) {
        for (int i = 0; i < itemsCount(link); i++)
            if (filter_[itemLink(i, link)]) return;

        for (int i = 0; i < itemsCount(link); i++)
            setEnabled(itemLink(i, link), true);
        return;
    }
}

void Selection::setEnabled(int index, int parent, bool enabled)
{
    setEnabled(itemLink(index, parent), enabled);
}

int Selection::enabledCount(int parent) const
{
    Q_ASSERT(parent < items_.size());

    int count = 0;
    foreach (int  link, items(parent))
        if (filter_[link]) count++;

    return count;
}

Selection & Selection::enableAll(int parent, bool recursive)
{
    QList<int> queue = items(parent);
    while (!queue.isEmpty()) {
        int link = queue.takeFirst();
        filter_[link] = true;

        if (recursive)
            queue << links_[link];
    }

    return *this;
}

Selection & Selection::disableAll(int parent, bool recursive)
{
    QList<int> queue = items(parent);
    while (!queue.isEmpty()) {
        int link = queue.takeFirst();
        filter_[link] = false;

        if (recursive)
            queue << links_[link];
    }

    return *this;
}

void Selection::clear()
{
    items_.clear();
    properties_.clear();
    filter_.clear();
    links_.clear();
    parents_.clear();
    topLevelItems_.clear();
}

bool Selection::operator==(const Selection & other) const
{
    Q_ASSERT(items_.size() == other.items_.size());
    return filter_ == other.filter_;
}

bool Selection::operator!=(const Selection & other) const
{
    return !operator==(other);
}

Selection Selection::operator&(const Selection & other) const
{
    Q_ASSERT(items_.size() == other.items_.size());
    Selection result(*this);
    for (int i = 0; i < filter_.size(); i++)
        result.filter_[i] = filter_[i] && other.filter_[i];

    return result;
}

}} // namespaces
