%{
#include "y.tab.h"
int lineno = 1;
int error = 0;
char linebuf[500];
%}
digit [0-9]
alpha [a-zA-Z]
alphanum [a-zA-Z0-9]
%%
[ \t]+                                             ;
\n.*                                                 {strcpy(linebuf, yytext+1);
                                                    lineno++;
                                                    yyless(1);}
"--"[^\n]*                                         ;
">="                                               {return GTE;}
"<="                                               {return LTE;}
"end if"                                           {return ENDIF;}
"end record"                                       {return ENDREC;}
"end loop"                                         {return ENDLOOP;}
"exit when"                                        {return EXITWHEN;}
"is"                                               {return IS;}
"of"                                               {return OF;}
"begin"                                            {return BEG;}
"end"                                              {return END;}
".."                                               {return DOTDOT;}
"'"                                                {return TICK;}
"procedure"                                        {return PROCEDURE;}
"type"                                             {return TYPE;}
"array"                                            {return ARRAY;}
"record"                                           {return RECORD;}
"in"                                               {return IN;}
"out"                                              {return OUT;}
"range"                                            {return RANGE;}
"constant"                                         {return CONSTANT;}
":="                                               {return ASSIGN;}
"exception"                                        {return EXCEPTION;}
"null"                                             {return NULLWORD;}
"loop"                                             {return LOOP;}
"if"                                               {return IF;}
"then"                                             {return THEN;}
"elsif"                                            {return ELSEIF;}
"else"                                             {return ELSE;}
"exit"                                             {return EXIT;}
"when"                                             {return WHEN;}
"and"                                              {return AND;}
"or"                                               {return OR;}
"not"                                              {return NOT;}
"="                                                {return EQ;}
"/="                                               {return NEQ;}
">"                                                {return GT;}
"<"                                                {return LT;}
"**"                                               {return EXP;}
"raise"                                            {return RAISE;}
"others"                                           {return OTHERS;}
"=>"                                               {return ARROW;}
{alpha}(_?{alphanum})*                             {
                                                    int i;
                                                    for (i = 0; 
                                                    i < sizeof(yytext); i++)
                                                    {yytext[i] = 
                                                       tolower(yytext[i]);}
                                                    yylval.var 
                                                       = malloc(sizeof(yytext)+1);
                                                    strcpy(yylval.var, yytext); 
                                                    return ID;
                                                   }
{digit}+                                           {
                                                    yylval.integer 
                                                        = atoi(yytext);
                                                    return NUMBER;
                                                   }
.                                                  {return yytext[0];}
%%

yyerror(char *s) 
{
    printf("%d: %s at '%s' in this line:\n%s\n", lineno, s, yytext, linebuf);
    error = 1;
}
