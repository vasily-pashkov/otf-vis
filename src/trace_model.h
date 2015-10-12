
#ifndef TRACE_MODEL_HPP_VP_2006_03_20
#define TRACE_MODEL_HPP_VP_2006_03_20

#include "time_vis3.h"
#include "selection.h"

#include <boost/shared_ptr.hpp>

#include <memory>
#include <vector>

class Trace;

namespace vis4 {

class Event_model;
class State_model;
class Group_model;
class Checker;

/** Абстракное представление трассы для визуализатора.

    Этот абстрактный класс описывает то, что должно быть показано на зкране (линии жизни,
    события, состояние), и предоставляет интерфейсы для навигации по командам пользователя.
    Метод определения показываемых данных определяется унаследованными классами, данный
    класс не предполагает ничего конкретного.

    Абсолютно весь доступ к трассе внутри Vis4 делается через этот интерфейс, включая все пользовальские
    интструменты.

    Все классы-потомки должны быть "легкими", объем используемой памяти на каждый экземпляров
    должен быть минимальным, и копирование/навигация должны быть быстрыми операциями. Это означает,
    что при использовании сложных структур данных для представления трассы экземпляры классов-потомков
    должы ссылаться на информацию о трассе, содержа внутри себя только выбранные времена/фильтры.

    Предоставляемые интерфейсы не дают возможности изменить объект. Все методы навигации создают новый
    экземпляр. Это, в сочетании с требованием "легкости" объектов, позволяет сохранять объекты, чтобы
    потом к ним можно быть быстро вернуться. Это полезно, например, для операции "back".

    Навигация выполняется по трем осям.

    Первая ось -- положение в структуре компонент. Предполагается, что есть конечное число позиций
    в структуре компонетнов. Для каждой позиции есть набор потенциально видимых линий жизни, событий
    и состояний. Набор таких позиций определяется конкретным экземпляром конкретной реализации
    интерфейса Trace_model.

    Между позициями возможны переходы двух типов
    - Переход "наверх"
    - "Раскрытие" одной из линий жизни в входом в него
    Для каждого положения возможность выполнения этих переходов определяет конкретная реализация
    интерфейса Trace_model.

    Вторая ось навигации -- фильтры линий жизни, событий и состояний. Если положение в структуре
    компонент определяет потенциальный набор этих объектов, то дополнительная фильтрация определяет
    реально видимый набор.

    Третья ось навигации -- это минимальное и максимальное показываемое время. Речь идет именно о
    показываемом времени -- левой и правой границе временной диаграмме. Объекты, возвращаемые Trace_model,
    могут выходить за этот диапазон, что особенно актуально для "протяженных" объектыов
    (состояния и групповые события). Можно считать, что все объекты рисуются на бесконечной плоскости,
    после чего плоскость обрезается заданному диапазону времени.

    Навигация по всем трем осям выполняется с помощью вызовом методов класса Trace_model, которые
    создают новый объект класс Trace_model с новым представлением трассы.
*/
class Trace_model
{
public:

    typedef boost::shared_ptr<Trace_model> Ptr;

    enum ComponentType { RCHM = 1, CHM, CHANNEL, INTERFACE, EXTERNAL_OBJECTS };

    /** Возвращает объект Trace_model соответствующий "начальному" положению
    в структуре компонетов, максимальному возможному диапазону времен и
    не имеющий никаких фильтров. */
    virtual Trace_model::Ptr root() = 0;

/** @defgroup hierarchy Methods for managing components hierarchy */
/// @{

    /** Returns the link of the parent component.
        Link may be used to refer to parent component in components selection. */
    virtual int parent_component() const = 0;

    /** Returns Trace_model with given component as current parent. */
    virtual Trace_model::Ptr set_parent_component(int component) = 0;

    /** Возвращает true если компонет component имеет "детей" и в него можно войдти.
        @sa set_parent_component. */
    virtual bool has_children(int component) const = 0;

/// @}

/** @defgroup time Methods for managing model time. */
/// @{

