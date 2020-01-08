/*
 * Parser interpretera maszyny wirtualnej do projektu z JFTT2019
 *
 * Autor: Maciek Gębala
 * http://ki.pwr.edu.pl/gebala/
 * 2019-11-12
*/
%{
#define YYSTYPE long long

#include <iostream>
#include <utility>
#include <vector>

#include "instructions.hh"

extern int yylineno;
int yylex( void );
void yyset_in( FILE * in_str );
void yyerror( std::vector< std::pair<int,long long> > & program, char const *s );
%}
%parse-param { std::vector< std::pair<int,long long> > & program }
%token COM_0
%token COM_1
%token JUMP_1
%token STOP
%token LABEL
%token ERROR
%%
input :
    input line
  | %empty
  ;

line :
    COM_0 		{ program.push_back( std::make_pair($1,0) ); }
  | COM_1 LABEL 	{ program.push_back( std::make_pair($1,$2) ); }
  | JUMP_1 LABEL 	{ program.push_back( std::make_pair($1,$2) ); }
  | STOP 		{ program.push_back( std::make_pair($1,0) ); }
  | ERROR 		{ yyerror( program, "Nierozpoznany symbol" ); }
  ;
%%
void yyerror( std::vector< std::pair<int,long long> > & program, char const *s )
{
  std::cerr << "Linia " << yylineno << ": " << s << std::endl;
  exit(-1);
}

void run_parser( std::vector< std::pair<int,long long> > & program, FILE * data ) 
{
  std::cout << "Czytanie kodu." << std::endl;
  yyset_in( data );
  yyparse( program );
  std::cout << "Skończono czytanie kodu (liczba rozkazów: " << program.size() << ")." << std::endl;
}
