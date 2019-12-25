%option noyywrap
%option yylineno
%option c++

%{
    #include <string>
    #include "compiler.tab.hpp"
    int yylex();
%}

pidentifier [_a-z]+
number -?([0-9]|[1-9][0-9]*)
comment \[.*\]

%%
\n              ;
[[:blank:]]+    ;
{comment}       ;
DECLARE         { return DECLARE; }
BEGIN           { return BEGIN; }
END             { return END; }
ASSIGN          { return ASSIGN; }
IF              { return IF; }
THEN            { return THEN; }
ELSE            { return ELSE; }
ENDIF           { return ENDIF; }
WHILE           { return WHILE; }
DO              { return DO; }
ENDDO           { return ENDDO; }
ENDWHILE        { return ENDWHILE; }
FOR             { return FOR; }
FROM            { return FROM; }
TO              { return TO; }
DOWNTO          { return DOWNTO; }
ENDFOR          { return ENDFOR; }
READ            { return READ; }
WRITE           { return WRITE; }
PLUS            { return PLUS; }
MINUS           { return MINUS; }
TIMES           { return TIMES; }
DIV             { return DIV; }
MOD             { return MOD; }
EQ              { return EQ; }
NEQ             { return NEQ; }
LE              { return LE; }
GE              { return GE; }
LEQ             { return LEQ; }
GEQ             { return GEQ; }
{pidentifier}   { yylval.stringVal = new std::string(yytext); return PIDENTIFIER; }
{number}        { yylval.numVal = atoi(yytext); return NUMBER; }
\(              { return LBRACKET; }
\)              { return RBRACKET; }
\:              { return COLON; }
\;              { return SEMICOLON; }
\,              { return COMMA; }
.               { return ERROR; }
%%