#pragma once
#include "core/global.h"

class DNAGroup
{
  public:
    DNAGroup(const std::string& name);
    ~DNAGroup();

    INLINE void set_name(const std::string& name)
    {
        m_name = name;
    }

    INLINE std::string get_name()
    {
        return m_name;
    }

    INLINE void set_parent(DNAGroup* parent)
    {
        m_parent = parent;
    }

    INLINE DNAGroup* get_parent()
    {
        return m_parent;
    }

    INLINE void set_vis_group(DNAGroup* vis_group)
    {
        m_vis_group = vis_group;
    }

    INLINE DNAGroup* get_vis_group()
    {
        return m_vis_group;
    }

    void add(DNAGroup* group);

  protected:
    std::vector<DNAGroup*> m_children;

    std::string m_name;
    DNAGroup* m_parent;
    DNAGroup* m_vis_group;
};