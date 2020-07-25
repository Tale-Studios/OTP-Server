#pragma once
#include "core/global.h"

void dna_init_lexer(std::istream &in, const std::string &filename);
int dna_error_count();
int dna_warning_count();

void dnayyerror(const std::string &msg);
void dnayywarning(const std::string &msg);

int dnayylex();

// we always read files
#define YY_NEVER_INTERACTIVE 1
