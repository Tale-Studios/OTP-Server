// Filename: dcClassParameter.cpp
// Created by:  drose (18Jun04)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "dcClassParameter.h"
#include "dcClass.h"
#include "dcArrayParameter.h"
#include "hashGenerator.h"

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
DCClassParameter::
DCClassParameter(const DCClass *dclass) :
  _dclass(dclass)
{
  set_name(dclass->get_name());

  int num_fields = _dclass->get_num_inherited_fields();

  _has_nested_fields = true;
  _pack_type = PT_class;

  if (_dclass->has_constructor()) {
    DCField *field = _dclass->get_constructor();
    _nested_fields.push_back(field);
    _has_default_value = _has_default_value || field->has_default_value();
  }
  int i;
  for (i = 0 ; i < num_fields; i++) {
    DCField *field = _dclass->get_inherited_field(i);
    if (!field->as_molecular_field()) {
      _nested_fields.push_back(field);
      _has_default_value = _has_default_value || field->has_default_value();
    }
  }
  _num_nested_fields = _nested_fields.size();

  // If all of the nested fields have a fixed byte size, then so does
  // the class (and its byte size is the sum of all of the nested
  // fields).
  _has_fixed_byte_size = true;
  _fixed_byte_size = 0;
  _has_fixed_structure = true;
  for (i = 0; i < _num_nested_fields; i++) {
    DCPackerInterface *field = get_nested_field(i);
    _has_fixed_byte_size = _has_fixed_byte_size && field->has_fixed_byte_size();
    _fixed_byte_size += field->get_fixed_byte_size();
    _has_fixed_structure = _has_fixed_structure && field->has_fixed_structure();

    _has_range_limits = _has_range_limits || field->has_range_limits();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::Copy Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
DCClassParameter::
DCClassParameter(const DCClassParameter &copy) :
  DCParameter(copy),
  _nested_fields(copy._nested_fields),
  _dclass(copy._dclass)
{
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::as_class_parameter
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
DCClassParameter *DCClassParameter::
as_class_parameter() {
  return this;
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::as_class_parameter
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
const DCClassParameter *DCClassParameter::
as_class_parameter() const {
  return this;
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::make_copy
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
DCParameter *DCClassParameter::
make_copy() const {
  return new DCClassParameter(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::is_valid
//       Access: Published, Virtual
//  Description: Returns false if the type is an invalid type
//               (e.g. declared from an undefined typedef), true if
//               it is valid.
////////////////////////////////////////////////////////////////////
bool DCClassParameter::
is_valid() const {
  return !_dclass->is_bogus_class();
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::get_class
//       Access: Published
//  Description: Returns the class object this parameter represents.
////////////////////////////////////////////////////////////////////
const DCClass *DCClassParameter::
get_class() const {
  return _dclass;
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::get_nested_field
//       Access: Public, Virtual
//  Description: Returns the DCPackerInterface object that represents
//               the nth nested field.  This may return NULL if there
//               is no such field (but it shouldn't do this if n is in
//               the range 0 <= n < get_num_nested_fields()).
////////////////////////////////////////////////////////////////////
DCPackerInterface *DCClassParameter::
get_nested_field(int n) const {
  nassertr(n >= 0 && n < (int)_nested_fields.size(), NULL);
  return _nested_fields[n];
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::output_instance
//       Access: Public, Virtual
//  Description: Formats the parameter in the C++-like dc syntax as a
//               typename and identifier.
////////////////////////////////////////////////////////////////////
void DCClassParameter::
output_instance(ostream &out, bool brief, const string &prename,
                const string &name, const string &postname) const {
  if (get_typedef() != (DCTypedef *)NULL) {
    output_typedef_name(out, brief, prename, name, postname);

  } else {
    _dclass->output_instance(out, brief, prename, name, postname);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::generate_hash
//       Access: Public, Virtual
//  Description: Accumulates the properties of this type into the
//               hash.
////////////////////////////////////////////////////////////////////
void DCClassParameter::
generate_hash(HashGenerator &hashgen) const {
  DCParameter::generate_hash(hashgen);
  _dclass->generate_hash(hashgen);
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::do_check_match
//       Access: Protected, Virtual
//  Description: Returns true if the other interface is bitwise the
//               same as this one--that is, a uint32 only matches a
//               uint32, etc. Names of components, and range limits,
//               are not compared.
////////////////////////////////////////////////////////////////////
bool DCClassParameter::
do_check_match(const DCPackerInterface *other) const {
  return other->do_check_match_class_parameter(this);
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::do_check_match_class_parameter
//       Access: Protected, Virtual
//  Description: Returns true if this field matches the indicated
//               class parameter, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCClassParameter::
do_check_match_class_parameter(const DCClassParameter *other) const {
  if (_nested_fields.size() != other->_nested_fields.size()) {
    return false;
  }
  for (size_t i = 0; i < _nested_fields.size(); i++) {
    if (!_nested_fields[i]->check_match(other->_nested_fields[i])) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCClassParameter::do_check_match_array_parameter
//       Access: Protected, Virtual
//  Description: Returns true if this field matches the indicated
//               array parameter, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCClassParameter::
do_check_match_array_parameter(const DCArrayParameter *other) const {
  if ((int)_nested_fields.size() != other->get_array_size()) {
    // We can only match a fixed-size array whose size happens to
    // exactly match our number of fields.
    return false;
  }

  const DCPackerInterface *element_type = other->get_element_type();
  for (size_t i = 0; i < _nested_fields.size(); i++) {
    if (!_nested_fields[i]->check_match(element_type)) {
      return false;
    }
  }

  return true;
}
