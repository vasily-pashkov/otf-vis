#include "grx.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
//#include <tree.h>
//#include "/vasya/libxml2/include/libxml/tree.h"
//#include "/vasya/libxml2/include/libxml/parse.h"


#include <algorithm>
#include <list>
#include <sstream>
#include <fstream>
using namespace std;

const ComponentTree::Link ComponentTree::ROOT;

ComponentTree::ComponentTree()
{
    names.push_back("ROOT");
    types.push_back(UNKNOWN);
    procs.push_back(vector<int>());
    links.push_back(std::vector<Link>());
    parents.push_back(ROOT);
}

bool ComponentTree::loadFromFile(const std::string & fileName)
{
    //LIBXML_TEST_VERSION

    // Load and parse xml file using libxml2
    xmlDocPtr doc = xmlReadFile(fileName.c_str(), NULL, 0);
    if (!doc) return false;

    // Get root element
    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    if (!root_element) return false;

    //-----------------------------------------------------
    // Traverse xml tree and fill internal arrays.
    //-----------------------------------------------------

    list<xmlNodePtr> node_queue;
    list<Link> parent_queue;
    node_queue.push_back(root_element);
    parent_queue.push_back(ROOT);

    bool is_OK = true;
    while (!node_queue.empty()) {
        xmlNodePtr cur_node = node_queue.front();
        node_queue.pop_front();

        Link parent = parent_queue.front();
        parent_queue.pop_front();

        for (xmlNodePtr node = cur_node->children; node; node = node->next) {

            if (node->type != XML_ELEMENT_NODE) continue;
            if (strcmp((const char*)node->name, "lifeline"))
                { is_OK = false; break; }

            // Found lifeline element, let's obtain its attributes...

            string name;
            ComponentType type = UNKNOWN;
            vector<int> proc;

            for (xmlAttrPtr prop = node->properties; prop; prop = prop->next) {
                if (strcmp((const char*)prop->name, "name") == 0)
                    name = (const char*)prop->children->content;

                if (strcmp((const char*)prop->name, "type") == 0)
                {
                    const char * type_str = (const char*)prop->children->content;

                    if      (strcmp(type_str, "rchm") == 0)             type = RCHM;
                    else if (strcmp(type_str, "chm") == 0)              type = CHM;
                    else if (strcmp(type_str, "channel") == 0)          type = CHANNEL;
                    else if (strcmp(type_str, "interface") == 0)        type = INTERFACE;
                    else if (strcmp(type_str, "external_objects") == 0) type = EXTERNAL_OBJECTS;
                }

                if (strcmp((const char*)prop->name, "proc") == 0) {
                    // Split proc attribute value using comma as delimiter
                    // and put it to proc array.

                    char proc_str[256];
                    strncpy(proc_str, (const char*)prop->children->content, 256);

                    char * s = strtok(proc_str, ",");
                    proc.push_back(atoi(s));

                    while ((s = strtok(0, ",")) != 0)
                        proc.push_back(atoi(s));
                }
            }

            // Create new lifeline.
            Link link = addLifeline(name, type, proc, parent);

            node_queue.push_back(node);
            parent_queue.push_back(link);
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return is_OK;
}

string itoa(int n)
{
    std::ostringstream o; o << n;
    return o.str();
}

void ComponentTree::saveToFile(const std::string & fileName) const
{
    ofstream f(fileName.c_str());
    f << toXML();
    f.close();
}

string ComponentTree::toXML() const
{
    LIBXML_TEST_VERSION

    // Create new xml document and root element
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);

    //-----------------------------------------------------
    // Traverse component tree and fill xml document.
    //-----------------------------------------------------

    list<xmlNodePtr> parent_queue;
    list<Link> comp_queue;
    parent_queue.push_back(root_node);
    comp_queue.push_back(ROOT);

    while (!comp_queue.empty()) {
        Link comp = comp_queue.front();
        comp_queue.pop_front();

        xmlNodePtr parent = parent_queue.front();
        parent_queue.pop_front();

        for (int i = 0; i < nestedCount(comp); i++)
        {
            Link link = this->link(i, comp);

            xmlNodePtr node = xmlNewChild(parent, NULL, BAD_CAST "lifeline", NULL);
            xmlNewProp(node, BAD_CAST "name", BAD_CAST name(link).c_str());

            switch (type(link)) {
                case RCHM:             xmlNewProp(node, BAD_CAST "type", BAD_CAST "rchm"); break;
                case CHM:              xmlNewProp(node, BAD_CAST "type", BAD_CAST "chm"); break;
                case CHANNEL:          xmlNewProp(node, BAD_CAST "type", BAD_CAST "channel"); break;
                case INTERFACE:        xmlNewProp(node, BAD_CAST "type", BAD_CAST "interface"); break;
                case EXTERNAL_OBJECTS: xmlNewProp(node, BAD_CAST "type", BAD_CAST "external_objects"); break;
                case UNKNOWN: break;
            }

            if (!procs[link].empty()) {
                string proc_str = itoa(procs[link][0]);
                for (uint j = 1; j < procs[link].size(); j++)
                    proc_str += ","+itoa(procs[link][j]);
                xmlNewProp(node, BAD_CAST "proc", BAD_CAST proc_str.c_str());
            }

            if (nestedCount(link) != 0) {
                comp_queue.push_front(link);
                parent_queue.push_front(node);
            }
        }
    }

    // Dump xml to string
    xmlChar * xmlStr; int xmlStrLen;
    xmlDocDumpFormatMemoryEnc(doc, &xmlStr, &xmlStrLen, "KOI8-R", 1);
    string result((char*)xmlStr);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return result;
}

ComponentTree::Link ComponentTree::addLifeline(const std::string & name,      ComponentType type,
                                               const std::vector<int> & proc, ComponentTree::Link parent)
{
    assert(parent >= 0 && parent < names.size());
    Link link = names.size();

    // Put the data into internal arrays
    names.push_back(name);
    types.push_back(type);
    procs.push_back(proc);
    links.push_back(std::vector<Link>());
    parents.push_back(parent);
    links[parent].push_back(link);

    for (unsigned i = 0; i < proc.size(); i++)
        proc2CompMap[proc[i]].push_back(link);

    return link;
}
