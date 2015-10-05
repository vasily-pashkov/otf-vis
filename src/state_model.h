#ifndef STATE_MODEL_HPP_VP_2006_03_21
#define STATE_MODEL_HPP_VP_2006_03_21

#include "time_vis3.h"

#include <QString>
#include <QColor>

namespace vis4 {

/** Описание состояния для целей визуализации. */
class State_model
{
public:
    /** Начальное время состояния. */
    common::Time begin;

    /** Конечное время состояния. */
    common::Time end;

    /** State id. Link to state in Trace_model::states(). */
    int type;

    /** Номер компонента, к которому относится состояние. */
    unsigned component;

    /** The color to be used when drawing it.  */
    QColor color;

    virtual ~State_model() {}
};

}
#endif
