QT += xml
LIBS = -L/usr/lib \
    -lm \
    -lotf \
    -lxml2 \
    -Wl,-rpath=/usr/lib
INCLUDEPATH += /usr/include/libxml2
SOURCES += vis4.cpp \
    trace_model.cpp \
    selection.cpp \
    time_vis3.cpp \
    otf_trace_model.cpp \
    event_list.cpp \
    canvas_item.cpp \
    main_window.cpp \
    canvas.cpp \
    trace_painter.cpp \
    timeline.cpp \
    timeunit_control.cpp \
    tools/tool.cpp \
    otf_main_window.cpp \
    tools/find_tabs.cpp \
    checker.cpp \
    tools/timeedit.cpp \
    tools/selection_widget.cpp \
    grx.cpp
HEADERS += trace_model.h \
    selection.h \
    time_vis3.h \
    otf_trace_model.h \
    state_model.h \
    group_model.h \
    event_model.h \
    event_list.h \
    canvas_item.h \
    main_window.h \
    canvas.h \
    trace_painter.h \
    timeline.h \
    timeunit_control.h \
    tools/tool.h \
    otf_main_window.h \
    tools/browser.h \
    tools/measure.h \
    tools/goto.h \
    tools/find.h \
    tools/filter.h \
    tools/find_tabs.h \
    checker.h \
    tools/timeedit.h \
    tools/selection_widget.h \
    grx.h
RESOURCES += vis3.qrc
OTHER_FILES += 
