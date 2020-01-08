%option noyywrap
%option yylineno

%{
    #include <string>
    #include "compiler.tab.h"
    #include "ast/node.h"

    #define TOKEN(t) (yylval.token = t)
%}

pidentifier [_a-z]+
number -?([0-9]|[1-9][0-9]*)
comment \[(.|\n)*\]

%%
{comment}       ;
\n              ;
\r              ;
[[:blank:]]+    ;
{pidentifier}   { yylval.stringValue = new std::string(yytext, yyleng); return PIDENTIFIER; }
{number}        { yylval.numberValue = atoi(yytext); return NUMBER; }
DECLARE         { return TOKEN(DECLARE); }
BEGIN           { return TOKEN(T_BEGIN); }
END             { return TOKEN(END); }
ASSIGN          { return TOKEN(ASSIGN); }
IF              { return TOKEN(IF); }
THEN            { return TOKEN(THEN); }
ELSE            { return TOKEN(ELSE); }
ENDIF           { return TOKEN(ENDIF); }
WHILE           { return TOKEN(WHILE); }
DO              { return TOKEN(DO); }
ENDDO           { return TOKEN(ENDDO); }
ENDWHILE        { return TOKEN(ENDWHILE); }
FOR             { return TOKEN(FOR); }
FROM            { return TOKEN(FROM); }
TO              { return TOKEN(TO); }
DOWNTO          { return TOKEN(DOWNTO); }
ENDFOR          { return TOKEN(ENDFOR); }
READ            { return TOKEN(READ); }
WRITE           { return TOKEN(WRITE); }
PLUS            { return TOKEN(PLUS); }
MINUS           { return TOKEN(MINUS); }
TIMES           { return TOKEN(TIMES); }
DIV             { return TOKEN(DIV); }
MOD             { return TOKEN(MOD); }
EQ              { return TOKEN(EQ); }
NEQ             { return TOKEN(NEQ); }
LE              { return TOKEN(LE); }
GE              { return TOKEN(GE); }
LEQ             { return TOKEN(LEQ); }
GEQ             { return TOKEN(GEQ); }
\(              { return TOKEN(LBRACKET); }
\)              { return TOKEN(RBRACKET); }
\:              { return TOKEN(COLON); }
\;              { return TOKEN(SEMICOLON); }
\,              { return TOKEN(COMMA); }
.               { return TOKEN(ERROR); }
%%