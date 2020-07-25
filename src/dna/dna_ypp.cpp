/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         dnayyparse
#define yylex           dnayylex
#define yyerror         dnayyerror
#define yylval          dnayylval
#define yychar          dnayychar
#define yydebug         dnayydebug
#define yynerrs         dnayynerrs

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 1 "dna.ypp"

#include "DNALexerDefs.h"
#include "DNAParserDefs.h"

// #define DO_YYDEBUG

#ifdef DO_YYDEBUG
#define YYDEBUG 1
#endif
#include "dna_ypp.hpp"

#include "DNAVisGroup.h"

// Because our token type contains objects of type string, which
// require correct copy construction (and not simply memcpying), we
// cannot use bison's built-in auto-stack-grow feature.  As an easy
// solution, we ensure here that we have enough yacc stack to start
// with, and that it doesn't ever try to grow.
#define YYINITDEPTH 1000
#define YYMAXDEPTH 1000

////////////////////////////////////////////////////////////////////
// Static variables
////////////////////////////////////////////////////////////////////

static DNALoader* _loader = nullptr;
static DNAStorage* _store = nullptr;
static DNAGroup* _cur_comp = nullptr;

////////////////////////////////////////////////////////////////////
// Defining the interface to the parser.
////////////////////////////////////////////////////////////////////

void dna_init_parser(std::istream &in, const std::string &filename,
                     DNALoader* loader, DNAStorage* store,
                     DNAGroup* root) {
#ifdef DO_YYDEBUG
  dnayydebug = 1;
#endif
  dna_init_lexer(in, filename);
  _loader = loader;
  _store = store;
  _cur_comp = root;
}

void dna_cleanup_parser() {
  _loader = nullptr;
  _store = nullptr;
  _cur_comp = nullptr;
}


/* Line 371 of yacc.c  */
#line 128 "dna_ypp.cpp"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "dna_ypp.hpp".  */
#ifndef YY_DNAYY_DNA_YPP_HPP_INCLUDED
# define YY_DNAYY_DNA_YPP_HPP_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int dnayydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SIGNED_INTEGER = 258,
     UNSIGNED_INTEGER = 259,
     FLOAT_REAL = 260,
     UNQUOTED_STRING = 261,
     QUOTED_STRING = 262,
     STORE_SUIT_POINT = 263,
     DNAGROUP = 264,
     VISGROUP = 265,
     VIS = 266,
     DNA_STREET_POINT = 267,
     DNA_FRONT_DOOR_POINT = 268,
     DNA_SIDE_DOOR_POINT = 269,
     DNA_COGHQ_IN_POINT = 270,
     DNA_COGHQ_OUT_POINT = 271,
     SUIT_EDGE = 272,
     BATTLE_CELL = 273,
     PROP = 274,
     POS = 275,
     HPR = 276,
     SCALE = 277,
     CODE = 278,
     COLOR = 279,
     MODEL = 280,
     STORE_NODE = 281,
     SIGN = 282,
     BASELINE = 283,
     WIDTH = 284,
     HEIGHT = 285,
     STOMP = 286,
     STUMBLE = 287,
     INDENT = 288,
     WIGGLE = 289,
     KERN = 290,
     DNATEXT = 291,
     LETTERS = 292,
     STORE_FONT = 293,
     FLAT_BUILDING = 294,
     WALL = 295,
     WINDOWS = 296,
     COUNT = 297,
     CORNICE = 298,
     LANDMARK_BUILDING = 299,
     TITLE = 300,
     ARTICLE = 301,
     BUILDING_TYPE = 302,
     DOOR = 303,
     STORE_TEXTURE = 304,
     STREET = 305,
     TEXTURE = 306,
     GRAPHIC = 307,
     HOODMODEL = 308,
     PLACEMODEL = 309,
     FLAGS = 310,
     NODE = 311,
     FLAT_DOOR = 312,
     ANIM = 313,
     CELL_ID = 314,
     ANIM_PROP = 315,
     INTERACTIVE_PROP = 316,
     ANIM_BUILDING = 317
   };
