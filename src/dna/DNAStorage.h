#pragma once
#include "DNAVisGroup.h"

class DNAStorage
{
  public:
    DNAStorage();
    ~DNAStorage();

    void store_DNA_vis_group(DNAVisGroup* group);
    size_t get_num_DNA_vis_groups();
    size_t get_num_DNA_vis_groups_AI();

    size_t get_num_visibles_in_DNA_vis_group(size_t index);
    std::string get_visible_name(size_t index, size_t visible_index);

    DNAVisGroup* get_DNA_vis_group_AI(size_t index);
    std::string get_DNA_vis_group_name(size_t index);

    void reset_DNA_vis_groups();
    void reset_DNA_vis_groups_AI();
  private:
    std::vector<DNAVisGroup*> m_vis_groups;
};