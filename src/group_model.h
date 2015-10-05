#ifndef GROUP_MODEL_HPP_VP_2006_03_30
#define GROUP_MODEL_HPP_VP_2006_03_30

#include "time_vis3.h"
#include <vector>

namespace vis4 {

/* Group is a collection of timeline points that must be
   specially decorated as a group.  */
class Group_model
{
public:
    static const int arrow = 1;

    struct point { int component; common::Time time; };

    int type;
    std::vector<point> points;
};

}
#endif
