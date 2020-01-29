%{
    #include <iostream>
    #include <cstdio>
    #include <string>
    #include <vector>
    #include "ast/node.h"

    extern int yylex();
    extern int yyparse();
    extern FILE *yyin;
    extern int yylineno;
    extern char* yytext;

    void yyerror(const char *s);

    DeclarationList *declarations = nullptr;
    CommandList *commands = nullptr;
    ConstantList *constants = new ConstantList();
%}

%code requires {
    #include "ast/node.h"
}

%union {
    int token;

    Command *cmd;
    AbstractIdentifier *ident;
    AbstractValue *value;
    AbstractExpression *exp;
    Condition *cond;
    AbstractDeclaration *decl;
    DeclarationList *decl_list;
    CommandList *cmd_list;

    long long numberValue;
    std::string *stringValue;
}

%token <token> DECLARE T_BEGIN END ASSIGN IF THEN ELSE ENDIF
%token <token> WHILE DO ENDDO ENDWHILE FOR FROM TO DOWNTO ENDFOR
%token <token> READ WRITE
%token <token> PLUS MINUS TIMES DIV MOD
%token <token> EQ NEQ LE GE LEQ GEQ
%token <token> RBRACKET LBRACKET COLON SEMICOLON COMMA
%token <token> ERROR

%token <numberValue> NUMBER
%token <stringValue> PIDENTIFIER

%type <cmd> command;
%type <ident> identifier;
%type <exp> expression;
%type <cond> condition;
%type <decl_list> declarations;
%type <cmd_list> commands;
%type <value> value;

%%
program:
    DECLARE declarations T_BEGIN commands END {
        declarations = $2;
        commands = $4;
    }
    | T_BEGIN commands END {
        commands = $2;
    }
;

declarations:
    declarations COMMA PIDENTIFIER {
        $$->declarations.push_back(new IdentifierDeclaration(*$3));
    }
    | declarations COMMA PIDENTIFIER LBRACKET NUMBER COLON NUMBER RBRACKET {
        constants->constants.push_back($5);
        $$->declarations.push_back(new ArrayDeclaration(*$3, $5, $7));
    }
    | PIDENTIFIER {
        $$ = new DeclarationList();
        $$->declarations.push_back(new IdentifierDeclaration(*$1));
    }
    | PIDENTIFIER LBRACKET NUMBER COLON NUMBER RBRACKET {
        $$ = new DeclarationList();
        constants->constants.push_back($3);
        $$->declarations.push_back(new ArrayDeclaration(*$1, $3, $5));
    }
;

commands:
    commands command {
        $1->commands.push_back($2);
    }
    | command {
        $$ = new CommandList();
        $$->commands.push_back($1);
    }
;

command:
    identifier ASSIGN expression SEMICOLON {
        $$ = new Assignment(*$1, *$3);
    }
    | IF condition THEN commands ELSE commands ENDIF {
        $$ = new IfElse(*$2, *$4, *$6);
    }
    | IF condition THEN commands ENDIF {
        $$ = new If(*$2, *$4);
    }
    | WHILE condition DO commands ENDWHILE {
        $$ = new While(*$2, *$4);
    }
    | DO commands WHILE condition ENDDO {
        $$ = new While(*$4, *$2, true);
    }
    | FOR PIDENTIFIER FROM value TO value DO commands ENDFOR {
        $$ = new For(*$2, *$4, *$6, *$8);
    }
    | FOR PIDENTIFIER FROM value DOWNTO value DO commands ENDFOR {
        $$ = new For(*$2, *$4, *$6, *$8, true);
    }
    | READ identifier SEMICOLON {
        $$ = new Read(*$2);
    }
    | WRITE value SEMICOLON {
        $$ = new Write(*$2);
    }
;

expression:
    value {
        $$ = new UnaryExpression(*$1);
    }
    | value PLUS value {
        $$ = new BinaryExpression(*$1, *$3, ADDITION);
    }
    | value MINUS value {
        $$ = new BinaryExpression(*$1, *$3, SUBTRACTION);
    }
    | value TIMES value {
        $$ = new BinaryExpression(*$1, *$3, MULTIPLICATION);
    }
    | value DIV value {
        $$ = new BinaryExpression(*$1, *$3, DIVISION);
    }
    | value MOD value {
        $$ = new BinaryExpression(*$1, *$3, MODULO);
    }
;

condition:
    value EQ value {
        $$ = new Condition(*$1, *$3, EQUAL);
    }
    | value NEQ value {
        $$ = new Condition(*$1, *$3, NOT_EQUAL);
    }
    | value LE value {
        $$ = new Condition(*$1, *$3, LESS);
    }
    | value GE value {
        $$ = new Condition(*$1, *$3, GREATER);
    }
    | value LEQ value {
        $$ = new Condition(*$1, *$3, LESS_OR_EQUAL);
    }
    | value GEQ value {
        $$ = new Condition(*$1, *$3, GREATER_OR_EQUAL);
    }
;

value:
    NUMBER {
        constants->constants.push_back($1);
        $$ = new NumberValue($1);
    } | identifier {
        $$ = new IdentifierValue(*$1);
    }
;

identifier:
    PIDENTIFIER {
        $$ = new VariableIdentifier(*$1);
    }
    | PIDENTIFIER LBRACKET PIDENTIFIER RBRACKET {
        $$ = new VariableAccessIdentifier(*$1, *$3);
    }
    | PIDENTIFIER LBRACKET NUMBER RBRACKET {
        constants->constants.push_back($3);
        $$ = new AccessIdentifier(*$1, $3);
    }
;
%%



void yyerror(const char *s) {
  std::cout << "   [e] " << s << " at line " << yylineno << std::endl;
  std::cout << "[e] Aborting" << std::endl;
  exit(1);
}