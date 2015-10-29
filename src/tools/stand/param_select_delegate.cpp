#include "param_select_delegate.h"

#include <QApplication>
#include <QPainter>
#include <QColorDialog>

namespace vis3 { namespace stand {

using vis3::common::Selection;

ParamSelectDelegate::ParamSelectDelegate(const Selection& selection,QObject *parent)
    : QItemDelegate(parent),selection_(selection)
{
}

QWidget *ParamSelectDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{

    ColorListEditor * editor = new ColorListEditor(option.palette.highlight().color(), parent);
    return editor;
}

void ParamSelectDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QColor value = qVariantValue<QColor>(index.model()->data(index, Qt::EditRole));
    ColorListEditor * ColorEditor = static_cast<ColorListEditor *>(editor);
    ColorEditor->setColor(value);
    QAbstractItemModel * model = const_cast <QAbstractItemModel*>(index.model());
    setModelData(editor, model, index);
}

void ParamSelectDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    ColorListEditor * ColorEditor = static_cast<ColorListEditor *>(editor);
    QColor color = ColorEditor->color();
    model->setData(index, color, Qt::EditRole);
}

void ParamSelectDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


void ParamSelectDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    if (index.column() != 1){
        QItemDelegate().paint(painter, option, index);
        return;
    }
    if (selection_.hasChildren(index.internalId())){
        if (option.state & QStyle::State_Selected ){
            painter->fillRect(option.rect, option.palette.highlight());
        }
        else {
            painter->fillRect(option.rect, option.palette.base());
        }
    }

    else {
        QColor color = qVariantValue<QColor> (index.model()->data(index, Qt::DisplayRole));
        if (option.state & QStyle::State_Selected)
           ColorListEditor::paintWidget(painter, option.rect, color, option.palette.highlight().color());
        else
           ColorListEditor::paintWidget(painter, option.rect, color, option.palette.base().color());
    }
}

//-----------------------------------------------------------------------------------------------------------

ColorListEditor::ColorListEditor(QColor bgColor, QWidget * widget):
                                 QWidget(widget), bgColor_(bgColor)
{
}

void ColorListEditor::paintWidget (QPainter * painter, QRect rect, QColor color, QColor bgColor)
{
    painter->fillRect(rect, bgColor);
    painter->setPen(color);
    painter->setBrush(color);
    int rect_middle = (rect.top() + rect.bottom()) / 2;
    QRect colorRect(0, 0, 13, 10);
    colorRect.moveTo(rect.left()+2, rect_middle-5);
    QFont font;
    int font_height = QFontMetrics(font).height();
    painter->drawRect(colorRect);
    painter->setPen(Qt::black);
    painter->setFont(font);
    painter->drawText(QPoint(rect.left() + 20, rect_middle + font_height/3), color.name());
}

QSize ColorListEditor::widgetSize()
{
    QSize result;
    QFont font;
    int textSize = QFontMetrics(font).width("#AAAAAA");
    result.setWidth(textSize + 20);
    result.setHeight(24);
    return result;
}

void ColorListEditor::setColor(QColor color)
{
    color_ = color;

    QColor tmpColor = QColorDialog::getColor(color);
    if (tmpColor.isValid())
        color_ = tmpColor;
}

void ColorListEditor::paintEvent(QPaintEvent * event)
{
    QPainter p(this);
    paintWidget(&p, this->rect(), color_, bgColor_);
}

}

}
