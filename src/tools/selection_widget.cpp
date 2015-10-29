#include "selection_widget.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QApplication>
#include <QScrollBar>
#include <QMouseEvent>

uint qHash(const QPair<int, int> & pair)
{
    return pair.first + (pair.second << 16);
}

namespace vis4 { namespace common {

class TreeViewWithoutDoubleClickExpanding : public QTreeView {

public:

    TreeViewWithoutDoubleClickExpanding(QWidget * parent)
        : QTreeView(parent)
    {
    }

    void mouseDoubleClickEvent(QMouseEvent * event)
    {
        if (!viewport()->rect().contains(event->pos()))
            return;

        QModelIndex index = indexAt(event->pos());
        if (!index.isValid()) return;

        emit doubleClicked(index);
    }

};


class SelectionWidgetModel : public QAbstractItemModel {

public: /*methods*/

    SelectionWidgetModel(QObject * parent = 0, int flags = 0)
        : QAbstractItemModel(parent), flags_(flags) {}

    void setSelection(const Selection & base, const Selection & current)
    {
        Q_ASSERT(base.totalItemsCount() == current.totalItemsCount());

        frozen.clear();
        for (int i = 0; i < base.totalItemsCount(); ++i)
            if (!base.isEnabled(i)) frozen << i;

        selection_ = current;

        // Emit signal to update itemView
        emit layoutChanged();
    }

    const Selection & selection() const
    {
        return selection_;
    }

    void enableAll(const QModelIndex & parent)
    {
        int parentLink = parent.isValid() ? (int)parent.internalId() : Selection::ROOT;

        foreach(int link, selection_.items(parentLink))
            if (!frozen.contains(link)) selection_.setEnabled(link, true);

        int children_count = selection_.itemsCount(parentLink);
        if (parentLink == Selection::ROOT)
            emit dataChanged(index(0, 0, parent), index(children_count-1, 0, parent));
        else
            emit dataChanged(parent, index(children_count-1, 0, parent));
    }

    void disableAll(const QModelIndex & parent)
    {
        int parentLink = parent.isValid() ? (int)parent.internalId() : Selection::ROOT;

        foreach(int link, selection_.items(parentLink))
            if (!frozen.contains(link)) selection_.setEnabled(link, false);

        int children_count = selection_.itemsCount(parentLink);
        if (parentLink == Selection::ROOT)
            emit dataChanged(index(0, 0, parent), index(children_count-1, 0, parent));
        else
            emit dataChanged(parent, index(children_count-1, 0, parent));
    }

public: /* overloaded item model methods */

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const
    {
        if (column != 0) return QModelIndex();

        int parentLink = parent.isValid() ? (int)parent.internalId() : Selection::ROOT;
        if (selection_.itemsCount(parentLink) <= row) return QModelIndex();

        return createIndex(row, column, selection_.itemLink(row, parentLink));
    }

    QModelIndex parent(const QModelIndex & index) const
    {
        int link = (int)index.internalId();
        int parentLink = selection_.itemParent(link);
        if (parentLink == Selection::ROOT) return QModelIndex();
        return createIndex(selection_.itemIndex(parentLink), 0, parentLink);
    }

    int rowCount(const QModelIndex & parent = QModelIndex()) const
    {
        int parentLink = parent.isValid() ? (int)parent.internalId() : Selection::ROOT;
        return selection_.itemsCount(parentLink);
    }

    int columnCount(const QModelIndex & parent = QModelIndex()) const
    {
        return 1;
    }

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid()) return QVariant();

        int link = (int)index.internalId();
        if (role == Qt::DisplayRole)
            return selection_.item(link);

        if (role == Qt::CheckStateRole) {
            if (!selection_.isEnabled(link))
                return Qt::Unchecked;

            if (selection_.hasChildren(link) &&
                selection_.enabledCount(link) != selection_.itemsCount(link))
            {
                return Qt::PartiallyChecked;
            }

            return Qt::Checked;
        }