#endif
/* Tokens.  */
#define SIGNED_INTEGER 258
#define UNSIGNED_INTEGER 259
#define FLOAT_REAL 260
#define UNQUOTED_STRING 261
#define QUOTED_STRING 262
#define STORE_SUIT_POINT 263
#define DNAGROUP 264
#define VISGROUP 265
#define VIS 266
#define DNA_STREET_POINT 267
#define DNA_FRONT_DOOR_POINT 268
#define DNA_SIDE_DOOR_POINT 269
#define DNA_COGHQ_IN_POINT 270
#define DNA_COGHQ_OUT_POINT 271
#define SUIT_EDGE 272
#define BATTLE_CELL 273
#define PROP 274
#define POS 275
#define HPR 276
#define SCALE 277
#define CODE 278
#define COLOR 279
#define MODEL 280
#define STORE_NODE 281
#define SIGN 282
#define BASELINE 283
#define WIDTH 284
#define HEIGHT 285
#define STOMP 286
#define STUMBLE 287
#define INDENT 288
#define WIGGLE 289
#define KERN 290
#define DNATEXT 291
#define LETTERS 292
#define STORE_FONT 293
#define FLAT_BUILDING 294
#define WALL 295
#define WINDOWS 296
#define COUNT 297
#define CORNICE 298
#define LANDMARK_BUILDING 299
#define TITLE 300
#define ARTICLE 301
#define BUILDING_TYPE 302
#define DOOR 303
#define STORE_TEXTURE 304
#define STREET 305
#define TEXTURE 306
#define GRAPHIC 307
#define HOODMODEL 308
#define PLACEMODEL 309
#define FLAGS 310
#define NODE 311
#define FLAT_DOOR 312
#define ANIM 313
#define CELL_ID 314
#define ANIM_PROP 315
#define INTERACTIVE_PROP 316
#define ANIM_BUILDING 317



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE dnayylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int dnayyparse (void *YYPARSE_PARAM);
#else
int dnayyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int dnayyparse (void);
#else
int dnayyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_DNAYY_DNA_YPP_HPP_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 317 "dna_ypp.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  87
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1328

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  127
/* YYNRULES -- Number of rules.  */
#define YYNRULES  230
/* YYNRULES -- Number of states.  */
#define YYNSTATES  496

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   317

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    64,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    65,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    10,    12,    14,    16,    18,
      20,    22,    24,    28,    37,    48,    50,    52,    54,    56,
      58,    60,    62,    65,    68,    71,    76,    81,    84,    86,
      92,    93,    95,    97,    99,   101,   103,   105,   107,   109,
     111,   113,   115,   117,   119,   121,   123,   125,   130,   135,
     140,   145,   150,   155,   160,   165,   170,   175,   180,   185,
     190,   195,   200,   205,   210,   213,   216,   219,   222,   224,
     226,   228,   231,   234,   236,   238,   241,   243,   245,   247,
     249,   255,   262,   265,   267,   270,   273,   276,   279,   281,
     286,   291,   296,   301,   303,   305,   307,   309,   311,   313,
     315,   320,   325,   327,   329,   331,   333,   335,   337,   339,
     341,   343,   345,   350,   352,   354,   356,   358,   360,   362,
     364,   366,   368,   370,   372,   374,   376,   378,   380,   382,
     384,   386,   388,   390,   392,   394,   396,   398,   403,   411,
     416,   421,   426,   434,   439,   444,   449,   454,   459,   464,
     469,   474,   479,   484,   489,   494,   499,   504,   509,   514,
     519,   524,   529,   534,   539,   544,   552,   560,   568,   576,
     584,   592,   600,   603,   606,   608,   611,   614,   617,   619,
     622,   625,   628,   631,   633,   636,   639,   641,   644,   647,
     649,   652,   654,   657,   660,   662,   665,   668,   670,   673,
     676,   678,   681,   684,   686,   689,   692,   694,   697,   700,
     702,   705,   708,   711,   713,   716,   719,   721,   724,   727,
     729,   732,   735,   738,   743,   746,   748,   754,   761,   767,
     774
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      67,     0,    -1,    67,    68,    -1,    68,    -1,    71,    -1,
      82,    -1,   188,    -1,   192,    -1,   191,    -1,     4,    -1,
       3,    -1,     5,    -1,    69,    69,    69,    -1,     8,    63,
      69,    64,    72,    64,    70,    65,    -1,     8,    63,    69,
      64,    72,    64,    70,    64,    69,    65,    -1,    12,    -1,
      13,    -1,    14,    -1,    15,    -1,    16,    -1,     7,    -1,
       6,    -1,     9,    73,    -1,    56,    73,    -1,    10,    73,
      -1,    74,    63,   115,    65,    -1,    76,    63,   116,    65,
      -1,    79,    73,    -1,    81,    -1,    11,    63,    73,    79,
      65,    -1,    -1,    77,    -1,    78,    -1,    83,    -1,    92,
      -1,    93,    -1,    96,    -1,    87,    -1,    85,    -1,    88,
      -1,    89,    -1,    90,    -1,    91,    -1,    94,    -1,    95,
      -1,    86,    -1,    84,    -1,    75,    63,   177,    65,    -1,
     109,    63,   172,    65,    -1,   110,    63,   178,    65,    -1,
      97,    63,   172,    65,    -1,    98,    63,   173,    65,    -1,
      99,    63,   174,    65,    -1,   111,    63,   175,    65,    -1,
     112,    63,   176,    65,    -1,   100,    63,   179,    65,    -1,
     101,    63,   180,    65,    -1,   102,    63,   181,    65,    -1,
     103,    63,   182,    65,    -1,   104,    63,   183,    65,    -1,
     105,    63,   184,    65,    -1,   108,    63,   186,    65,    -1,
     106,    63,   185,    65,    -1,   107,    63,   185,    65,    -1,
      19,    73,    -1,    60,    73,    -1,    61,    73,    -1,    39,
      73,    -1,    40,    -1,    41,    -1,    43,    -1,    44,    73,
      -1,    62,    73,    -1,    48,    -1,    57,    -1,    50,    73,
      -1,    27,    -1,    52,    -1,    28,    -1,    36,    -1,    17,
      63,    69,    69,    65,    -1,    18,    63,    69,    69,    70,
      65,    -1,   115,    82,    -1,    81,    -1,   116,    82,    -1,
     116,   113,    -1,   116,   114,    -1,   116,    80,    -1,    81,
      -1,    20,    63,    70,    65,    -1,    21,    63,    70,    65,
      -1,    22,    63,    70,    65,    -1,    55,    63,    73,    65,
      -1,    82,    -1,   117,    -1,   118,    -1,   119,    -1,   156,
      -1,   165,    -1,   125,    -1,    59,    63,    69,    65,    -1,
      58,    63,    73,    65,    -1,   158,    -1,   166,    -1,   145,
      -1,   148,    -1,   152,    -1,   153,    -1,   151,    -1,   154,
      -1,   155,    -1,   120,    -1,    37,    63,    73,    65,    -1,
     146,    -1,   149,    -1,   160,    -1,   168,    -1,   147,    -1,
     150,    -1,   163,    -1,   170,    -1,   164,    -1,   171,    -1,
     144,    -1,   161,    -1,   169,    -1,   157,    -1,   139,    -1,
     140,    -1,   141,    -1,   142,    -1,   143,    -1,   159,    -1,
     167,    -1,   162,    -1,   137,    -1,   138,    -1,    51,    63,
      73,    65,    -1,    24,    63,    69,    69,    69,    69,    65,
      -1,    45,    63,    73,    65,    -1,    46,    63,    73,    65,
      -1,    47,    63,    73,    65,    -1,    24,    63,    69,    69,
      69,    69,    65,    -1,    58,    63,    73,    65,    -1,    42,
      63,    69,    65,    -1,    29,    63,    69,    65,    -1,    29,
      63,    69,    65,    -1,    29,    63,    69,    65,    -1,    30,
      63,    69,    65,    -1,    30,    63,    69,    65,    -1,    30,
      63,    69,    65,    -1,    31,    63,    69,    65,    -1,    33,
      63,    69,    65,    -1,    35,    63,    69,    65,    -1,    32,
      63,    69,    65,    -1,    34,    63,    69,    65,    -1,    23,
      63,    73,    65,    -1,    23,    63,    73,    65,    -1,    23,
      63,    73,    65,    -1,    23,    63,    73,    65,    -1,    23,
      63,    73,    65,    -1,    23,    63,    73,    65,    -1,    23,
      63,    73,    65,    -1,    23,    63,    73,    65,    -1,    23,
      63,    73,    65,    -1,    24,    63,    69,    69,    69,    69,
      65,    -1,    24,    63,    69,    69,    69,    69,    65,    -1,
      24,    63,    69,    69,    69,    69,    65,    -1,    24,    63,
      69,    69,    69,    69,    65,    -1,    24,    63,    69,    69,
      69,    69,    65,    -1,    24,    63,    69,    69,    69,    69,
      65,    -1,    24,    63,    69,    69,    69,    69,    65,    -1,
     172,   121,    -1,   172,   122,    -1,    81,    -1,   173,   121,
      -1,   173,   122,    -1,   173,   123,    -1,    81,    -1,   174,
     121,    -1,   174,   122,    -1,   174,   123,    -1,   174,   124,
      -1,    81,    -1,   175,   121,    -1,   175,   126,    -1,    81,
      -1,   176,   121,    -1,   176,   127,    -1,    81,    -1,   177,
     121,    -1,    81,    -1,   178,   121,    -1,   178,   128,    -1,
      81,    -1,   179,   121,    -1,   179,   129,    -1,    81,    -1,
     180,   121,    -1,   180,   130,    -1,    81,    -1,   181,   121,
      -1,   181,   131,    -1,    81,    -1,   182,   121,    -1,   182,
     132,    -1,    81,    -1,   183,   121,    -1,   183,   133,    -1,
      81,    -1,   184,   121,    -1,   184,   133,    -1,   184,   134,
      -1,    81,    -1,   185,   121,    -1,   185,   135,    -1,    81,
      -1,   186,   121,    -1,   186,   136,    -1,    81,    -1,    25,
      73,    -1,    53,    73,    -1,    54,    73,    -1,   187,    63,
     189,    65,    -1,   189,   190,    -1,    81,    -1,    26,    63,
      73,    73,    65,    -1,    26,    63,    73,    73,    73,    65,
      -1,    49,    63,    73,    73,    65,    -1,    49,    63,    73,
      73,    73,    65,    -1,    38,    63,    73,    73,    73,    65,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   125,   125,   126,   129,   130,   131,   132,   133,   136,
     140,   144,   149,   153,   156,   160,   164,   168,   172,   176,
     181,   185,   190,   199,   203,   212,   217,   222,   227,   232,
     240,   243,   244,   245,   246,   247,   248,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   263,   268,   273,
     278,   282,   286,   291,   296,   299,   304,   309,   314,   319,
     323,   328,   333,   337,   342,   346,   350,   354,   358,   362,
     366,   370,   374,   378,   382,   386,   390,   394,   398,   402,
     406,   410,   414,   415,   418,   419,   420,   421,   422,   425,
     429,   433,   437,   441,   442,   443,   444,   447,   448,   451,
     454,   458,   462,   463,   464,   465,   466,   467,   468,   469,
     470,   471,   474,   478,   479,   480,   481,   484,   487,   488,
     489,   492,   493,   494,   497,   498,   501,   502,   503,   504,
     505,   508,   511,   512,   515,   516,   517,   520,   524,   528,
     532,   536,   540,   544,   548,   552,   556,   560,   564,   568,
     572,   576,   580,   584,   588,   592,   596,   600,   604,   608,
     612,   616,   620,   624,   628,   632,   636,   640,   644,   648,
     652,   656,   660,   661,   662,   665,   666,   667,   668,   671,
     672,   673,   674,   675,   678,   679,   680,   683,   684,   685,
     688,   689,   692,   693,   694,   697,   698,   699,   702,   703,
     704,   707,   708,   709,   712,   713,   714,   717,   718,   719,
     722,   723,   724,   725,   728,   729,   730,   733,   734,   735,
     738,   741,   744,   748,   751,   752,   755,   758,   762,   765,
     769
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SIGNED_INTEGER", "UNSIGNED_INTEGER",
  "FLOAT_REAL", "UNQUOTED_STRING", "QUOTED_STRING", "STORE_SUIT_POINT",
  "DNAGROUP", "VISGROUP", "VIS", "DNA_STREET_POINT",
  "DNA_FRONT_DOOR_POINT", "DNA_SIDE_DOOR_POINT", "DNA_COGHQ_IN_POINT",
  "DNA_COGHQ_OUT_POINT", "SUIT_EDGE", "BATTLE_CELL", "PROP", "POS", "HPR",
  "SCALE", "CODE", "COLOR", "MODEL", "STORE_NODE", "SIGN", "BASELINE",
  "WIDTH", "HEIGHT", "STOMP", "STUMBLE", "INDENT", "WIGGLE", "KERN",
  "DNATEXT", "LETTERS", "STORE_FONT", "FLAT_BUILDING", "WALL", "WINDOWS",
  "COUNT", "CORNICE", "LANDMARK_BUILDING", "TITLE", "ARTICLE",
  "BUILDING_TYPE", "DOOR", "STORE_TEXTURE", "STREET", "TEXTURE", "GRAPHIC",
  "HOODMODEL", "PLACEMODEL", "FLAGS", "NODE", "FLAT_DOOR", "ANIM",
  "CELL_ID", "ANIM_PROP", "INTERACTIVE_PROP", "ANIM_BUILDING", "'['",
  "','", "']'", "$accept", "dna", "object", "number", "lpoint3f",
  "suitpoint", "suitpointtype", "string", "dnagroupdef", "dnanodedef",
  "visgroupdef", "dnagroup", "visgroup", "string_opt_list", "vis", "empty",
  "group", "dnanode", "dnanode_grp", "sign", "signgraphic", "prop",
  "signbaseline", "signtext", "flatbuilding", "wall", "windows", "cornice",
  "landmarkbuilding", "street", "door", "propdef", "animpropdef",
  "interactivepropdef", "flatbuildingdef", "walldef", "windowsdef",
  "cornicedef", "landmarkbuildingdef", "animbuildingdef", "doordef",
  "flatdoordef", "streetdef", "signdef", "signgraphicdef", "baselinedef",
  "textdef", "suitedge", "battlecell", "subgroup_list", "subvisgroup_list",
  "pos", "hpr", "scale", "flags", "dnanode_sub", "dnaprop_sub",
  "dnaanimprop_sub", "dnainteractiveprop_sub", "anim", "baseline_sub",
  "text_sub", "signgraphic_sub", "flatbuilding_sub", "wall_sub",
  "windows_sub", "cornice_sub", "landmarkbuilding_sub", "animbuilding_sub",
  "door_sub", "street_sub", "texture", "street_color", "title", "article",
  "building_type", "lb_wall_color", "ab_anim", "windowcount",
  "baseline_width", "signgraphic_width", "flatbuilding_width",
  "baseline_height", "signgraphic_height", "wall_height", "stomp",
  "indent", "kern", "stumble", "wiggle", "prop_code",
  "landmarkbuilding_code", "baseline_code", "door_code",
  "signgraphic_code", "cornice_code", "street_code", "wall_code",
  "windows_code", "prop_color", "baseline_color", "door_color",
  "signgraphic_color", "cornice_color", "wall_color", "windows_color",
  "subprop_list", "subanimprop_list", "subinteractiveprop_list",
  "subbaseline_list", "subtext_list", "subdnanode_list",
  "subsigngraphic_list", "subflatbuilding_list", "subwall_list",
  "subwindows_list", "subcornice_list", "sublandmarkbuilding_list",
  "subanimbuilding_list", "subdoor_list", "substreet_list", "modeldef",
  "model", "modelnode_list", "node", "store_texture", "font", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,    91,    44,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    68,    68,    68,    69,
      69,    69,    70,    71,    71,    72,    72,    72,    72,    72,
      73,    73,    74,    75,    76,    77,    78,    79,    79,    80,
      81,    82,    82,    82,    82,    82,    82,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    84,    85,    86,
      87,    87,    87,    88,    89,    90,    91,    92,    93,    94,
      94,    95,    96,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   115,   116,   116,   116,   116,   116,   117,
     118,   119,   120,   121,   121,   121,   121,   122,   122,   123,
     124,   125,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   127,   128,   128,   128,   128,   129,   130,   130,
     130,   131,   131,   131,   132,   132,   133,   133,   133,   133,
     133,   134,   135,   135,   136,   136,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   172,   172,   173,   173,   173,   173,   174,
     174,   174,   174,   174,   175,   175,   175,   176,   176,   176,
     177,   177,   178,   178,   178,   179,   179,   179,   180,   180,
     180,   181,   181,   181,   182,   182,   182,   183,   183,   183,
     184,   184,   184,   184,   185,   185,   185,   186,   186,   186,
     187,   187,   187,   188,   189,   189,   190,   190,   191,   191,
     192
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     8,    10,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     4,     4,     2,     1,     5,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     2,     2,     2,     2,     1,     1,
       1,     2,     2,     1,     1,     2,     1,     1,     1,     1,
       5,     6,     2,     1,     2,     2,     2,     2,     1,     4,
       4,     4,     4,     1,     1,     1,     1,     1,     1,     1,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     7,     4,
       4,     4,     7,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     7,     7,     7,     7,     7,
       7,     7,     2,     2,     1,     2,     2,     2,     1,     2,
       2,     2,     2,     1,     2,     2,     1,     2,     2,     1,
       2,     1,     2,     2,     1,     2,     2,     1,     2,     2,
       1,     2,     2,     1,     2,     2,     1,     2,     2,     1,
       2,     2,     2,     1,     2,     2,     1,     2,     2,     1,
       2,     2,     2,     4,     2,     1,     5,     6,     5,     6,
       6
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,    76,    78,    79,     0,
       0,    68,    69,    70,     0,    73,     0,     0,    77,     0,
       0,     0,    74,     0,     0,     0,     0,     3,     4,     0,
       0,     0,    31,    32,     5,    33,    46,    38,    45,    37,
      39,    40,    41,    42,    34,    35,    43,    44,    36,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     6,     8,     7,     0,
      21,    20,    22,    24,    64,   220,     0,    67,    71,     0,
      75,   221,   222,    23,    65,    66,    72,     1,     2,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    10,
       9,    11,     0,     0,     0,    83,     0,   191,     0,    88,
       0,   174,     0,   178,     0,   183,     0,   197,     0,   200,
       0,   203,     0,   206,     0,   209,     0,   213,     0,   216,
       0,     0,   219,     0,     0,   194,     0,   186,     0,   189,
       0,   225,     0,     0,     0,     0,    25,    82,     0,     0,
       0,    47,    93,    94,    95,    96,   190,     0,     0,     0,
      26,    87,    84,    85,    86,     0,     0,    50,   172,   173,
      97,    98,     0,    51,   175,   176,   177,    99,     0,    52,
     179,   180,   181,   182,     0,    55,   195,   196,   117,     0,
       0,     0,    56,   198,   199,   118,   119,   120,     0,     0,
       0,    57,   201,   202,   123,   121,   122,     0,     0,    58,
     204,   205,   124,   125,     0,     0,     0,     0,     0,    59,
     207,   208,   127,   128,   129,   130,   126,     0,    60,   210,
     211,   212,   131,     0,     0,    62,   214,   215,   132,   133,
      63,     0,     0,     0,    61,   217,   218,   135,   136,   134,
      48,     0,     0,     0,     0,    49,   192,   193,   113,   114,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    53,   111,   184,   185,   104,   105,   108,   106,
     107,   109,   110,   102,   103,     0,    54,   187,   188,     0,
     223,   224,    15,    16,    17,    18,    19,     0,     0,   228,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   230,   229,
       0,     0,     0,     0,    30,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    89,
      90,    91,     0,    28,     0,     0,   156,     0,   101,   100,
     147,   163,     0,   150,   164,     0,   144,   161,     0,   157,
       0,   139,   140,   141,   143,   159,     0,   162,     0,   137,
     160,     0,   146,   149,   158,     0,   145,   148,   151,   154,
     152,   155,   153,    92,   112,     0,     0,    13,    12,    29,
      27,    80,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   226,     0,     0,    81,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   227,    14,   165,   170,   171,
     169,   142,   167,   138,   168,   166
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    26,    27,   360,   361,    28,   307,    72,    29,    30,
      31,    32,    33,   412,   171,   121,   162,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,   173,   174,   116,
     120,   163,   164,   165,   283,   178,   179,   186,   193,   187,
     285,   298,   267,   197,   204,   213,   221,   231,   241,   247,
     256,   257,   258,   232,   233,   234,   235,   242,   214,   286,
     268,   198,   287,   269,   205,   288,   289,   290,   291,   292,
     180,   236,   293,   248,   270,   222,   259,   206,   215,   181,
     294,   249,   271,   223,   207,   216,   122,   124,   126,   148,
     150,   118,   146,   128,   130,   132,   134,   136,   138,   140,
     143,    65,    66,   152,   301,    67,    68
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -310
static const yytype_int16 yypact[] =
{
    1266,   -44,    59,    59,    59,    59,  -310,  -310,  -310,   -37,
      59,  -310,  -310,  -310,    59,  -310,   -13,    59,  -310,    59,
      59,    59,  -310,    59,    59,    59,    43,  -310,  -310,    -2,
       0,     6,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,     9,
      14,    15,    17,    22,    25,    26,    27,    31,    35,    38,
      46,    49,    51,    52,    53,    54,  -310,  -310,  -310,    41,
    -310,  -310,  -310,  -310,  -310,  -310,    59,  -310,  -310,    59,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,   -17,    59,    59,  -310,  1221,  -310,  1149,  -310,
    1185,  -310,   806,  -310,   605,  -310,   502,  -310,  1051,  -310,
     659,  -310,   708,  -310,   855,  -310,   217,  -310,   163,  -310,
     904,   953,  -310,   757,  1002,  -310,   556,  -310,   114,  -310,
    1100,  -310,   -16,    44,    59,    -1,  -310,  -310,    55,    56,
      57,  -310,  -310,  -310,  -310,  -310,  -310,    58,    62,    63,
    -310,  -310,  -310,  -310,  -310,    64,    66,  -310,  -310,  -310,
    -310,  -310,    67,  -310,  -310,  -310,  -310,  -310,    68,  -310,
    -310,  -310,  -310,  -310,    76,  -310,  -310,  -310,  -310,    77,
      93,    96,  -310,  -310,  -310,  -310,  -310,  -310,    97,    98,
     100,  -310,  -310,  -310,  -310,  -310,  -310,   102,   104,  -310,
    -310,  -310,  -310,  -310,   105,   115,   117,   118,   125,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,   126,  -310,  -310,
    -310,  -310,  -310,   129,   130,  -310,  -310,  -310,  -310,  -310,
    -310,   131,   132,   133,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,   134,   135,   137,   138,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,   142,   149,   151,   153,   154,   155,   159,   166,
     168,   169,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,   170,  -310,  -310,  -310,   171,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,   113,    37,  -310,
      42,    41,    41,    41,    59,    41,    41,    59,    41,    59,
      41,    41,    59,    41,    41,    59,    41,    41,    59,    41,
      59,    41,    59,    59,    59,    59,    59,    41,    59,    41,
      59,    59,    41,    41,    41,    59,    41,    41,    41,    41,
      41,    41,    41,    41,    59,    59,    59,    41,  -310,  -310,
      41,   165,   177,   178,  -310,    41,    41,   181,    41,   182,
     183,   184,   185,    41,   186,   187,    41,   189,   190,    41,
     194,    41,   201,   203,   205,   206,   207,    41,   210,    41,
     211,   215,    41,   216,   218,   219,    41,   221,   222,   223,
     224,   225,   227,   229,   231,   235,    59,    10,    41,  -310,
    -310,  -310,     2,  -310,   237,    41,  -310,    41,  -310,  -310,
    -310,  -310,    41,  -310,  -310,    41,  -310,  -310,    41,  -310,
      41,  -310,  -310,  -310,  -310,  -310,    41,  -310,    41,  -310,
    -310,    41,  -310,  -310,  -310,    41,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,    48,    41,  -310,  -310,  -310,
    -310,  -310,   239,    41,    41,    41,    41,    41,    41,    41,
      41,    41,  -310,   241,   244,  -310,   247,   253,   255,   256,
     258,   259,   261,   263,   269,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -310,  -310,   209,    39,  -309,  -310,  -310,    -3,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,   -66,    12,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,   167,  -111,    -4,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,   198,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,   236,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,   242,
    -310,  -310,  -310,  -310,  -310,  -310,  -310
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      73,    74,    75,   362,   363,    70,    71,    77,    70,    71,
     299,    78,    34,   185,    80,   191,    81,    82,    83,    69,
      84,    85,    86,   115,   117,   119,    76,   123,   125,   127,
     129,   131,   133,   135,   137,   139,   139,   142,    34,   145,
     147,   149,   151,    87,   109,   110,   111,   153,   407,   300,
      79,     1,     2,     3,    70,    71,   302,   303,   304,   305,
     306,    89,     4,    90,   309,    70,    71,   459,     5,    91,
       6,     7,    92,   113,   456,   457,   114,    93,    94,     8,
      95,     9,    10,    11,    12,    96,    13,    14,    97,    98,
      99,    15,    16,    17,   100,    18,    19,    20,   101,    21,
      22,   102,   358,    23,    24,    25,   462,   359,   112,   103,
     154,   155,   104,   472,   105,   106,   107,   108,   311,   312,
     313,   314,   192,     2,     3,   315,   316,   317,   157,   318,
     319,   320,   172,     4,   158,   159,   160,   272,   273,   321,
     322,     6,     7,   274,   275,   276,   277,   278,   279,   280,
       8,   308,   310,    10,    11,    12,   323,    13,    14,   324,
     325,   326,    15,   327,    17,   328,    18,   329,   330,   281,
      21,    22,     2,     3,    23,    24,    25,   357,   331,   282,
     332,   333,     4,   158,   159,   160,   224,   225,   334,   335,
       6,     7,   336,   337,   338,   339,   340,   341,   342,     8,
     343,   344,    10,    11,    12,   345,    13,    14,   226,   227,
     228,    15,   346,    17,   347,    18,   348,   349,   350,    21,
      22,   237,   351,    23,    24,    25,     2,     3,   238,   352,
     409,   353,   354,   355,   356,    88,     4,   158,   159,   160,
     224,   225,   410,   411,     6,     7,   416,   418,   419,   420,
     421,   423,   424,     8,   426,   427,    10,    11,    12,   429,
      13,    14,   226,   227,   228,    15,   431,    17,   432,    18,
     433,   434,   435,    21,    22,   437,   439,    23,    24,    25,
     440,   442,   229,   443,   444,   166,   446,   447,   448,   449,
     450,   184,   451,   190,   452,   196,   453,   203,   413,   212,
     454,   220,   461,   230,   475,   239,   485,   246,   246,   486,
     255,   364,   487,   266,   367,   284,   369,   297,   488,   372,
     489,   490,   375,   491,   492,   378,   493,   380,   494,   382,
     383,   384,   385,   386,   495,   388,   240,   390,   391,     0,
     144,     0,   395,     0,   141,     0,     0,     0,     0,     0,
       0,   404,   405,   406,   365,   366,     0,   368,     0,   370,
     371,     0,   373,   374,     0,   376,   377,     0,   379,     0,
     381,     0,     0,     0,     0,     0,   387,     0,   389,     0,
       0,   392,   393,   394,     0,   396,   397,   398,   399,   400,
     401,   402,   403,     0,     0,     0,     0,     0,     0,   408,
       0,     0,     0,   455,   414,   415,     0,   417,     0,   460,
       0,     0,   422,     0,     0,   425,     0,     0,   428,     0,
     430,     0,     0,     0,     0,     0,   436,     0,   438,     0,
       0,   441,     0,     0,     0,   445,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   458,     0,     0,
       0,     0,   473,     0,     0,     0,   463,     0,     0,     0,
       0,   464,     0,     0,   465,     0,     0,   466,     0,   467,
       0,     0,     0,     0,     0,   468,     0,   469,     0,     0,
     470,     0,     0,     0,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   474,     0,     0,     0,     0,
       0,     0,   476,   477,   478,   479,   480,   481,   482,   483,
     484,     2,     3,     0,     0,     0,     0,     0,     0,     0,
       0,     4,   158,   159,   160,   175,   176,     0,     0,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     8,     0,
       0,    10,    11,    12,     0,    13,    14,     0,     0,     0,
      15,     0,    17,     0,    18,     0,     0,     0,    21,    22,
     182,   188,    23,    24,    25,     2,     3,   189,     0,     0,
       0,     0,     0,     0,     0,     4,   158,   159,   160,   261,
     262,     0,     0,     6,     7,   263,   264,     0,     0,     0,
       0,     0,     8,     0,     0,    10,    11,    12,     0,    13,
      14,     0,     0,     0,    15,     0,    17,     0,    18,     0,
       0,     0,    21,    22,     2,     3,    23,    24,    25,     0,
       0,   265,     0,     0,     4,   158,   159,   160,   175,   176,
       0,     0,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     8,     0,     0,    10,    11,    12,     0,    13,    14,
       0,     0,     0,    15,     0,    17,     0,    18,     0,     0,
       0,    21,    22,   182,     0,    23,    24,    25,     2,     3,
     183,     0,     0,     0,     0,     0,     0,     0,     4,   158,
     159,   160,   199,   200,     0,     0,     6,     7,     0,   201,
       0,     0,     0,     0,     0,     8,     0,     0,    10,    11,
      12,     0,    13,    14,     0,     0,     0,    15,     0,    17,
       0,    18,     0,     0,     0,    21,    22,     2,     3,    23,
      24,    25,     0,     0,   202,     0,     0,     4,   158,   159,
     160,   208,   209,     0,     0,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     8,     0,     0,    10,    11,    12,
     210,    13,    14,     0,     0,     0,    15,     0,    17,     0,
      18,     0,     0,     0,    21,    22,     2,     3,    23,    24,
      25,     0,     0,   211,     0,     0,     4,   158,   159,   160,
     251,   252,     0,     0,     6,     7,     0,     0,     0,     0,
       0,     0,     0,     8,     0,     0,    10,    11,    12,     0,
      13,    14,     0,     0,     0,    15,     0,    17,   253,    18,
       0,     0,     0,    21,    22,     2,     3,    23,    24,    25,
       0,     0,   254,     0,     0,     4,   158,   159,   160,   175,
     176,     0,     0,     6,     7,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     0,    10,    11,    12,     0,    13,
      14,     0,     0,     0,    15,     0,    17,     0,    18,     0,
       0,     0,    21,    22,     2,     3,    23,    24,    25,     0,
       0,   177,     0,     0,     4,   158,   159,   160,   217,   218,
       0,     0,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     8,     0,     0,    10,    11,    12,     0,    13,    14,
       0,     0,     0,    15,     0,    17,     0,    18,     0,     0,
       0,    21,    22,     2,     3,    23,    24,    25,     0,     0,
     219,     0,     0,     4,   158,   159,   160,   243,   244,     0,
       0,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       8,     0,     0,    10,    11,    12,     0,    13,    14,     0,
       0,     0,    15,     0,    17,     0,    18,     0,     0,     0,
      21,    22,     2,     3,    23,    24,    25,     0,     0,   245,
       0,     0,     4,   158,   159,   160,   243,   244,     0,     0,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     8,
       0,     0,    10,    11,    12,     0,    13,    14,     0,     0,
       0,    15,     0,    17,     0,    18,     0,     0,     0,    21,
      22,     2,     3,    23,    24,    25,     0,     0,   250,     0,
       0,     4,   158,   159,   160,   175,   176,     0,     0,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     8,     0,
       0,    10,    11,    12,     0,    13,    14,     0,     0,     0,
      15,     0,    17,     0,    18,     0,     0,     0,    21,    22,
       2,     3,    23,    24,    25,     0,     0,   260,     0,     0,
       4,   158,   159,   160,     0,     0,     0,     0,     6,     7,
     194,     0,     0,     0,     0,     0,     0,     8,     0,     0,
      10,    11,    12,     0,    13,    14,     0,     0,     0,    15,
       0,    17,     0,    18,     0,     0,     0,    21,    22,     2,
       3,    23,    24,    25,     0,     0,   195,     0,     0,     4,
     158,   159,   160,     0,     0,     0,     0,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     8,   295,     0,    10,
      11,    12,     0,    13,    14,     0,     0,     0,    15,     0,
      17,     0,    18,     0,     0,     0,    21,    22,     2,     3,
      23,    24,    25,     0,     0,   296,     0,     0,     4,   158,
     159,   160,     0,     0,     0,     0,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     0,    10,    11,
      12,     0,    13,    14,     2,     3,   167,    15,     0,    17,
       0,    18,   168,   169,     4,    21,    22,     0,     0,    23,
      24,    25,     6,     7,   161,     0,     0,     0,     0,     0,
       0,     8,     0,     0,    10,    11,    12,     0,    13,    14,
       2,     3,     0,    15,     0,    17,     0,    18,     0,     0,
       4,    21,    22,     0,     0,    23,    24,    25,     6,     7,
     170,     0,     0,     0,     0,     0,     0,     8,     0,     0,
      10,    11,    12,     0,    13,    14,     0,     0,     0,    15,
       0,    17,     0,    18,     1,     2,     3,    21,    22,     0,
       0,    23,    24,    25,     0,     4,   156,     0,     0,     0,
       0,     5,     0,     6,     7,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     9,    10,    11,    12,     0,    13,
      14,     0,     0,     0,    15,    16,    17,     0,    18,    19,
      20,     0,    21,    22,     0,     0,    23,    24,    25
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-310)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       3,     4,     5,   312,   313,     6,     7,    10,     6,     7,
      26,    14,     0,   124,    17,   126,    19,    20,    21,    63,
      23,    24,    25,    89,    90,    91,    63,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    26,   105,
     106,   107,   108,     0,     3,     4,     5,    64,   357,    65,
      63,     8,     9,    10,     6,     7,    12,    13,    14,    15,
      16,    63,    19,    63,    65,     6,     7,    65,    25,    63,
      27,    28,    63,    76,    64,    65,    79,    63,    63,    36,
      63,    38,    39,    40,    41,    63,    43,    44,    63,    63,
      63,    48,    49,    50,    63,    52,    53,    54,    63,    56,
      57,    63,    65,    60,    61,    62,   415,    65,    69,    63,
     113,   114,    63,    65,    63,    63,    63,    63,    63,    63,
      63,    63,   126,     9,    10,    63,    63,    63,   116,    63,
      63,    63,   120,    19,    20,    21,    22,    23,    24,    63,
      63,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,   154,   155,    39,    40,    41,    63,    43,    44,    63,
      63,    63,    48,    63,    50,    63,    52,    63,    63,    55,
      56,    57,     9,    10,    60,    61,    62,    64,    63,    65,
      63,    63,    19,    20,    21,    22,    23,    24,    63,    63,
      27,    28,    63,    63,    63,    63,    63,    63,    63,    36,
      63,    63,    39,    40,    41,    63,    43,    44,    45,    46,
      47,    48,    63,    50,    63,    52,    63,    63,    63,    56,
      57,    58,    63,    60,    61,    62,     9,    10,    65,    63,
      65,    63,    63,    63,    63,    26,    19,    20,    21,    22,
      23,    24,    65,    65,    27,    28,    65,    65,    65,    65,
      65,    65,    65,    36,    65,    65,    39,    40,    41,    65,
      43,    44,    45,    46,    47,    48,    65,    50,    65,    52,
      65,    65,    65,    56,    57,    65,    65,    60,    61,    62,
      65,    65,    65,    65,    65,   118,    65,    65,    65,    65,
      65,   124,    65,   126,    65,   128,    65,   130,   364,   132,
      65,   134,    65,   136,    65,   138,    65,   140,   141,    65,
     143,   314,    65,   146,   317,   148,   319,   150,    65,   322,
      65,    65,   325,    65,    65,   328,    65,   330,    65,   332,
     333,   334,   335,   336,    65,   338,   138,   340,   341,    -1,
     104,    -1,   345,    -1,   102,    -1,    -1,    -1,    -1,    -1,
      -1,   354,   355,   356,   315,   316,    -1,   318,    -1,   320,
     321,    -1,   323,   324,    -1,   326,   327,    -1,   329,    -1,
     331,    -1,    -1,    -1,    -1,    -1,   337,    -1,   339,    -1,
      -1,   342,   343,   344,    -1,   346,   347,   348,   349,   350,
     351,   352,   353,    -1,    -1,    -1,    -1,    -1,    -1,   360,
      -1,    -1,    -1,   406,   365,   366,    -1,   368,    -1,   412,
      -1,    -1,   373,    -1,    -1,   376,    -1,    -1,   379,    -1,
     381,    -1,    -1,    -1,    -1,    -1,   387,    -1,   389,    -1,
      -1,   392,    -1,    -1,    -1,   396,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   408,    -1,    -1,
      -1,    -1,   455,    -1,    -1,    -1,   417,    -1,    -1,    -1,
      -1,   422,    -1,    -1,   425,    -1,    -1,   428,    -1,   430,
      -1,    -1,    -1,    -1,    -1,   436,    -1,   438,    -1,    -1,
     441,    -1,    -1,    -1,   445,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   456,    -1,    -1,    -1,    -1,
      -1,    -1,   463,   464,   465,   466,   467,   468,   469,   470,
     471,     9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,    22,    23,    24,    -1,    -1,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,
      48,    -1,    50,    -1,    52,    -1,    -1,    -1,    56,    57,
      58,    59,    60,    61,    62,     9,    10,    65,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    22,    23,
      24,    -1,    -1,    27,    28,    29,    30,    -1,    -1,    -1,
      -1,    -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,
      44,    -1,    -1,    -1,    48,    -1,    50,    -1,    52,    -1,
      -1,    -1,    56,    57,     9,    10,    60,    61,    62,    -1,
      -1,    65,    -1,    -1,    19,    20,    21,    22,    23,    24,
      -1,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,    44,
      -1,    -1,    -1,    48,    -1,    50,    -1,    52,    -1,    -1,
      -1,    56,    57,    58,    -1,    60,    61,    62,     9,    10,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    23,    24,    -1,    -1,    27,    28,    -1,    30,
      -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    39,    40,
      41,    -1,    43,    44,    -1,    -1,    -1,    48,    -1,    50,
      -1,    52,    -1,    -1,    -1,    56,    57,     9,    10,    60,
      61,    62,    -1,    -1,    65,    -1,    -1,    19,    20,    21,
      22,    23,    24,    -1,    -1,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    -1,    -1,    39,    40,    41,
      42,    43,    44,    -1,    -1,    -1,    48,    -1,    50,    -1,
      52,    -1,    -1,    -1,    56,    57,     9,    10,    60,    61,
      62,    -1,    -1,    65,    -1,    -1,    19,    20,    21,    22,
      23,    24,    -1,    -1,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    39,    40,    41,    -1,
      43,    44,    -1,    -1,    -1,    48,    -1,    50,    51,    52,
      -1,    -1,    -1,    56,    57,     9,    10,    60,    61,    62,
      -1,    -1,    65,    -1,    -1,    19,    20,    21,    22,    23,
      24,    -1,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,
      44,    -1,    -1,    -1,    48,    -1,    50,    -1,    52,    -1,
      -1,    -1,    56,    57,     9,    10,    60,    61,    62,    -1,
      -1,    65,    -1,    -1,    19,    20,    21,    22,    23,    24,
      -1,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,    44,
      -1,    -1,    -1,    48,    -1,    50,    -1,    52,    -1,    -1,
      -1,    56,    57,     9,    10,    60,    61,    62,    -1,    -1,
      65,    -1,    -1,    19,    20,    21,    22,    23,    24,    -1,
      -1,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    39,    40,    41,    -1,    43,    44,    -1,
      -1,    -1,    48,    -1,    50,    -1,    52,    -1,    -1,    -1,
      56,    57,     9,    10,    60,    61,    62,    -1,    -1,    65,
      -1,    -1,    19,    20,    21,    22,    23,    24,    -1,    -1,
      27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    39,    40,    41,    -1,    43,    44,    -1,    -1,
      -1,    48,    -1,    50,    -1,    52,    -1,    -1,    -1,    56,
      57,     9,    10,    60,    61,    62,    -1,    -1,    65,    -1,
      -1,    19,    20,    21,    22,    23,    24,    -1,    -1,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,
      48,    -1,    50,    -1,    52,    -1,    -1,    -1,    56,    57,
       9,    10,    60,    61,    62,    -1,    -1,    65,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,
      39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,    48,
      -1,    50,    -1,    52,    -1,    -1,    -1,    56,    57,     9,
      10,    60,    61,    62,    -1,    -1,    65,    -1,    -1,    19,
      20,    21,    22,    -1,    -1,    -1,    -1,    27,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,
      40,    41,    -1,    43,    44,    -1,    -1,    -1,    48,    -1,
      50,    -1,    52,    -1,    -1,    -1,    56,    57,     9,    10,
      60,    61,    62,    -1,    -1,    65,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    -1,    27,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    39,    40,
      41,    -1,    43,    44,     9,    10,    11,    48,    -1,    50,
      -1,    52,    17,    18,    19,    56,    57,    -1,    -1,    60,
      61,    62,    27,    28,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,    39,    40,    41,    -1,    43,    44,
       9,    10,    -1,    48,    -1,    50,    -1,    52,    -1,    -1,
      19,    56,    57,    -1,    -1,    60,    61,    62,    27,    28,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,
      39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,    48,
      -1,    50,    -1,    52,     8,     9,    10,    56,    57,    -1,
      -1,    60,    61,    62,    -1,    19,    65,    -1,    -1,    -1,
      -1,    25,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    -1,    38,    39,    40,    41,    -1,    43,
      44,    -1,    -1,    -1,    48,    49,    50,    -1,    52,    53,
      54,    -1,    56,    57,    -1,    -1,    60,    61,    62
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,     9,    10,    19,    25,    27,    28,    36,    38,
      39,    40,    41,    43,    44,    48,    49,    50,    52,    53,
      54,    56,    57,    60,    61,    62,    67,    68,    71,    74,
      75,    76,    77,    78,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   187,   188,   191,   192,    63,
       6,     7,    73,    73,    73,    73,    63,    73,    73,    63,
      73,    73,    73,    73,    73,    73,    73,     0,    68,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,     3,
       4,     5,    69,    73,    73,    81,   115,    81,   177,    81,
     116,    81,   172,    81,   173,    81,   174,    81,   179,    81,
     180,    81,   181,    81,   182,    81,   183,    81,   184,    81,
     185,   185,    81,   186,   172,    81,   178,    81,   175,    81,
     176,    81,   189,    64,    73,    73,    65,    82,    20,    21,
      22,    65,    82,   117,   118,   119,   121,    11,    17,    18,
      65,    80,    82,   113,   114,    23,    24,    65,   121,   122,
     156,   165,    58,    65,   121,   122,   123,   125,    59,    65,
     121,   122,   123,   124,    29,    65,   121,   129,   147,    23,
      24,    30,    65,   121,   130,   150,   163,   170,    23,    24,
      42,    65,   121,   131,   144,   164,   171,    23,    24,    65,
     121,   132,   161,   169,    23,    24,    45,    46,    47,    65,
     121,   133,   139,   140,   141,   142,   157,    58,    65,   121,
     133,   134,   143,    23,    24,    65,   121,   135,   159,   167,
      65,    23,    24,    51,    65,   121,   136,   137,   138,   162,
      65,    23,    24,    29,    30,    65,   121,   128,   146,   149,
     160,   168,    23,    24,    29,    30,    31,    32,    33,    34,
      35,    55,    65,   120,   121,   126,   145,   148,   151,   152,
     153,   154,   155,   158,   166,    37,    65,   121,   127,    26,
      65,   190,    12,    13,    14,    15,    16,    72,    73,    65,
      73,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    64,    65,    65,
      69,    70,    70,    70,    73,    69,    69,    73,    69,    73,
      69,    69,    73,    69,    69,    73,    69,    69,    73,    69,
      73,    69,    73,    73,    73,    73,    73,    69,    73,    69,
      73,    73,    69,    69,    69,    73,    69,    69,    69,    69,
      69,    69,    69,    69,    73,    73,    73,    70,    69,    65,
      65,    65,    79,    81,    69,    69,    65,    69,    65,    65,
      65,    65,    69,    65,    65,    69,    65,    65,    69,    65,
      69,    65,    65,    65,    65,    65,    69,    65,    69,    65,
      65,    69,    65,    65,    65,    69,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    73,    64,    65,    69,    65,
      73,    65,    70,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    65,    73,    69,    65,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 9:
