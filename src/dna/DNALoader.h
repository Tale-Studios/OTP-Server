#pragma once
#include "DNAGroup.h"
#include "DNAStorage.h"

class DNALoader
{
  public:
    DNALoader();
    ~DNALoader();

    DNAGroup* load_DNA_file_AI(DNAStorage* store, std::istream* in, const std::string &filename);

  private:
    DNAStorage* m_cur_store;
    DNAGroup* m_cur_comp;
};