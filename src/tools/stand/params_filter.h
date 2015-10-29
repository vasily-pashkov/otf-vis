#ifndef PARAMS_FILTER_H
#define PARAMS_FILTER_H

#include "stand_trace_model.h"
#include "param_select.h"

#include <QDialog>

class QPushButton;

namespace vis3 { namespace stand {

class ParamsFilterWidget : public QDialog {

    Q_OBJECT

public: /* methods */

    ParamsFilterWidget(QWidget * parent = 0);
    void setModel(Stand_trace_model::Ptr model);

private: /* events */

    void closeEvent(QCloseEvent * event);

signals:

    void modelChanged(Stand_trace_model::Ptr);

    /** Signal is used to unpush filter button when close its window. */
    void windowClosed();

private slots:

    void slotSelectionChanged(const vis3::common::Selection & selection);

private: /* members */

    ParamSelection * paramsList;
    Stand_trace_model::Ptr model_;

};

}

}
#endif
