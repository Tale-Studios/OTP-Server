#pragma once
#include "DNAGroup.h"
#include "DNAStorage.h"
#include "core/Logger.h"

class DNALoader
{
  public:
    DNALoader();
    ~DNALoader();

    DNAGroup* load_DNA_file_AI(DNAStorage* store, std::istream* in, const std::string &filename);

  private:
    std::unique_ptr<LogCategory> m_log;

    DNAStorage* m_cur_store;
    DNAGroup* m_cur_comp;
};