/* Line 1792 of yacc.c  */
#line 137 "dna.ypp"
    {
  (yyval.u.real) = (double)(yyvsp[(1) - (1)].u.uint64);
}
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 141 "dna.ypp"
    {
  (yyval.u.real) = (double)(yyvsp[(1) - (1)].u.int64);
}
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 145 "dna.ypp"
    {
  (yyval.u.real) = (yyvsp[(1) - (1)].u.real);
}
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 150 "dna.ypp"
    {
}
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 154 "dna.ypp"
    {
}
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 157 "dna.ypp"
    {
}
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 161 "dna.ypp"
    {
  #undef DNA_STREET_POINT
}
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 165 "dna.ypp"
    {
  #undef DNA_FRONT_DOOR_POINT
}
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 169 "dna.ypp"
    {
  #undef DNA_SIDE_DOOR_POINT
}
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 173 "dna.ypp"
    {
  #undef DNA_COGHQ_IN_POINT
}
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 177 "dna.ypp"
    {
  #undef DNA_COGHQ_OUT_POINT
}
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 182 "dna.ypp"
    {
  (yyval.str) = (yyvsp[(1) - (1)].str);
}
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 186 "dna.ypp"
    {
  (yyval.str) = (yyvsp[(1) - (1)].str);
}
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 191 "dna.ypp"
    {
  DNAGroup* g = new DNAGroup((yyvsp[(2) - (2)].str));
  g->set_parent(_cur_comp);
  g->set_vis_group(_cur_comp->get_vis_group());
  _cur_comp->add(g);
  _cur_comp = g;
}
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 200 "dna.ypp"
    {
}
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 204 "dna.ypp"
    {
  DNAGroup* g = new DNAVisGroup((yyvsp[(2) - (2)].str));
  g->set_parent(_cur_comp);
  _cur_comp->add(g);
  _cur_comp = g;
  _store->store_DNA_vis_group((DNAVisGroup*)g);
}
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 213 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 26:
/* Line 1792 of yacc.c  */
#line 218 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 223 "dna.ypp"
    {
  (yyvsp[(1) - (2)].str_vec).push_back((yyvsp[(2) - (2)].str));
  (yyval.str_vec) = (yyvsp[(1) - (2)].str_vec);
}
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 228 "dna.ypp"
    {
  (yyval.str_vec) = {};
}
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 233 "dna.ypp"
    {
  DNAVisGroup* g = (DNAVisGroup*)_cur_comp;
  g->add_visible((yyvsp[(3) - (5)].str));
  for (auto& vis : (yyvsp[(4) - (5)].str_vec))
    g->add_visible(vis);
}
    break;

  case 47:
