// Filename: dcbase.h
// Created by:  drose (05Oct00)
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

#ifndef DCBASE_H
#define DCBASE_H

// This file defines a few headers and stuff necessary for compilation
// of the files in this directory.  This is different from most of the
// other source directories within Panda, since the dcparser is
// designed to be compilable outside of Panda (for use by the server
// code).  Therefore, it must not depend on including any of the Panda
// header files, and we have to duplicate some setup stuff here.

#ifdef WITHIN_PANDA
// On the other hand, if WITHIN_PANDA is defined, we *are* safely
// within the Panda environment.

#include "directbase.h"
#include "pnotify.h"
#include "numeric_types.h"
#include "filename.h"
#include "pvector.h"
#include "pmap.h"
#include "pset.h"

#else  // NOT WITHIN_PANDA

#ifdef WIN32
/* C4786: 255 char debug symbols */
#pragma warning (disable : 4786)
/* C4503: decorated name length exceeded */
#pragma warning (disable : 4503)
#endif  /* WIN32_VC */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <string>
#include <assert.h>

// These header files are needed to compile dcLexer.cpp, the output
// from flex.  flex doesn't create a perfectly windows-friendly source
// file right out of the box.
#ifdef WIN32
#include <io.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif

using namespace std;

#define INLINE inline
#define TYPENAME typename

// These symbols are used within the Panda environment for exporting
// classes and functions to the scripting language.  They're largely
// meaningless if we're not compiling within Panda.
#define PUBLISHED public
#define BEGIN_PUBLISH
#define END_PUBLISH
#define BLOCKING

// Panda defines some assert-type macros.  We map those to the
// standard assert macro outside of Panda.
#define nassertr(condition, return_value) assert(condition)
#define nassertr_always(condition, return_value) assert(condition)
#define nassertv(condition) assert(condition)
#define nassertv_always(condition) assert(condition)

// Panda defines these export symbols for building DLL's.  Outside of
// Panda, we assume we're not putting this code in a DLL, so we define
// them to nothing.
#define EXPCL_DIRECT
#define EXPTP_DIRECT

// Panda defines a special Filename class.  We'll use an ordinary
// string instead.
typedef string Filename;

// Panda defines WORDS_BIGENDIAN on a bigendian machine; otherwise,
// the machine is assumed to be littleendian.  Outside of Panda,
// you're responsible for defining this yourself if necessary.
//#define WORDS_BIGENDIAN

#include <vector>
#include <map>
#include <set>
#define pvector vector
#define pmap map
#define pset set
typedef ifstream pifstream;
typedef ofstream pofstream;
typedef fstream pfstream;


#endif  // ENDIF WITHIN_PANDA


#include <stdint.h>

#ifdef DCPARSER_32BIT_LENGTH_TAG
typedef uint32_t length_tag_t;
#else
typedef uint16_t length_tag_t;
#endif

// TODO: Channels, doids, and zones should not exist in the dcparser! The code that uses these
// belongs in another module (for example, within the CMU code, etc....)
//typedef       unsigned long   CHANNEL_TYPE;
typedef uint64_t CHANNEL_TYPE;
typedef uint32_t DOID_TYPE;
typedef uint32_t ZONEID_TYPE;



#endif  // DCBASE_H
