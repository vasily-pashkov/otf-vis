#include "param_select.h"
#include "param_select_delegate.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QApplication>
#include <QScrollBar>
#include <QMouseEvent>
#include <QItemDelegate>

namespace vis3 { namespace stand {

using vis3::common::Selection;

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

class ParamSelectionWidgetModel : public QAbstractItemModel {

public: /*methods*/

    ParamSelectionWidgetModel(QObject * parent = 0)
        : QAbstractItemModel(parent) {}

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
        if (!hasIndex(row, column, parent))
            return QModelIndex();

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
          return 2;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole){
            if (section == 0){
                return QApplication::translate("ParamSelectionWidgetModel","Parameter");
            }
            else if (section == 1){
                return QApplication::translate("ParamSelectionWidgetModel","Color");
            }
        }
        return QVariant();
    }

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid()) return QVariant();

        int link = (int)index.internalId();

        if (index.column() == 1)
        {
            if (role == Qt::SizeHintRole)
                return ColorListEditor::widgetSize();

            if ((role != Qt::DisplayRole) && ( role != Qt::EditRole))
                return QVariant();

            return QColor(selection_.itemProperty(link, "color").toUInt());
        }

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

        if (role == Qt::ForegroundRole) {
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
        if (index.column() == 0 || selection_.hasChildren(link))
         return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        else
         return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole)
    {
        if (!index.isValid()) return false;

        if (role ==  Qt::EditRole && index.column() == 1){
            int link = (int)index.internalId();

            QColor oldcolor = selection_.itemProperty(link, "color").toUInt();
            QColor newcolor = qVariantValue<QColor>(value);

            if (oldcolor == newcolor) return true;

            selection_.setItemProperty(link, "color", newcolor.rgb());
            emit dataChanged(index, index);
            return true;
        }

        int link = (int)index.internalId();
        if (role == Qt::CheckStateRole)
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
};

ParamSelection::ParamSelection(QWidget* parent)
: QWidget(parent), minimizeSize_(false)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    view = new QTreeView(this);
    view->setRootIsDecorated(false);
    mainLayout->addWidget(view);

    viewModel = new ParamSelectionWidgetModel(this);
    view->setModel(viewModel);
    ParamSelectDelegate * delegate = new ParamSelectDelegate(viewModel->selection(),this);
    view->header()->setStretchLastSection(true);
    view->header()->setMovable(false);
    view->header()->setResizeMode(1, QHeaderView::Interactive);
    view->header()->resizeSection(1, ColorListEditor::widgetSize().width());
    int first_section_size = 150;
    view->header()->setResizeMode(0, QHeaderView::Interactive);
    view->header()->resizeSection(0, first_section_size);
    view->setItemDelegate(delegate);
    QStringList headerLabels;
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

void ParamSelection::minimizeSize(bool enable)
{
    minimizeSize_ = enable;
}

QSize ParamSelection::sizeHint() const
{
    return (minimizeSize_) ? minimumSizeHint() : QWidget::sizeHint();
}

void ParamSelection::initialize(const Selection & base, const Selection & current)
{
    view->setRootIsDecorated(current.hasSubitems());
    viewModel->setSelection(base, current);

    bool selection_empty = (current.totalItemsCount() == 0);
    showAll->setEnabled(!selection_empty);
    hideAll->setEnabled(!selection_empty);
}

void ParamSelection::initialize(const Selection & selection)
{
    Selection unfiltered = selection;
    unfiltered.enableAll(Selection::ROOT, true);
    initialize(unfiltered, selection);
}

const Selection & ParamSelection::selection() const
{
    return viewModel->selection();
}

void ParamSelection::showOrHideAll()
{
    QModelIndex current = view->currentIndex();
    QModelIndex parent = current.isValid() ? current.parent() : QModelIndex();

    if (sender()->objectName() == "show_all_button")
        viewModel->enableAll(parent);
    if (sender()->objectName() == "hide_all_button")
        viewModel->disableAll(parent);
}

void ParamSelection::emitSelectionChanged()
{
    emit selectionChanged(viewModel->selection());
}

void ParamSelection::saveState(QSettings & settings) const
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

void ParamSelection::restoreState(QSettings & settings)
{
    if (!settings.contains("items"))
    {
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

void ParamSelection::slotDoubleClick(const QModelIndex & index)
{
    emit itemDoubleClicked((int)index.internalId());
}

}

}