/* Line 1792 of yacc.c  */
#line 264 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 48:
/* Line 1792 of yacc.c  */
#line 269 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 49:
/* Line 1792 of yacc.c  */
#line 274 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 50:
/* Line 1792 of yacc.c  */
#line 279 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 51:
/* Line 1792 of yacc.c  */
#line 283 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 52:
/* Line 1792 of yacc.c  */
#line 287 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 53:
/* Line 1792 of yacc.c  */
#line 292 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 55:
/* Line 1792 of yacc.c  */
#line 300 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 56:
/* Line 1792 of yacc.c  */
#line 305 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 57:
/* Line 1792 of yacc.c  */
#line 310 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 58:
/* Line 1792 of yacc.c  */
#line 315 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 59:
/* Line 1792 of yacc.c  */
#line 320 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 60:
/* Line 1792 of yacc.c  */
#line 324 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 61:
/* Line 1792 of yacc.c  */
#line 329 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 62:
/* Line 1792 of yacc.c  */
#line 334 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 63:
/* Line 1792 of yacc.c  */
#line 338 "dna.ypp"
    {
  _cur_comp = _cur_comp->get_parent();
}
    break;

  case 64:
/* Line 1792 of yacc.c  */
#line 343 "dna.ypp"
    {
}
    break;

  case 65:
