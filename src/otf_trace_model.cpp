#include "otf_trace_model.h"
#include <QDebug>

#include <OTF_RBuffer.h>

namespace vis4 {

    using namespace common;

    OTF_trace_model:: OTF_trace_model(const QString& filename)
        : min_time_(getTime(0))
    {
        states_.clear();

        ha = (HandlerArgument){ 0 /* count */, parent_component_ /*parent */, &components_ /*components*/, 0, 0};

        manager= OTF_FileManager_open( 100 );
        assert( manager );

        initialize();
        max_time_ = getTime(100);

        handlers = OTF_HandlerArray_open();
        assert( handlers );

        /* processes */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleDefProcessGroup, OTF_DEFPROCESSGROUP_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_DEFPROCESSGROUP_RECORD );

        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleDefProcess, OTF_DEFPROCESS_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_DEFPROCESS_RECORD );

         /* functions */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleDefFunctionGroup, OTF_DEFFUNCTIONGROUP_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_DEFFUNCTIONGROUP_RECORD );

        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleDefFunction, OTF_DEFFUNCTION_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_DEFFUNCTION_RECORD );

         /* markers */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleDefMarker, OTF_DEFMARKER_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_DEFMARKER_RECORD );

        /* for reading enter/leave functions */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleEnter, OTF_ENTER_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_ENTER_RECORD );

        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleLeave, OTF_LEAVE_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_LEAVE_RECORD );

        /* for reading markers */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleMarker, OTF_MARKER_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_MARKER_RECORD );

        /* messages */
        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleSendMsg, OTF_SEND_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_SEND_RECORD );

        OTF_HandlerArray_setHandler( handlers, (OTF_FunctionPointer*) handleRecvMsg, OTF_RECEIVE_RECORD );
        OTF_HandlerArray_setFirstHandlerArg( handlers, &ha, OTF_RECEIVE_RECORD );

        initialize_component_list();

        reader = OTF_Reader_open( filename.toAscii().data(), manager );  // hello_world
        assert( reader );



        // чтение определений и обработка их обработчикоми handlers
        ret = OTF_Reader_readDefinitions( reader, handlers );

        // чтение событий Events
        OTF_Reader_setRecordLimit(reader, 1);
        OTF_Reader_readEvents( reader, handlers);
        OTF_Reader_setRecordLimit(reader, 3);
        OTF_Reader_readMarkers( reader, handlers);

        qDebug() << "read definition records: " << (unsigned long long int)ret;
        qDebug() << "countSend=" << ha.countSend << " countRecv=" << ha.countRecv;
    }

    OTF_trace_model::~OTF_trace_model()
    {
        /*
        OTF_Reader_close( reader );
        OTF_HandlerArray_close( handlers );
        OTF_FileManager_close( manager );
        */
    }


    void OTF_trace_model:: initialize_component_list()
    {
        components_.clear();
        int rootLink = components_.addItem("Stand", Selection::ROOT);
        //components_.setItemProperty(rootLink, "ctlink", ComponentTree::ROOT);
        //components_.setItemProperty(0, "current_parent", rootLink);
        parent_component_ = rootLink;

        //components_map_[ComponentTree::ROOT] = rootLink;
    }


    int OTF_trace_model:: parent_component() const
    {
        return parent_component_;
#if 0
        if (currentElement.tagName() == "component")
            return currentElement.attribute("name");
        else
            return "";
#endif
    }


    const QList<int> & OTF_trace_model:: visible_components() const
    {
        return visible_components_;
    }

    int OTF_trace_model:: lifeline(int component) const
    {
        if (!lifeline_map_.contains(component)) return -1;
        return lifeline_map_[component];
    }

    int OTF_trace_model:: component_type(int component) const
    {
        return has_children(component) ? RCHM : CHM;
    }


    QString OTF_trace_model::component_name(int component, bool full) const
    {
        Q_ASSERT(component >= 0 && component < components_.totalItemsCount());

        if (!full) return components_.item(component);

        QString fullname = components_.item(component).trimmed();
        for(;;)
        {
            QString splitter = "::";
            if (component_type(component) == INTERFACE) splitter = ":";

            component = components_.itemParent(component);
            if (component == Selection::ROOT) break;
            if (component == parent_component()) break;

            fullname = components_.item(component).trimmed() + splitter + fullname;
        }

        return fullname;
    }

    bool OTF_trace_model::has_children(int component) const
    {
        return components_.hasChildren(component);
    }

    Time OTF_trace_model::min_time() const { return min_time_; }
    Time OTF_trace_model::max_time() const { return max_time_; }
    Time OTF_trace_model::min_resolution() const { return getTime(3); }


    void OTF_trace_model::rewind()
    {
        currentItem.clear();
        currentSubcomponent = -1;

        allEvents.clear();
        QMultiMap<Time, Event_model*> events;

        for(;;)
        {
            std::auto_ptr<Event_model> e = next_event_unsorted();

            Event_model* ep = e.release();

            if (!ep)
                break;

            events.insert(ep->time, ep);
        }

        for(QMultiMap<Time, Event_model*>::iterator i = events.begin(),
                e = events.end(); i != e; ++i)
        {
            allEvents.push_back(i.value());
        }
        currentEvent = 0;

        currentItem.clear();
        currentSubcomponent = -1;
    }

    std::auto_ptr<State_model> OTF_trace_model::next_state()
    {
        for(;;)
        {
            findNextItem("states");

            if (currentItem.isNull())
                return std::auto_ptr<State_model>();

            int component = visible_components_[currentSubcomponent];
            QString state_name = currentItem.attribute("name").toAscii().data();

            int sparent = states_.itemLink(0, Selection::ROOT);
            while (states_.itemProperty(sparent, "component").toInt() != component)
            {
                int index = states_.itemIndex(sparent);
                Q_ASSERT(index+1 < states_.itemsCount());
                sparent = states_.itemLink(index+1, Selection::ROOT);
            }

            int state_id = states_.itemLink(state_name, sparent);
            Q_ASSERT(state_id != -1);

            if (!states_.isEnabled(sparent)) continue;
            if (!states_.isEnabled(state_id)) continue;

            std::auto_ptr<State_model> r(new State_model);

            r->begin = getTime(currentItem.attribute("begin").toInt());
            r->end = getTime(currentItem.attribute("end").toInt());
            r->component = component;
            r->type = state_id;
            r->color = Qt::yellow;

            return r;
        }
    }

    std::auto_ptr<Group_model> OTF_trace_model:: next_group()
    {
        if (!groups_enabled_) return std::auto_ptr<Group_model>();

        for(;;)
        {
            findNextItem("groups");

            if (currentItem.isNull())
            {
                return std::auto_ptr<Group_model>();
            }
            else
            {
                std::auto_ptr<Group_model> r(new Group_model);

                r->type = Group_model::arrow;
                r->points.resize(2);
                r->points[0].component = visible_components_[currentSubcomponent];
                r->points[0].time = getTime(
                    currentItem.attribute("time").toInt());
                r->points[1].time = getTime(
                    currentItem.attribute("target_time").toInt());

                QString to_component_s =
                    currentItem.attribute("target_component");

                int to_component = -1;
                for(unsigned i = 0; i < subcomponents.size(); ++i)
                {
                    if (to_component_s == subcomponents[i].attribute("name"))
                    {
                        to_component = i;
                        break;
                    }
                }
                if (to_component != -1)
                {
                    r->points[1].component = visible_components_[to_component];
                    return r;
                }
            }
        }
    }

    std::auto_ptr<Event_model> OTF_trace_model:: next_event_unsorted()
    {
        Time time;
        for(;;)
        {
            findNextItem("events");

            if (currentItem.isNull())
                break;

            time = getTime(currentItem.attribute("time").toInt());

            QString kind_att = currentItem.attribute("kind");
            bool allowed_kind = false;
            if (kind_att.isEmpty())
            {
                allowed_kind = true;
            }
            else
            {
                foreach(int event, events_.items())
                {
                    if (events_.item(event) == kind_att)
                    {
                        allowed_kind = events_.isEnabled(event);
                        break;
                    }
                }
            }

            if (time >= min_time_ && time <= max_time_ && allowed_kind)
                break;
        }

        if (currentItem.isNull())
            return std::auto_ptr<Event_model>();
        else
        {
            std::auto_ptr<Event_model> r(new Event_model);

            r->time = time;
            r->letter = currentItem.attribute("letter")[0].toAscii();
            if (currentItem.hasAttribute("subletter"))
                r->subletter = currentItem.attribute("subletter")[0].toAscii();
            else
                r->subletter = '\0';
            if (currentItem.hasAttribute("position"))
            {
                QString p = currentItem.attribute("position");
                if (p == "left_top")
                    r->letter_position = Event_model::left_top;
                else if (p == "right_top")
                    r->letter_position = Event_model::right_top;
                else if (p == "left_bottom")
                    r->letter_position = Event_model::left_bottom;
                else if (p == "right_bottom")
                    r->letter_position = Event_model::right_bottom;
            }
            else
                r->letter_position = Event_model::right_top;

            if (currentItem.hasAttribute("priority"))
                r->priority = currentItem.attribute("priority").toInt();
            else
                r->priority = 0;

            r->kind = currentItem.attribute("kind").toAscii().data();
            r->component = visible_components_[currentSubcomponent];

            return r;
        }
    }

    std::auto_ptr<Event_model> OTF_trace_model::next_event()
    {
        if (currentEvent >= allEvents.count())
        {
            return std::auto_ptr<Event_model>();
        }
        else
        {
            return std::auto_ptr<Event_model>(allEvents[currentEvent++]);
        }
    }

    Trace_model::Ptr OTF_trace_model::root()
    {
        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->currentElement = root_;
        n->parent_component_ = Selection::ROOT;

        n->min_time_ = getTime(0);
        QString max_time_at = n->currentElement.attribute("max_time");
        if (max_time_at.isEmpty())
        {
            n->max_time_ = max_time_;
        }
        else
        {
            n->max_time_ = getTime(max_time_at.toInt());
        }

        n->events_.enableAll(Selection::ROOT, true);
        n->adjust_components();

        return n;
    }

    Trace_model::Ptr OTF_trace_model::set_parent_component(int component)
    {
        
        if (parent_component_ == component)
            return shared_from_this();

        if (component == Selection::ROOT)
        {
            OTF_trace_model::Ptr n(new OTF_trace_model(*this));
            n->currentElement = root_;
            n->parent_component_ = Selection::ROOT;
            n->adjust_components();
            return n;
        }

        if (component == components_.itemParent(parent_component_))
        {
            OTF_trace_model::Ptr n(new OTF_trace_model(*this));
            n->currentElement = currentElement.parentNode().toElement();
            n->parent_component_ = component;
            n->adjust_components();
            return n;
        }

        if (components_.itemParent(component) == parent_component_)
        {
            OTF_trace_model::Ptr n(new OTF_trace_model(*this));

            QDomNodeList subcomponents = currentElement.childNodes();
            n->currentElement = subcomponents.at(components_.itemIndex(component)).toElement();

            n->parent_component_ = component;
            n->adjust_components();
            return n;
        }

        OTF_trace_model::Ptr n(new OTF_trace_model(*this));

        QList<int> parents; parents << component;
        while (parents.front() != component)
            parents.prepend(components_.itemParent(component));
        parents.pop_front();

        n->currentElement = root_;
        while (!parents.isEmpty())
        {
            int component = parents.first();

            QDomNodeList subcomponents = currentElement.childNodes();
            currentElement = subcomponents.at(components_.itemIndex(component)).toElement();
        }

        n->parent_component_ = component;
        n->adjust_components();
        return n;
    }

    Trace_model::Ptr OTF_trace_model::set_range(const Time& min, const Time& max)
    {
        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->min_time_ = min;
        n->max_time_ = max;
        return n;
    }

    const Selection & OTF_trace_model::components() const
    {
        return components_;
    }

    Trace_model::Ptr OTF_trace_model::filter_components(const Selection & filter)
    {
        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->components_ = filter;
        n->adjust_components();
        return n;
    }

    const Selection & OTF_trace_model::events() const
    {
        return events_;
    }

    const Selection & OTF_trace_model::states() const
    {
        return states_;
    }

    const Selection& OTF_trace_model::available_states() const
    {
        return available_states_;
    }

    Trace_model::Ptr OTF_trace_model::filter_states(const Selection & filter)
    {
        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->states_ = filter;
        //n->adjust_components();
        return n;
    }

    Trace_model::Ptr OTF_trace_model::install_checker(Checker * checker)
    {
        return shared_from_this();
    }

    Trace_model::Ptr OTF_trace_model::filter_events(const Selection & filter)
    {
        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->events_ = filter;
        return n;
    }

    QString OTF_trace_model::save() const
    {
        QString componentPos;
        for(QDomElement c = currentElement;
            !c.isNull() && c.tagName() == "component";
            c = c.parentNode().toElement())
        {
            componentPos = c.attribute("name")+"/"+componentPos;
        }
        componentPos = "/"+componentPos;

        return  componentPos + ":" +
            min_time_.toString() + ":" + max_time_.toString();
    }

    bool OTF_trace_model::groupsEnabled() const
    {
        return groups_enabled_;
    }

    Trace_model::Ptr OTF_trace_model::setGroupsEnabled(bool enabled)
    {
        if (groups_enabled_ == enabled) return shared_from_this();

        OTF_trace_model::Ptr n(new OTF_trace_model(*this));
        n->groups_enabled_ = enabled;
        return n;
    }

    void OTF_trace_model::restore(const QString& s)
    {
        QStringList parts = s.split(":");
        if (parts.size() < 2)
            return;

        QStringList path_parts = parts[0].split("/",QString::SkipEmptyParts);
        currentElement = root_; parent_component_ = Selection::ROOT;
        foreach(QString s, path_parts)
        {
            for(QDomElement e = currentElement.firstChildElement();
                !e.isNull();
                e = e.nextSiblingElement("component"))
            {
                if (e.attribute("name") == s)
                {
                    currentElement = e;
                    parent_component_ = components_.itemLink(s, parent_component_);
                    break;
                }
            }
        }
        adjust_components();

        min_time_ = getTime(parts[1].toInt());
        max_time_ = getTime(parts[2].toInt());
    }

