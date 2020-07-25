#include "DNAVisGroup.h"

DNAVisGroup::DNAVisGroup(const std::string& name): DNAGroup(name)
{
    m_vis_group = this;
}

DNAVisGroup::~DNAVisGroup()
{
}

void DNAVisGroup::add_visible(const std::string& visible)
{
    m_visibles.push_back(visible);
}

bool DNAVisGroup::remove_visible(const std::string& visible)
{
    std::vector<std::string>::iterator it = std::find(m_visibles.begin(), m_visibles.end(), visible);
    if (it == m_visibles.end())
        return false;

    m_visibles.erase(it);
    return true;
}

size_t DNAVisGroup::get_num_visibles()
{
    return m_visibles.size();
}

std::string DNAVisGroup::get_visible(size_t index)
{
    return m_visibles.at(index);
}