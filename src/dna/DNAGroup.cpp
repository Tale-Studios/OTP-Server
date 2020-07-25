#include "DNAGroup.h"

DNAGroup::DNAGroup(const std::string& name): m_name(name), m_parent(nullptr),
                                             m_vis_group(nullptr)
{
}

DNAGroup::~DNAGroup()
{
}

void DNAGroup::add(DNAGroup* group)
{
    m_children.push_back(group);
}