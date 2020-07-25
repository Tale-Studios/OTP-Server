#pragma once
#include "DNAGroup.h"

class DNAVisGroup : public DNAGroup
{
  public:
    DNAVisGroup(const std::string& name);
    ~DNAVisGroup();

    void add_visible(const std::string& visible);
    bool remove_visible(const std::string& visible);
    size_t get_num_visibles();
    std::string get_visible(size_t index);
    INLINE std::string get_visible_name(size_t index)
    {
        return get_visible(index);
    }

  protected:
    std::vector<std::string> m_visibles;
};