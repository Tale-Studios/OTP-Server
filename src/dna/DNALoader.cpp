#include "DNALoader.h"
#include "DNAGroup.h"
#include "DNAVisGroup.h"
#include "DNALexerDefs.h"
#include "DNAParserDefs.h"

DNALoader::DNALoader(): m_cur_comp(nullptr), m_cur_store(nullptr)
{
}

DNALoader::~DNALoader()
{
}

DNAGroup* DNALoader::load_DNA_file_AI(DNAStorage* store, std::istream* in, const std::string &filename)
{
    m_cur_store = store;
    m_cur_comp = new DNAGroup("root");
    dna_init_parser(*in, filename, this, m_cur_store, m_cur_comp);
    dnayyparse();
    dna_cleanup_parser();

    m_cur_store = nullptr;

    if (dna_error_count() != 0)
        m_cur_comp = nullptr;

    return m_cur_comp;
}