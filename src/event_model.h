#ifndef EVENT_MODEL_HPP_VP_2006_03_21
#define EVENT_MODEL_HPP_VP_2006_03_21

#include "time_vis3.h"
#include <QString>

class QWidget;

namespace vis4 {


/** Данный класс описывает событие для целей визуализации.
    Указатели на Event_model возвращаются методом Trace_model::next_event.
*/
class Event_model
{
public:
    /** Врямя возникновения события. */
    common::Time time;

    /** Тип события (строка). Используется только для показа типа
        события в пользовательком интерфейсе.
    */
    QString kind;

    /** Буква, используемая для показа события. Визуализатором всегда
       показывается как заглавная. */
    char letter;

    /** Дополнительная буква, используемая для показа события. Визуализатором всегда
    показывается как строчная. Рисуется справа от основной буквы меньшим шрифтом.
    Если равна нулю, не показывается. */
    char subletter;

    /** Позиция буквы относительно "засечки" события. */
    enum letter_position_t { left_top = 1, right_top, right_bottom, left_bottom}
    letter_position;

    /** Номер компонента, в котором произошло событие. */
    int component;

    /** Event priority, used to decide which letter to draw
        if letters from different event overlap.
    */
    unsigned priority;

    /** Возвращает краткое описание события, пригодное например для
        списка событий. Возвращаемое значение не должно включать время. */
    virtual QString shortDescription() const { return kind; }

    /** Метод может быть перегружен унаследованными классами, чтобы предоставить
    специфичный метод показа детальной информации о событии.

    Метод должен создать интерфейсные элементы для показа подробной информации о событии.
    Эти элементы должны быть внутри объекта parent. Метод может вызываться многократно
    с одним и тем же parent для показа разных событий и должен определять ситуацию
    когда графичиские элементы для данного parent уже созданы. Максимально желательно
    не удалять ранее созданные элементы а только менять текст и другие их свойства.

    Гарантируется, что никакой другой код не добавляет ничего к parent.
    Гарантируется, что событие для которого вызывается метод, не будет удалено ранее, чем
    графические элементы, им созданные.
    */
    virtual void createDetailsWidget(QWidget* parent) {}

    /** Возвращает true если createDetailsWidget переопределен. */
    virtual bool customDetailsWidget() const { return false; }

    virtual ~Event_model() {}

    virtual bool operator==(const Event_model & other) const
    {
        // Very stupid implementation for vis_xml and vis_fake

        if (time != other.time) return false;
        if (kind != other.kind) return false;
        if (letter != other.letter) return false;
        if (subletter != other.subletter) return false;
        if (component != other.component) return false;

        return true;
    }

protected:
    /** Создает объект, пригодный для показа текстовой детальной информации о событии.
    Унаследованные классы могут использовать этот метод в своей реализации createDetailsWidget,
    если и них есть детальная информация как строка, и при этом не требуется тонкий контроль
    за представлением этой информации.
    */
    QWidget* createTextDetailsWidget(const QString& text, const QWidget* parent);
};

}
#endif