/* Line 1792 of yacc.c  */
#line 347 "dna.ypp"
    {
}
    break;

  case 66:
/* Line 1792 of yacc.c  */
#line 351 "dna.ypp"
    {
}
    break;

  case 67:
/* Line 1792 of yacc.c  */
#line 355 "dna.ypp"
    {
}
    break;

  case 68:
/* Line 1792 of yacc.c  */
#line 359 "dna.ypp"
    {
}
    break;

  case 69:
/* Line 1792 of yacc.c  */
#line 363 "dna.ypp"
    {
}
    break;

  case 70:
/* Line 1792 of yacc.c  */
#line 367 "dna.ypp"
    {
}
    break;

  case 71:
/* Line 1792 of yacc.c  */
#line 371 "dna.ypp"
    {
}
    break;

  case 72:
/* Line 1792 of yacc.c  */
#line 375 "dna.ypp"
    {
}
    break;

  case 73:
/* Line 1792 of yacc.c  */
#line 379 "dna.ypp"
    {
}
    break;

  case 74:
/* Line 1792 of yacc.c  */
#line 383 "dna.ypp"
    {
}
    break;

  case 75:
/* Line 1792 of yacc.c  */
#line 387 "dna.ypp"
    {
}
    break;

  case 76:
/* Line 1792 of yacc.c  */
#line 391 "dna.ypp"
    {
}
    break;

  case 77:
