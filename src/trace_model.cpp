#include "trace_model.h"

namespace vis4 {

int delta(const Trace_model& a, const Trace_model& b)
{
    int result = 0;

    if (a.parent_component() != b.parent_component())
        result |= Trace_model_delta::component_position;

    if (a.components() != b.components())
        result |= Trace_model_delta::components;

    if (a.events() != b.events())
        result |= Trace_model_delta::event_types;

    if (a.groupsEnabled() != b.groupsEnabled())
        result |= Trace_model_delta::event_types;

    if (a.states() != b.states())
        result |= Trace_model_delta::state_types;

    if (a.available_states() != b.available_states())
        result |= Trace_model_delta::state_types;

    if (!a.min_time().sameType(b.min_time())
        || a.min_time() != b.min_time()
        || !a.max_time().sameType(b.max_time())
        || a.max_time() != b.max_time())
        result |= Trace_model_delta::time_range;

    return result;
}

}