        if (role == Qt::BackgroundRole && flags_ == SelectionWidget::COMPONENTS_SELECTOR)
        {
            Q_ASSERT(selection_.itemProperty(0, "current_parent").isValid());
            if (selection_.itemProperty(0, "current_parent").toInt() != link)
            {
                return QVariant();
            }

            return QApplication::palette().brush(QPalette::Active, QPalette::Highlight);
        }

        if (role == Qt::ForegroundRole)
        {

            if (flags_ == SelectionWidget::COMPONENTS_SELECTOR)
                return QVariant();

            int parentLink = selection_.itemParent(link);

            if (parentLink != Selection::ROOT && !selection_.isEnabled(parentLink))
                return QApplication::palette().brush(QPalette::Disabled, QPalette::Text);

            if (frozen.contains(link))
                return QApplication::palette().brush(QPalette::Disabled, QPalette::Text);
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex & index) const
    {
        if (!index.isValid()) return 0;

        int link = (int)index.internalId();
        if (frozen.contains(link)) return 0;

        if (flags_ == SelectionWidget::COMPONENTS_SELECTOR &&
            selection_.itemParent(link) == Selection::ROOT)
        {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole)
    {
        if (!index.isValid()) return false;
        if (role != Qt::CheckStateRole) return false;

        int link = (int)index.internalId();
        selection_.setEnabled(link, value.toInt() != Qt::Unchecked);

        int parentLink = selection_.itemParent(link);
        if (parentLink == Selection::ROOT)
            emit dataChanged(index, index);
        else {
            int children_count = selection_.itemsCount(parentLink);
            emit dataChanged(index.parent(), this->index(children_count-1,
                0, index.parent()));
        }
        return true;
    }

private: /* variables */

    Selection selection_;
    QSet<int> frozen;
    bool flags_;

};

SelectionWidget::SelectionWidget(QWidget* parent, int flags)
: QWidget(parent), flags_(flags), minimizeSize_(false)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    view = new TreeViewWithoutDoubleClickExpanding(this);
    view->header()->hide();
    view->setRootIsDecorated(false);
    view->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(view);

    viewModel = new SelectionWidgetModel(this, flags);
    view->setModel(viewModel);

    QHBoxLayout* buttons = new QHBoxLayout();
    mainLayout->addLayout(buttons);
    buttons->addStretch();

    showAll = new QPushButton(tr("Show All"), this);
    showAll->setObjectName("show_all_button");
    buttons->addWidget(showAll);

    hideAll = new QPushButton(tr("Hide All"), this);
    hideAll->setObjectName("hide_all_button");
    buttons->addWidget(hideAll);

    connect(showAll, SIGNAL(clicked(bool)),
            this, SLOT(showOrHideAll()));

    connect(hideAll, SIGNAL(clicked(bool)),
            this, SLOT(showOrHideAll()));

    connect(viewModel, SIGNAL( dataChanged(const QModelIndex &,
        const QModelIndex &) ), this, SLOT( emitSelectionChanged() ));