/* Line 1792 of yacc.c  */
#line 395 "dna.ypp"
    {
}
    break;

  case 78:
/* Line 1792 of yacc.c  */
#line 399 "dna.ypp"
    {
}
    break;

  case 79:
/* Line 1792 of yacc.c  */
#line 403 "dna.ypp"
    {
}
    break;

  case 80:
/* Line 1792 of yacc.c  */
#line 407 "dna.ypp"
    {
}
    break;

  case 81:
/* Line 1792 of yacc.c  */
#line 411 "dna.ypp"
    {
}
    break;

  case 89:
/* Line 1792 of yacc.c  */
#line 426 "dna.ypp"
    {
}
    break;

  case 90:
/* Line 1792 of yacc.c  */
#line 430 "dna.ypp"
    {
}
    break;

  case 91:
/* Line 1792 of yacc.c  */
#line 434 "dna.ypp"
    {
}
    break;

  case 92:
/* Line 1792 of yacc.c  */
#line 438 "dna.ypp"
    {
}
    break;

  case 100:
/* Line 1792 of yacc.c  */
#line 455 "dna.ypp"
    {
}
    break;

  case 101:
/* Line 1792 of yacc.c  */
#line 459 "dna.ypp"
    {
}
    break;

  case 112:
/* Line 1792 of yacc.c  */
#line 475 "dna.ypp"
    {
}
    break;

  case 137:
