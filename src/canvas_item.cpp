
#include "canvas_item.h"

#include <QWidget>

namespace vis4 {

CanvasItem::CanvasItem()
{}


void CanvasItem::draw(QPainter& painter)
{
    currentRect = xdraw(painter);
}

void CanvasItem::new_geomerty(const QRect& r)
{
    static_cast<QWidget*>(parent())->update(r | currentRect);
}

}
