#include "paraminfo.h"
#include "stand_event_model.h"

#include <QToolButton>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QApplication>
#include <QDesktopWidget>

namespace vis3 {
namespace stand {

ParamInfoPopup::ParamInfoPopup(QWidget * parent)
    : QFrame(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

    setFrameStyle(QFrame::Box);
    setLineWidth(2);

    // Create and setup widgets

    btnPrevious = new QToolButton(this);
    btnPrevious->setAutoRaise(true);
    btnPrevious->setIconSize(QSize(16, 16));
    btnPrevious->setIcon(QIcon(":/previous.png"));
    btnPrevious->setToolTip(tr("Previous"));
    connect(btnPrevious, SIGNAL(pressed()), this, SLOT(previousParam()));

    lblCounter = new QLabel(this);
    lblCounter->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont font = lblCounter->font();
    font.setWeight(font.weight()+4);
    lblCounter->setFont(font);

    btnNext = new QToolButton(this);
    btnNext->setAutoRaise(true);
    btnNext->setIconSize(QSize(16, 16));
    btnNext->setIcon(QIcon(":/next.png"));
    btnNext->setToolTip(tr("Next"));
    connect(btnNext, SIGNAL(pressed()), this, SLOT(nextParam()));

    btnClose = new QToolButton(this);
    btnClose->setAutoRaise(true);
    btnClose->setIconSize(QSize(16, 16));
    btnClose->setIcon(QIcon(":/close.png"));
    btnClose->setToolTip(tr("Close"));
    connect(btnClose, SIGNAL(pressed()), this, SLOT(close()));

    lblParamInfo = new QLabel(this);
    lblParamInfo->setContentsMargins(5, 0, 0, 0);

    lblShowList = new QLabel(this);
    lblShowList->setText("<a href=\"#\"><i><small>"+tr("show full list")+"</small></i></a>");
    lblShowList->setAlignment(Qt::AlignCenter);
    lblShowList->setCursor(Qt::PointingHandCursor);
    connect(lblShowList, SIGNAL(linkActivated(const QString &)), this, SLOT(showList(const QString &)));

    // Lay out  widgets
    QHBoxLayout * hLayout = new QHBoxLayout();
    hLayout->addWidget(btnPrevious);
    hLayout->addStretch();
    hLayout->addWidget(lblCounter);
    hLayout->addStretch();
    hLayout->addWidget(btnNext);
    hLayout->addStretch(1);
    hLayout->addWidget(btnClose);

    QVBoxLayout * vLayout = new QVBoxLayout(this);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(lblParamInfo);
    vLayout->addWidget(lblShowList);
}

QSize ParamInfoPopup::sizeHint() const
{
    return layout()->minimumSize();
}

void ParamInfoPopup::show(const Stand_update_event_list & p)
{
    if (p.isEmpty()) return;

    params = p;
    currentParam = 0;

    showParam();

    // Select comfortable window position
    QPoint pos = QCursor::pos();
    QRect screen = QApplication::desktop()->availableGeometry();
    QSize size = sizeHint();
    if (pos.x() + size.width() >= screen.width()) pos.rx() -= size.width();
    if (pos.y() + size.height() >= screen.height()) pos.ry() -= size.height();
    move(pos);

    QFrame::show();
}

void ParamInfoPopup::showParam()
{
    lblCounter->setText(QString("<b>%1/%2</b>").arg(currentParam+1).arg(params.size()));

    QString paramInfo;
    QString formatStr("<b>%1:</b> %2");
    paramInfo  = formatStr.arg(tr("Component")).arg(params[currentParam]->component_name()) + "<br>";
    paramInfo += formatStr.arg(tr("Name")).arg(params[currentParam]->param_name()) + "<br>";
    paramInfo += formatStr.arg(tr("Time")).arg(params[currentParam]->time.toString(true)) + "<br>";
    paramInfo += formatStr.arg(tr("Value")).arg(params[currentParam]->param.value.toString());

    lblParamInfo->setText(paramInfo);

    btnPrevious->setEnabled(currentParam > 0);
    btnNext->setEnabled(currentParam < params.size()-1);
}

void ParamInfoPopup::nextParam()
{
    currentParam++;
    showParam();
}

void ParamInfoPopup::previousParam()
{
    currentParam--;
    showParam();
}

void ParamInfoPopup::showList(const QString &)
{
    ParamInfoWnd * infoWnd = new ParamInfoWnd(params, parentWidget());
    infoWnd->show();

    hide();
}

//---------------------------------------------------------------------------------------

ParamInfoWnd::ParamInfoWnd(const Stand_update_event_list & p, QWidget * parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Parameters"));
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose, true);

    paramList = new QTreeWidget(this);
    paramList->setRootIsDecorated(false);

    QStringList headerLabels;
    headerLabels << tr("Component") <<  tr("Name") << tr("Time") << tr("Value");
    paramList->setHeaderLabels(headerLabels);
    paramList->setSortingEnabled(true);

    for (int i = 0; i < p.size(); i++)
    {
        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(0, p[i]->component_name());
        item->setText(1, p[i]->param_name());
        item->setText(2, p[i]->time.toString());
        item->setText(3, p[i]->param.value.toString());

        paramList->addTopLevelItem(item);
    }

    QLayout * layout = new QGridLayout(this);
    layout->addWidget(paramList);
}

QSize ParamInfoWnd::sizeHint () const
{
    return paramList->size();
}

}

}
