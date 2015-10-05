#ifndef OTF_TRACE_MODEL_H
#define OTF_TRACE_MODEL_H

#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QMap>
#include <QDebug>
#include <sstream>
#include <set>
#include <queue>
#include <algorithm>

#include <stdio.h>
#include <assert.h>

#include <boost/enable_shared_from_this.hpp>

#include "trace_model.h"
#include "state_model.h"
#include "event_model.h"
#include "canvas_item.h"
#include "group_model.h"
#include "event_list.h"
#include "grx.h"

#include "otf.h"

namespace vis4 {

using namespace common;
class OTF_trace_model;

typedef struct {
    uint64_t count;
    int parent_component;
    Selection* components;
    uint64_t countSend;
    uint64_t countRecv;
} HandlerArgument;

class OTF_trace_model : public Trace_model,
                        public boost::enable_shared_from_this<OTF_trace_model>
{
public: /* members */
    typedef boost::shared_ptr<OTF_trace_model> Ptr;

public: /* methods */
    OTF_trace_model(const QString& filename);
    ~OTF_trace_model();

    int parent_component() const;
    const QList<int> & visible_components() const;
    int lifeline(int component) const;
    int component_type(int component) const;
    QString component_name(int component, bool full = false) const;
    bool has_children(int component) const;

    Time min_time() const;
    Time max_time() const;
    Time min_resolution() const;

    void rewind();

    std::auto_ptr<State_model> next_state();
    std::auto_ptr<Group_model> next_group();
    std::auto_ptr<Event_model> next_event_unsorted();
    std::auto_ptr<Event_model> next_event();

    Trace_model::Ptr root();
    Trace_model::Ptr set_parent_component(int component);
    Trace_model::Ptr set_range(const Time& min, const Time& max);

    const Selection & components() const;
    Trace_model::Ptr filter_components(const Selection & filter);

    const Selection & events() const;
    const Selection & states() const;
    const Selection& available_states() const;

    Trace_model::Ptr filter_states(const Selection & filter);
    Trace_model::Ptr install_checker(Checker * checker);
    Trace_model::Ptr filter_events(const Selection & filter);

    QString save() const;
    bool groupsEnabled() const;
    Trace_model::Ptr setGroupsEnabled(bool enabled);
    void restore(const QString& s);

private:    /* members */
    OTF_FileManager* manager;
    OTF_Reader* reader;

    int parent_component_;
    Selection components_;
    Selection events_;
    bool groups_enabled_;
    Selection states_;
    Selection available_states_;

    Time min_time_;
    Time max_time_;

    OTF_HandlerArray* handlers;
    HandlerArgument ha;
    uint64_t ret;

//?
    ComponentTree::Link hierarchy_pos;
    QMap<ComponentTree::Link, int> components_map_;
    boost::shared_ptr<ComponentTree> hierarchy_backend;

private:    /* methods */
    Time getTime(int t) const;
    void initialize();
    void adjust_components();
    void initialize_component_list();

//?
    void findNextItem(const QString& elementName);
    QDomElement currentElement;
    std::vector<QDomElement> subcomponents;

    QList<int> visible_components_;
    QMap<int, int> lifeline_map_;

    // The current item we iterate over -- could be state,
    // or event, or group event.
    QDomElement currentItem;
    int currentSubcomponent;

    // All events sorted by the time.
    QVector<Event_model*> allEvents;
    int currentEvent;

    // Need to hold a reference to root so that it's not
    // deleted.
    QDomElement root_;
};



//*******************************************************************************************************************************************************8

// Обработчики определений компонентов
static int handleDefProcessGroup (void *userData, uint32_t stream, uint32_t procGroup, const char *name, uint32_t numberOfProcs, const uint32_t *procs)
{
    qDebug() << "stream = " << stream << " procGroup: " << procGroup << " name: " << name << " number: " << numberOfProcs;
    QString str = "";
    for (int i=0; i<numberOfProcs; i++) str += QString::number(procs[i]) + " ";
    qDebug() << "process list: " << str;
    return OTF_RETURN_OK;
}

static int handleDefProcess (void *userData, uint32_t stream, uint32_t process, const char *name, uint32_t parent)
{
    int current_link = ((HandlerArgument*)userData)->components->addItem( QString(name), (int) parent);
    qDebug() << "stream = " << stream << " process: " << process << " name: " << name << " parent: " << parent;
    return OTF_RETURN_OK;
}

//=========================

// Обработчики определений состояний
static int handleDefFunctionGroup (void *userData, uint32_t stream, uint32_t funcGroup, const char *name)
{
    qDebug() << "stream = " << stream << " funcGroup: " << funcGroup << " name: " << name;
    return OTF_RETURN_OK;
}

static int handleDefFunction (void *userData, uint32_t stream, uint32_t func, const char *name, uint32_t funcGroup, uint32_t source)
{
    qDebug() << "stream = " << stream << " func: " << func << " name: " << name << " funcGroup: " << funcGroup << " source: " << source;
    return OTF_RETURN_OK;
}

//=========================

// Обработчики определений событий
static int handleDefMarker(void *userData, uint32_t stream, uint32_t token, const char *name, uint32_t type)
{
    qDebug() << "DEFMARKER: stream = " << stream << " token: " << token << " name: " << name << " type: " << type;
    return OTF_RETURN_OK;
}


//===========================================================================
//===========================================================================
//===========================================================================

// Обработчики событий и состояний
static int handleEnter (void *userData, uint64_t time, uint32_t function, uint32_t process, uint32_t source, OTF_KeyValueList *list)
{
     //fprintf( stdout, "      Enter time: %u function name: '%u'\n", process, function );

     qDebug() << "ENTER: " << function;
     return OTF_RETURN_OK;
}

 static int handleLeave (void *userData, uint64_t time, uint32_t function, uint32_t process, uint32_t source, OTF_KeyValueList *list)
{
     //fprintf( stdout, "      Leave time: %u function name: '%u'\n", process, function );
     qDebug() << "LEAVE: " << function;
     return OTF_RETURN_OK;
}

 static int handleMarker(void *userData, uint64_t time, uint32_t process, uint32_t token, const char *text, OTF_KeyValueList *list)
{
     qDebug() << "MARKER: " << text;
     return OTF_RETURN_OK;
}


// *************************** MESSAGES **************************************
// ***************************************************************************
 static int handleSendMsg(void *userData, uint64_t time, uint32_t sender, uint32_t receiver, uint32_t group, uint32_t type, uint32_t length, uint32_t source, OTF_KeyValueList *list)
{
     //qDebug() << "SendMsg: ";
     ((HandlerArgument*)userData)->countSend++;
     return OTF_RETURN_OK;
}

 static int handleRecvMsg(void *userData, uint64_t time, uint32_t recvProc, uint32_t sendProc, uint32_t group, uint32_t type, uint32_t length, uint32_t source, OTF_KeyValueList *list)
{
     //qDebug() << "RecvMsg: ";
     ((HandlerArgument*)userData)->countRecv++;
     return OTF_RETURN_OK;
}

}   // End of Namespace

#endif //
