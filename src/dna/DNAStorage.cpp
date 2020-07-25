#include "DNAStorage.h"
#include "DNAVisGroup.h"

DNAStorage::DNAStorage()
{
}

DNAStorage::~DNAStorage()
{
}

void DNAStorage::store_DNA_vis_group(DNAVisGroup* group)
{
    m_vis_groups.push_back(group);
}

size_t DNAStorage::get_num_DNA_vis_groups()
{
    return m_vis_groups.size();
}

size_t DNAStorage::get_num_DNA_vis_groups_AI()
{
    return get_num_DNA_vis_groups();
}

size_t DNAStorage::get_num_visibles_in_DNA_vis_group(size_t index)
{
    return get_DNA_vis_group_AI(index)->get_num_visibles();
}

std::string DNAStorage::get_visible_name(size_t index, size_t visible_index)
{
    return get_DNA_vis_group_AI(index)->get_visible_name(visible_index);
}

DNAVisGroup* DNAStorage::get_DNA_vis_group_AI(size_t index)
{
    return m_vis_groups.at(index);
}

std::string DNAStorage::get_DNA_vis_group_name(size_t index)
{
    return get_DNA_vis_group_AI(index)->get_name();
}

void DNAStorage::reset_DNA_vis_groups()
{
    m_vis_groups.clear();
}

void DNAStorage::reset_DNA_vis_groups_AI()
{
    reset_DNA_vis_groups();
}