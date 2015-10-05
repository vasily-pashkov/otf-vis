#include "timeunit_control.h"
#include "time_vis3.h"

#include <QStyleOption>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

namespace vis4 { namespace common {

TimeUnitControl::TimeUnitControl(QWidget * parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_Hover, true);

    // Create popup menu with unit and format settings
    menu = new QMenu(this);
    connect(menu, SIGNAL( triggered(QAction *) ),
        this, SLOT( menuActionTriggered(QAction *) ));

    QAction * action;
    QActionGroup * actionGroup;

    QMenu * timeUnitMenu = menu->addMenu(tr("Time unit"));
    timeUnitMenu->setObjectName("timeunit_menu");

    actionGroup = new QActionGroup(timeUnitMenu);
    for(int i = 0; i < Time::units().size(); ++i)
    {
        action = new QAction(Time::unit_name(i), actionGroup);
        action->setCheckable(true);
    }
    timeUnitMenu->addActions(actionGroup->actions());

    QMenu * timeFormatMenu = menu->addMenu(tr("Time format"));
    timeFormatMenu->setObjectName("timeformat_menu");

    actionGroup = new QActionGroup(timeFormatMenu);
    action = actionGroup->addAction(tr("plain"));
    action->setCheckable(true);
    action = actionGroup->addAction(tr("separated"));
    action->setCheckable(true);
    timeFormatMenu->addActions(actionGroup->actions());
}

// !!! All QStyle stuff was copy-pasted from qcombobox.cpp

QSize TimeUnitControl::sizeHint() const
{
    QSize size;

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QFontMetrics fm = fontMetrics();
    size.setHeight(qMax(fm.lineSpacing(), 14) + 2);
    size.setWidth(fm.width(tr("h:m:s.us")));

    size = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, size, this);
    return size;
}

void TimeUnitControl::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);
    style()->drawControl(QStyle::CE_ComboBoxLabel, &opt, &painter, this);

    // Set tooltip
    QString toolTip = Time::unit_name(Time::unit()) + " (" +
        (Time::format() == Time::Plain ? tr("plain") :  tr("separated")) + ")";
    setToolTip(toolTip);
}

void TimeUnitControl::mousePressEvent(QMouseEvent *e)
{
    QMenu * timeUnitMenu = qFindChild<QMenu*>(menu, ("timeunit_menu"));
    if (Time::format() == Time::Plain)
        timeUnitMenu->setTitle(tr("Time unit"));
    else
        timeUnitMenu->setTitle(tr("Precision"));
    timeUnitMenu->actions()[Time::unit()]->setChecked(true);

    QMenu * timeFormatMenu = qFindChild<QMenu*>(menu, ("timeformat_menu"));
    timeFormatMenu->actions()[Time::format()]->setChecked(true);

    // Show popup menu with time unit and format choice
    QPoint p = parentWidget()->mapToGlobal(pos());
    p.setY(p.y()-menu->sizeHint().height());
    menu->popup(p);
    update();
}

void TimeUnitControl::initStyleOption(QStyleOptionComboBox *option) const
{
    option->initFrom(this);
    option->editable = false;
    option->frame = false;
    if (Time::format() == Time::Advanced){
        QString text;
        switch (Time::unit()){
            case Time::hour:
                text = tr("h");
                break;
            case Time::min:
                text = tr("h:m");
                break;
            case Time::sec:
                text = tr("h:m:s");
                break;
            case Time::ms:
                text = tr("h:m:s.ms");
                break;
            case Time::us:
                text = tr("h:m:s.us");
                break;

        }
        option->currentText = text;
    }
    else {
        option->currentText = Time::unit_name(Time::unit());
    }

    if (menu->isVisible())
        option->state |= QStyle::State_On;
}

void TimeUnitControl::menuActionTriggered(QAction * action)
{
    QMenu * menu = qobject_cast<QMenu*>(action->parentWidget());
    if (menu->objectName() == "timeunit_menu")
    {
        int unit = menu->actions().indexOf(action);
        Time::setUnit(unit);
    }
    else
    {
        int format = menu->actions().indexOf(action);
        if (format == 1)
            Time::setFormat(Time::Advanced);
        else
            Time::setFormat(Time::Plain);
    }

    emit timeSettingsChanged();
}

}} // namespaces
