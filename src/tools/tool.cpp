#include "tool.h"
#include "canvas.h"
#include "trace_model.h"
#include "main_window.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>

namespace vis4 {

Tool::Tool(QWidget* parent, Canvas* canvas)
    : QWidget(parent), canvas_(canvas), action_(0)
{
    mainWnd_ = dynamic_cast<MainWindow*>(canvas_->parent());
}

boost::shared_ptr<Trace_model>& Tool::model() const
{
    return canvas_->model();
}

QAction* Tool::action()
{
    if (!action_)
        action_ = createAction();
    return action_;
}

void Tool::prepare_settings(QSettings & settings) const
{
    assert(mainWnd_ != 0);
    mainWnd_->prepare_settings(settings, model());
}

bool Tool::event(QEvent * event)
{
    if (event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent * keyEvent = static_cast<QKeyEvent*>(event);
        switch(keyEvent->key()) {
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Up:
            case Qt::Key_Down:
                event->accept();
        }
    }

    return QWidget::event(event);
}

}