    /** Возвращает минимальное время, которое должно показываться на временной диаграмме. */
    virtual common::Time min_time() const = 0;

    /** Возвращает максимальное время, которое должно показываться на временной диаграмме. */
    virtual common::Time max_time() const = 0;

    /** Возвращает минимальный интервал времени, который может отобразить ВД. */
    virtual common::Time min_resolution() const = 0;

    /** Переводит внутренние указатели событий, состояний и групповых событий
       на минимальное время. */
    virtual void rewind() = 0;

    /** Возврашает новый объект Trace_model с указанным диапазоном времен. */
    virtual Trace_model::Ptr set_range(const common::Time& min, const common::Time& max) = 0;

/// @}

/** @defgroup data Methods for obtaining trace data. */
/// @{

    /** Возвращает следующее событие с трассе и увеличивает внутренний указатель
        событий. Если больше собитый нет, возвращает нулевой указатель. */
    virtual std::auto_ptr<Event_model> next_event() = 0;

    /** Возвращает следующее состояние в трассе и увеличивает внутренний указатель
    событий. Если больше собитый нет, возвращает нулевой указатель. */
    virtual std::auto_ptr<State_model> next_state() = 0;

    /** Возвращает следующее состояние в трассе и увеличивает внутренний указатель
    событий. Если больше собитый нет, возвращает нулевой указатель. */
    virtual std::auto_ptr<Group_model> next_group() = 0;

/// @}

/** @defgroup filters Methods for managing filters. */
/// @{

    /** Returns new object with given selection of components. */
    virtual Trace_model::Ptr filter_components(
        const common::Selection & filter) = 0;

    /** Returns current component selection. */
    virtual const common::Selection & components() const = 0;

    /** Returns the list with links of visible components.
        Visibility means that component is not filtered and
        it's a child of current parent. */
    virtual const QList<int> & visible_components() const = 0;

    /** Returns a lifeline number for given component.
        Returns -1 if there is no corresponding lifeline. */
    virtual int lifeline(int component) const = 0;

    /** Returns a name for given component.
        If "fullname" is true returns full component name,
        which includes all its parents. */
    virtual QString component_name(int component, bool fullname = false) const = 0;
    virtual int component_type(int component) const = 0;

    /** Returns new object with given selection of event. */
    virtual Trace_model::Ptr filter_events(
        const common::Selection & filter) = 0;

    virtual bool groupsEnabled() const = 0;
    virtual Trace_model::Ptr setGroupsEnabled(bool enabled) = 0;

    /** Returns current event selection. */
    virtual const common::Selection & events() const = 0;

    /** Returns new object with given selection of states. */
    virtual Trace_model::Ptr filter_states(
        const common::Selection & filter) = 0;

    /** Returns current state selection. */
    virtual const common::Selection & states() const = 0;

    /** Returns selection with only available states enabled.
        State is available when though one of corresponding
        components are enabled. */
    virtual const common::Selection & available_states() const = 0;

    /** Return new model with given checker installed into. */
    virtual Trace_model::Ptr install_checker(Checker * checker) = 0;

/// @}

    /** Сохраняет текущее состояние в строку и возвращает его. Структура строки не
       определена. */
    virtual QString save() const = 0;

    /** По строке, ранее возвращенной функцией save, пытается восстановить
       состояние. Функция не обязательно что-то делает, если реальная данные трассы
       слишком отличаются от данных трассы в момент вызова save, функция может ничего
       не делать.

       Основное предназначение -- для восстановления состояния графического интерфейса
       при последующих запусках. */
    virtual void restore(const QString& s) = 0;

    virtual ~Trace_model() {}

};


struct Trace_model_delta
{
    enum {
        component_position = 1,
        components = component_position << 1,
        event_types = components << 1,
        state_types = event_types << 1,
        time_range = state_types << 1
    };
};

/* This is experimental trace comparison interface.
   It is not yet clear if we want to be able to compare
   "inner" state of trace models. We also don't
   compare time unit. This interface may change any
   second. */
int delta(const Trace_model& a, const Trace_model& b);

}
#endif
