#include <set>
#include <map>
#include "dclass/dcClass.h"
#include "dclass/dcField.h"

// pointer comparison implementation for fields

struct FieldPtrComp {
    inline bool operator()(DCField* lhs, DCField* rhs) const;
};

inline bool FieldPtrComp::operator()(DCField* lhs, DCField* rhs) const
{
    return lhs->get_number() < rhs->get_number();
}

// Convenience typedefs
typedef std::set<DCField*, FieldPtrComp> FieldSet;
typedef std::map<DCField*, std::vector<uint8_t>, FieldPtrComp> FieldValues;