/* Line 1792 of yacc.c  */
#line 521 "dna.ypp"
    {
}
    break;

  case 138:
/* Line 1792 of yacc.c  */
#line 525 "dna.ypp"
    {
}
    break;

  case 139:
/* Line 1792 of yacc.c  */
#line 529 "dna.ypp"
    {
}
    break;

  case 140:
/* Line 1792 of yacc.c  */
#line 533 "dna.ypp"
    {
}
    break;

  case 141:
/* Line 1792 of yacc.c  */
#line 537 "dna.ypp"
    {
}
    break;

  case 142:
/* Line 1792 of yacc.c  */
#line 541 "dna.ypp"
    {
}
    break;

  case 143:
/* Line 1792 of yacc.c  */
#line 545 "dna.ypp"
    {
}
    break;

  case 144:
/* Line 1792 of yacc.c  */
#line 549 "dna.ypp"
    {
}
    break;

  case 145:
/* Line 1792 of yacc.c  */
#line 553 "dna.ypp"
    {
}
    break;

  case 146:
/* Line 1792 of yacc.c  */
#line 557 "dna.ypp"
    {
}
    break;

  case 147:
/* Line 1792 of yacc.c  */
#line 561 "dna.ypp"
    {
}
    break;

  case 148:
/* Line 1792 of yacc.c  */
#line 565 "dna.ypp"
    {
}
    break;

  case 149:
