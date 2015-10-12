#include <QApplication>
#include <boost/enable_shared_from_this.hpp>

#include "otf_trace_model.h"
#include "otf_main_window.h"

int main(int ac, char* av[])
{
    using namespace vis4;
    QApplication app(ac, av);
    app.setOrganizationName("Computer Systems Laboratory");
    app.setOrganizationDomain("lvk.cs.msu.su");
    app.setApplicationName("Vis4");

    Trace_model::Ptr model(new OTF_trace_model("hello_world.otf"));
    //Trace_model::Ptr model(new OTF_trace_model("philosophers.otf"));
    //Trace_model::Ptr model(new OTF_trace_model("wrf.otf"));

    OTF_main_window mw;
    mw.initialize(model);
    mw.show();

    app.exec();
    return 0;
}
