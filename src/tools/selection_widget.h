#ifndef SELECTION_WIDGET_HPP_VP_2006_04_03
#define SELECTION_WIDGET_HPP_VP_2006_04_03

#include "selection.h"

#include <QWidget>
#include <QVector>
#include <QSettings>
#include <QModelIndex>

#include <boost/shared_ptr.hpp>

class QTreeView;
class QPushButton;

namespace vis4 {
    namespace common {

class SelectionWidgetModel;

/** Standart widget for managing a selection of items.

    Widget must be initialized by initialize() method,
    which uses two selections: base and current.

    Widget shows all items from selection.
    Items that disabled both in base and current objects
    is shown disabled. Other items is shown in accordance
    with it states in current selection.

    Widget emits signal selectionChanged() when user change items state.
*/
class SelectionWidget : public QWidget
{
    Q_OBJECT
public:

    enum { COMPONENTS_SELECTOR = 0x1 };

    /** Standart Qt constructor. Widget must be initialized by
       initialize() method. */
    SelectionWidget(QWidget* parent, int flags = 0);

    /** Initialize widget using base and current selection */
    void initialize(const Selection & base, const Selection & current);

    /** Initialize widget using given selection */
    void initialize(const Selection & selection);

    /** Returns current selection. */
    const Selection & selection() const;

    /** Minimizes widget size. If enabled, sizeHint will
       return a value of minimumSizeHint(). Used for
       minimization width of the side bar. */
    void minimizeSize(bool enable);

    QSize sizeHint() const;

    void saveState(QSettings & settings) const;

    void restoreState(QSettings & settings);

signals:

    /** Signal emits when user change selection. */
    void selectionChanged(const vis4::common::Selection &);

    void itemDoubleClicked(int itemLink);

private slots:

    void showOrHideAll();

    void emitSelectionChanged();

    void slotDoubleClick(const QModelIndex &);

private:

    QTreeView * view;
    SelectionWidgetModel * viewModel;

    int flags_;

    QPushButton * showAll;
    QPushButton* hideAll;

    bool minimizeSize_;

};

}} // namespaces

#endif
