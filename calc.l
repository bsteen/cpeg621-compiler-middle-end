%{
// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <stdlib.h> // for atoi call
#include <string.h>
#include "calc.tab.h"

// #define DEBUG 			// for debugging: print tokens and their line numbers

extern int flex_line_num; 	// line number from cal.tab.c
extern FILE * yyin;		// Defined in calc.y as the input file
void yyerror(char *);	// Defined in calc.y

// Options below exclude unused input and yyunput functions
%}

%option noinput	
%option nounput 	

%%

[A-Za-z][A-Za-z0-9]* {	// Variable names are case insensitive
	#ifdef DEBUG
	printf("token %s at line %d\n", yytext, flex_line_num);
	#endif

	yylval.str = strdup(yytext); // Must be freed in bison grammar rules
	return VARIABLE;
	}

[0-9]+ {
	#ifdef DEBUG
	printf("token %s at line %d\n", yytext, flex_line_num);
	#endif

	yylval.str = strdup(yytext); // Must be freed in bison grammar rules
	return INTEGER;
	}

"**" {
	#ifdef DEBUG
	printf("token %s at line %d\n", yytext, flex_line_num);
	#endif

	return POWER;
	}

[-+()=*/!?]	{
		#ifdef DEBUG
		printf("token %s at line %d\n", yytext, flex_line_num);
		#endif

		return *yytext;		// Return character literal as the token value
	}

"\n" {
	#ifdef DEBUG
	printf("token \\n at line %d\n", flex_line_num);
	#endif

	flex_line_num++;
	return *yytext;
	}

[ \t]+ {
	#ifdef DEBUG
	printf("token is whitespace(s) at line %d\n", flex_line_num);
	#endif
	// Ignore whitespaces (return nothing)
	}

.	{ yyerror("invalid character"); }

%%

int yywrap() { /* need this to avoid link problem */
	return 1;
}
