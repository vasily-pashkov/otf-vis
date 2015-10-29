#ifndef PARAM_SELECT_HPP
#define PARAM_SELECT_HPP

#include "trace_model.h"

#include <QWidget>
#include <QVector>
#include <QSettings>
#include <QModelIndex>

class QTreeView;
class QPushButton;

namespace vis3 { namespace stand {

class ParamSelectionWidgetModel;

/** Standart widget for managing a selection of items.

    Widget must be initialized by initialize() method,
    which uses two selections: base and current.

    Widget shows all items from selection.
    Items that disabled both in base and current objects
    is shown disabled. Other items is shown in accordance
    with it states in current selection.

    Widget emits signal selectionChanged() when user change items state.
*/
class ParamSelection : public QWidget
{
    Q_OBJECT
public:
    /** Standart Qt constructor. Widget must be initialized by
       initialize() method. */
    ParamSelection(QWidget* parent);

    /** Initialize widget using base and current selection */
    void initialize(const vis3::common::Selection & base, const vis3::common::Selection & current);

    /** Initialize widget using given selection */
    void initialize(const vis3::common::Selection & selection);

    /** Returns current selection. */
    const vis3::common::Selection & selection() const;

    /** Minimizes widget size. If enabled, sizeHint will
       return a value of minimumSizeHint(). Used for
       minimization width of the side bar. */
    void minimizeSize(bool enable);

    QSize sizeHint() const;

    void saveState(QSettings & settings) const;

    void restoreState(QSettings & settings);

signals:

    /** Signal emits when user change selection. */
    void selectionChanged(const vis3::common::Selection &);
    void itemDoubleClicked(int itemLink);

private slots:

    void showOrHideAll();

    void emitSelectionChanged();

    void slotDoubleClick(const QModelIndex &);

private:

    QTreeView * view;
    ParamSelectionWidgetModel * viewModel;

    QPushButton * showAll;
    QPushButton* hideAll;

    bool minimizeSize_;

};

}

}
#endif
