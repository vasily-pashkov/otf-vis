#include "params_filter.h"
#include "stand_event_model.h"
#include "param_select.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QCloseEvent>

namespace vis3 { namespace stand {

using vis3::common::Selection;

ParamsFilterWidget::ParamsFilterWidget(QWidget * parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Parameters filter"));
    setWindowIcon(QIcon(":/filter.png"));

    paramsList = new ParamSelection(this);
    connect(paramsList, SIGNAL( selectionChanged (const vis3::common::Selection &) ),
        this, SLOT( slotSelectionChanged(const vis3::common::Selection &) ));
    QVBoxLayout * vLayout = new QVBoxLayout();
    vLayout->setMargin(0);
    vLayout->addWidget(paramsList);
    setLayout(vLayout);
}

void ParamsFilterWidget::setModel(Stand_trace_model::Ptr model)
{
    if (model.get() == model_.get()) return;
    paramsList->initialize(model->params());
    model_ = model;
    update();
}

void ParamsFilterWidget::slotSelectionChanged(const Selection & selection)
{
    Stand_trace_model::Ptr new_model;
    new_model = model_->filter_params(selection);
    setModel(new_model);

    emit modelChanged(model_);
}

void ParamsFilterWidget::closeEvent(QCloseEvent * event)
{
    event->accept();
    emit windowClosed();
}

}

}
