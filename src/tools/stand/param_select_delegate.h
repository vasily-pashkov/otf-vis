#ifndef PARAM_SELECT_DELEGATE_HPP
#define PARAM_SELECT_DELEGATE_HPP

#include <selection.h>

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QLabel>
#include <QMap>
#include <QColor>

namespace vis3 { namespace stand {

class ColorListEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor USER true);
    QColor color_;
    QColor bgColor_;

public:

    ColorListEditor(QColor bgColor, QWidget *widget = 0);

    static void paintWidget (QPainter * painter, QRect rect, QColor color,QColor bgColor = Qt::white);

    static QSize widgetSize();

    QColor color() const { return color_; }
    void setColor(QColor color);

protected:
    void paintEvent(QPaintEvent * event);

};

class ParamSelectDelegate : public QItemDelegate
{
     Q_OBJECT
private: /* members */

    const vis3::common::Selection & selection_;

public: /* methods*/

    ParamSelectDelegate(const vis3::common::Selection& selection, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

}

}
#endif
