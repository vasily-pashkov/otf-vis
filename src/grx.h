#ifndef GRX_HPP
#define GRX_HPP

#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include <algorithm>

// All prefixes in UTF-8 encoding!
#define RCHM_PREFIX       "РЧМ "
#define CHM_PREFIX        "ЧМ   "
#define CHANNEL_PREFIX    "К      "
#define INTERFACE_PREFIX  "        "
#define EXTOBJECTS        "Внешние объекты"

/** Class contains hierarchy of components displaying in vis.
    Provide methods for load and save data to xml file and
    various methods for creating and access to hierarchy of components. */
class ComponentTree {

public:

    /** Datatype for links to the tree node.
        Use links for access to nodes. */
    typedef uint Link;

    enum ComponentType { UNKNOWN = 0, RCHM, CHM, CHANNEL, INTERFACE, EXTERNAL_OBJECTS };

public:

    /** Link to the root node, wich contains all toplevel components. */
    static const Link ROOT = 0;

    /** Class constructor. Create root node. */
    ComponentTree();

    /** Loads and parses xml file. */
    bool loadFromFile(const std::string & fileName);

    /** Saves hierarchy to xml file. */
    void saveToFile(const std::string & fileName) const;

    /** Returns component tree in xml format. */
    std::string toXML() const;

    /** Returns link to the child with given number. */
    Link link(uint index, Link parent) const;

    /** */
    const std::vector<Link> & link(int proc) const;

    /** Checks link. */
    bool isLinkValid(Link link) const;

    /** Returns link to the parent node. */
    Link parent(Link link) const;

    /** Returns an index of given node at the parent's children list. */
    int index(Link link) const;

    /** Returns a count of children of given node. */
    int nestedCount(Link parent) const;

    /** Returns the name of lifeline. */
    const std::string & name(Link link) const;
    const std::string & name(uint index, Link parent) const;

    ComponentType type(Link link) const;
    ComponentType type(uint index, Link parent) const;

    /** Returns the list of process numbers for lifeline.
        Normally each lifeline corresponds to one process,
        but some special lifelines like 'External components'
        may corresponds to a few process. */
    const std::vector<int> & proc(Link link) const;
    const std::vector<int> & proc(uint index, Link parent) const;

    /** Add new lifeline with given name, procs, visibility state
        and attach it to given parent */
    Link addLifeline(const std::string & name, ComponentType type, const std::vector<int> & proc, Link parent);

private: /* members */

    std::vector<std::string> names;         ///< Lifeline names.
    std::vector<ComponentType> types;                 ///< Lifeline types.
    std::vector< std::vector<int> > procs;  ///< Lifeline procs.

    std::vector< std::vector<Link> > links;  ///< Links to lifeline children.
    std::vector<Link> parents;               ///< Link to lifeline parent.

    /// Map to correspond proccess number with lifeline for each hierarchy level.
    std::map< int, std::vector<Link> > proc2CompMap;

};

//-------------------------------------------------------------------------------------------------
// DEFINISIONS OF INLINE METHODS
//-------------------------------------------------------------------------------------------------

inline ComponentTree::Link ComponentTree::link(uint index, ComponentTree::Link parent) const
{
    assert(parent >= 0 && parent < links.size());
    assert(index >= 0 && index < links[parent].size());
    return links[parent][index];
}

inline int ComponentTree::nestedCount(ComponentTree::Link parent) const
{
    assert(parent < links.size());
    return links[parent].size();
}

inline bool ComponentTree::isLinkValid(ComponentTree::Link link) const
{
    return link < links.size();
}

inline ComponentTree::Link ComponentTree::parent(ComponentTree::Link link) const
{
    assert(link < links.size());
    return parents[link];
}

inline int ComponentTree::index(ComponentTree::Link link) const
{
    assert(link < links.size());
    Link parent = parents[link];
    return find(links[parent].begin(), links[parent].end(), link)
        - links[parent].begin();
}

inline const std::string & ComponentTree::name(ComponentTree::Link link) const
{
    assert(link < names.size());
    return names[link];
}

inline const std::string & ComponentTree::name(uint index, ComponentTree::Link parent) const
{
    assert(parent < links.size());
    assert(index < links[parent].size());

    return name(link(index, parent));
}

inline ComponentTree::ComponentType ComponentTree::type(Link link) const
{
    assert(link < names.size());
    return types[link];
}

inline ComponentTree::ComponentType ComponentTree::type(uint index, Link parent) const
{
    assert(parent < links.size());
    assert(index < links[parent].size());

    return type(link(index, parent));
}

inline const std::vector<int> & ComponentTree::proc(ComponentTree::Link link) const
{
    assert(link < procs.size());
    return procs[link];
}

inline const std::vector<int> & ComponentTree::proc(uint index, ComponentTree::Link parent) const
{
    assert(parent < links.size());
    assert(index < links[parent].size());
    return proc(link(index, parent));
}

inline const std::vector<ComponentTree::Link> & ComponentTree::link(int proc) const
{
    assert(proc2CompMap.count(proc) != 0);
    return (*proc2CompMap.find(proc)).second;
}

#endif