/* Line 1792 of yacc.c  */
#line 569 "dna.ypp"
    {
}
    break;

  case 150:
/* Line 1792 of yacc.c  */
#line 573 "dna.ypp"
    {
}
    break;

  case 151:
/* Line 1792 of yacc.c  */
#line 577 "dna.ypp"
    {
}
    break;

  case 152:
/* Line 1792 of yacc.c  */
#line 581 "dna.ypp"
    {
}
    break;

  case 153:
/* Line 1792 of yacc.c  */
#line 585 "dna.ypp"
    {
}
    break;

  case 154:
/* Line 1792 of yacc.c  */
#line 589 "dna.ypp"
    {
}
    break;

  case 155:
/* Line 1792 of yacc.c  */
#line 593 "dna.ypp"
    {
}
    break;

  case 156:
/* Line 1792 of yacc.c  */
#line 597 "dna.ypp"
    {
}
    break;

  case 157:
/* Line 1792 of yacc.c  */
#line 601 "dna.ypp"
    {
}
    break;

  case 158:
/* Line 1792 of yacc.c  */
#line 605 "dna.ypp"
    {
}
    break;

  case 159:
/* Line 1792 of yacc.c  */
#line 609 "dna.ypp"
    {
}
    break;

  case 160:
/* Line 1792 of yacc.c  */
#line 613 "dna.ypp"
    {
}
    break;

  case 161:
/* Line 1792 of yacc.c  */
#line 617 "dna.ypp"
    {
}
    break;

  case 162:
/* Line 1792 of yacc.c  */
#line 621 "dna.ypp"
    {
}
    break;

  case 163:
/* Line 1792 of yacc.c  */
#line 625 "dna.ypp"
    {
}
    break;

  case 164:
/* Line 1792 of yacc.c  */
#line 629 "dna.ypp"
    {
}
    break;

  case 165:
/* Line 1792 of yacc.c  */
#line 633 "dna.ypp"
    {
}
    break;

  case 166:
/* Line 1792 of yacc.c  */
#line 637 "dna.ypp"
    {
}
    break;

  case 167:
/* Line 1792 of yacc.c  */
#line 641 "dna.ypp"
    {
}
    break;

  case 168:
/* Line 1792 of yacc.c  */
#line 645 "dna.ypp"
    {
}
    break;

  case 169:
/* Line 1792 of yacc.c  */
#line 649 "dna.ypp"
    {
}
    break;

  case 170:
/* Line 1792 of yacc.c  */
#line 653 "dna.ypp"
    {
}
    break;

  case 171:
/* Line 1792 of yacc.c  */
#line 657 "dna.ypp"
    {
}
    break;

  case 220:
/* Line 1792 of yacc.c  */
#line 739 "dna.ypp"
    {
}
    break;

  case 221:
/* Line 1792 of yacc.c  */
#line 742 "dna.ypp"
    {
}
    break;

  case 222:
/* Line 1792 of yacc.c  */
#line 745 "dna.ypp"
    {
}
    break;

  case 226:
/* Line 1792 of yacc.c  */
#line 756 "dna.ypp"
    {
}
    break;

  case 227:
/* Line 1792 of yacc.c  */
#line 759 "dna.ypp"
    {
}
    break;

  case 228:
/* Line 1792 of yacc.c  */
#line 763 "dna.ypp"
    {
}
    break;

  case 229:
/* Line 1792 of yacc.c  */
#line 766 "dna.ypp"
    {
}
    break;

  case 230:
/* Line 1792 of yacc.c  */
#line 770 "dna.ypp"
    {
}
    break;


/* Line 1792 of yacc.c  */
#line 2928 "dna_ypp.cpp"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