// private functions

    Time OTF_trace_model::getTime(int t) const
    {
        return scalar_time<int>(t);
    }


    void OTF_trace_model::initialize()
    {

        QString max_time_at = root_.attribute("max_time");
        max_time_ = getTime(max_time_at.toInt());

        // Initialize components and states
/*
        components_.clear();
        states_.clear();
        typedef std::pair<QDomElement, int> node;
        std::queue<node> nodes;
        nodes.push(node(root_, Selection::ROOT));

        fprintf( stdout, "  $$$$$$    parent component %u \n", parent_component_);

        int current_link = components_.addItem("Component_1", parent_component_);
        current_link = components_.addItem("Component_2", parent_component_);
        parent_component_ = current_link;
*/
       /*
        QString fullComponentName = component_name(current_link, true);
        int sparent = states_.itemLink(fullComponentName);

        if (sparent == -1) {
            sparent = states_.addItem(fullComponentName);
            states_.setItemProperty(sparent, "component", current_link);
        }

        states_.addItem("STATE_1", sparent);
*/
/*
        while (!nodes.empty())
        {
            QDomElement parent = nodes.front().first.toElement();
            for (QDomElement e = parent.firstChildElement("component");
            !e.isNull();
            e = e.nextSiblingElement("component"))
            {
                int current_link = components_.addItem(e.attribute("name"),  nodes.front().second);
                nodes.push(node(e, current_link));

                QDomElement statesNode = e.firstChildElement("states");
                if (!statesNode.isNull()) {
                    QString fullComponentName = component_name(current_link, true);
                    int sparent = states_.itemLink(fullComponentName);

                    if (sparent == -1) {
                        sparent = states_.addItem(fullComponentName);
                        states_.setItemProperty(sparent, "component", current_link);
                    }

                    for (QDomElement stateNode = statesNode.firstChildElement("state");
                        !stateNode.isNull(); stateNode = stateNode.nextSiblingElement("state"))
                    {
                        states_.addItem(stateNode.attribute("name"), sparent);
                    }
                }
            }
            nodes.pop();
        }

        components_.setItemProperty(0, "current_parent", parent_component_);
        available_states_ = states_;
*/
        // Initialize events

        events_.clear();
        events_.addItem("Update");
        events_.addItem("Delay");
        events_.addItem("Send");
        events_.addItem("Receive");
        events_.addItem("Stop");

    }

    void OTF_trace_model::adjust_components()
    {
        visible_components_ = components_.enabledItems(parent_component_);
        components_.setItemProperty(0, "current_parent", parent_component_);

        subcomponents.clear();
        QDomNodeList nodes = currentElement.childNodes();
        foreach(int component, visible_components_)
        {
            int index = components_.itemIndex(component);
            subcomponents.push_back(nodes.at(index).toElement());
        }

        lifeline_map_.clear();
        for (int ll = 0; ll < visible_components_.size(); ll++)
        {
            QList<int> queue; queue << visible_components_[ll];
            while (!queue.isEmpty())
            {
                int comp = queue.takeFirst();
                lifeline_map_[comp] = ll;

                foreach(int child, components_.enabledItems(comp))
                    queue << child;
            }
        }

    }

    void OTF_trace_model::findNextItem(const QString& elementName)
    {
        if (!currentItem.isNull())
            currentItem = currentItem.nextSiblingElement();

        if (currentItem.isNull())
        {
            for(;;)
            {
                ++currentSubcomponent;
                if (unsigned(currentSubcomponent) >= subcomponents.size())
                    break;

                QDomElement group = subcomponents[currentSubcomponent]
                    .firstChildElement(elementName);

                if (!group.isNull())
                {
                    currentItem = group.firstChildElement();
                    break;
                }
            }
        }
    }
}
