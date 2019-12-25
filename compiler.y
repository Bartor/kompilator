%{
    #include <iostream>
    #include <cstdio>
    #include <string>

    extern int yylex();
    extern int yyparse();
    extern FILE *yyin;

    void yyerror(const char *s);
%}

%code requires {
    #include "ast/node.h"
}

%union {
    int token;

    Command *cmd;
    Identifier *ident;
    Expression *exp;

    int numberValue;
    std::string *stringValue;
}

%token <token> DECLARE T_BEGIN END ASSIGN IF THEN ELSE ENDIF
%token <token> WHILE DO ENDDO ENDWHILE FOR FROM TO DOWNTO ENDFOR
%token <token> READ WRITE
%token <token> PLUS MINUS TIMES DIV MOD
%token <token> EQ NEQ LE GE LEQ GEQ
%token <token> RBRACKET LBRACKET COLON SEMICOLON COMMA
%token <token> ERROR

%type <cmd> command
%type <ident> identifier;
%type <numberValue> value;
%type <exp> expression;

%token <numberValue> NUMBER
%token <stringValue> PIDENTIFIER

%%
program:
    DECLARE declarations T_BEGIN commands END
    | T_BEGIN commands END
;

declarations:
    declarations COMMA PIDENTIFIER
    | declarations COMMA PIDENTIFIER LBRACKET NUMBER COLON NUMBER RBRACKET
    | PIDENTIFIER
    | PIDENTIFIER LBRACKET NUMBER COLON NUMBER RBRACKET
;

commands:
    commands command
    | command
;

command:
    identifier ASSIGN expression SEMICOLON {
        std::cout << "new assign" << std::endl;
        $$ = new Assignment(*$1, *$3);
    }
    | IF condition THEN commands ELSE commands ENDIF
    | IF condition THEN commands ENDIF
    | WHILE condition DO commands ENDWHILE
    | DO commands WHILE condition ENDDO
    | FOR PIDENTIFIER FROM value TO value DO commands ENDFOR
    | FOR PIDENTIFIER FROM value DOWNTO value DO commands ENDFOR
    | READ identifier COLON
    | WRITE value COLON
;

expression:
    value {
        std::cout << "new value" << std::endl;
        $$ = new ValueExpression(*new Value($1));
    }
    | value PLUS value
    | value MINUS value
    | value TIMES value
    | value DIV value
    | value MOD value
;

condition:
    value EQ value
    | value NEQ value
    | value LE value
    | value GE value
    | value LEQ value
    | value GEQ value
;

value:
    NUMBER {
        $$ = $1;
    } | identifier
;

identifier:
    PIDENTIFIER {
        $$ = new Identifier(*$1);
    }
    | PIDENTIFIER LBRACKET PIDENTIFIER RBRACKET
    | PIDENTIFIER LBRACKET NUMBER RBRACKET
;
%%

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "No source file specified" << std::endl;
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) {
        std::cerr << "Can't open " << argv[1] << std::endl;
        return 1;
    }

    yyin = source;
    yyparse();
}

void yyerror(const char *s) {
  std::cout << "Parsing error: " << s << std::endl;
  exit(1);
}