    connect(view, SIGNAL( doubleClicked(const QModelIndex &) ),
        this, SLOT( slotDoubleClick(const QModelIndex &) ));

}

void SelectionWidget::minimizeSize(bool enable)
{
    minimizeSize_ = enable;
}

QSize SelectionWidget::sizeHint() const
{
    return (minimizeSize_) ? minimumSizeHint() : QWidget::sizeHint();
}

void SelectionWidget::initialize(const Selection & base, const Selection & current)
{
    view->setRootIsDecorated(current.hasSubitems());
    viewModel->setSelection(base, current);

    bool selection_empty = (current.totalItemsCount() == 0);
    showAll->setEnabled(!selection_empty);
    hideAll->setEnabled(!selection_empty);

    // Expand current parent component
    if (flags_ == SelectionWidget::COMPONENTS_SELECTOR)
    {
        QList<int> path;

        int link = current.itemProperty(0, "current_parent").toInt();
        while (link != -1)
        {
            path << current.itemIndex(link);
            link = current.itemParent(link);
        }

        view->collapseAll();
        QModelIndex index; view->expand(index);
        while (!path.isEmpty())
        {
            index = viewModel->index(path.takeLast(), 0, index);
            view->expand(index);
        }
    }
}

void SelectionWidget::initialize(const Selection & selection)
{
    Selection unfiltered = selection;
    unfiltered.enableAll(Selection::ROOT, true);
    initialize(unfiltered, selection);
}

const Selection & SelectionWidget::selection() const
{
    return viewModel->selection();
}

void SelectionWidget::showOrHideAll()
{
    QModelIndex current = view->currentIndex();
    QModelIndex parent = current.isValid() ? current.parent() : QModelIndex();

    if (sender()->objectName() == "show_all_button")
        viewModel->enableAll(parent);
    if (sender()->objectName() == "hide_all_button")
        viewModel->disableAll(parent);
}

void SelectionWidget::emitSelectionChanged()
{
    emit selectionChanged(viewModel->selection());
}

void SelectionWidget::saveState(QSettings & settings) const
{
    QStringList list;

    QList<QModelIndex> queue; queue << QModelIndex();
    while (!queue.isEmpty())
    {
        QModelIndex index = queue.takeFirst();

        if (index.isValid())
        {
            QString title = viewModel->data(index, Qt::DisplayRole).toString();
            bool enabled = viewModel->data(index, Qt::CheckStateRole).toInt() != Qt::Unchecked;
            bool expanded = view->isExpanded(index);

            list << title << (enabled ? "enabled" : "disabled")
                << (expanded ? "expanded" : "collapsed");
        }
        list << QString::number(viewModel->rowCount(index));

        for(int i = viewModel->rowCount(index)-1; i >= 0; i--)
            queue.prepend(viewModel->index(i, 0, index));
    }

    settings.setValue("items", list);
}

void SelectionWidget::restoreState(QSettings & settings)
{
    if (!settings.contains("items"))
    {
        if (flags_ == COMPONENTS_SELECTOR)
            view->setExpanded(viewModel->index(0,0), true);

        return;
    }

    // Drop old config format
    if (settings.contains("subitems"))
    {
        settings.remove("subitems");
        return;
    }

    QStringList list = settings.value("items").toStringList();

    QList<int> counters; counters << list.takeFirst().toInt();
    QList<QModelIndex> parents;  parents << QModelIndex();
    QList<bool> filters; QList<bool> expansion;
    while (!parents.isEmpty())
    {
        QModelIndex parent = parents.first();
        if (counters.first() == 0)
        {
            if (parent.isValid())
            {
                int state = filters.takeFirst() ? Qt::Checked : Qt::Unchecked;
                viewModel->setData(parent, state, Qt::CheckStateRole);
                view->setExpanded(parent, expansion.takeFirst());
            }

            parents.pop_front(); counters.pop_front();
            continue;
        }

        QString name = list.takeFirst();
        bool enabled = list.takeFirst() == "enabled";
        bool expanded = list.takeFirst() == "expanded";
        int count = list.takeFirst().toInt();

        counters.first()--;

        bool found = false;
        for (int i = 0; i < viewModel->rowCount(parent); i++)
        {
            QModelIndex index = viewModel->index(i, 0, parent);
            if (name != viewModel->data(index, Qt::DisplayRole).toString()) continue;

            filters.prepend(enabled);
            expansion.prepend(expanded);

            parents.prepend(index);
            counters.prepend(count);
            found = true; break;
        }

        if (!found)
        {
            int skip_count = count;
            while (skip_count > 0)
            {
                list.pop_front(); list.pop_front(); list.pop_front();
                skip_count += list.takeFirst().toInt();
            }
        }
    }
}

void SelectionWidget::slotDoubleClick(const QModelIndex & index)
{
    emit itemDoubleClicked((int)index.internalId());
}

}} // namespaces
