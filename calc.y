%{
// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic-block.h"

#define MAX_USR_NUM_VARS 128
#define MAX_USR_VAR_NAME_LEN 64

int yylex(void);					// Will be generated in lex.yy.c by flex

// Following are defined below in sub-routines section
void my_free(char * ptr);
char* lc(char *str);
void gen_tac_assign(char * var, char * expr);
char* gen_tac_expr(char * one, char * op, char * three);
void gen_tac_if(char * cond_expr);
void gen_tac_assign_else(char * expr);
void gen_tac_empty_else();
void track_user_var(char * var);
void yyerror(const char *);

int do_gen_else = 0;				// When set do the else part of the if/else statement
int num_temp_vars = 0;				// Number of temp vars in use
int num_user_vars = 0;				// Number of user variables in use
int num_user_vars_wo_def = 0;		// Number of user variables that didn't have declarations
char user_vars[MAX_USR_NUM_VARS][MAX_USR_VAR_NAME_LEN + 1];			// List of all unique user vars

int flex_line_num = 1;		// Used for debugging
FILE * yyin;				// Input calc program file pointer
FILE * tac_file;			// Three address code file pointer
FILE * c_code_file;			// C code produced by backend file pointer
%}

%define parse.error verbose		// Enable verbose errors
%token INTEGER POWER VARIABLE	// bison adds these #defines in calc.tab.h for use in flex
								// Tells flex what the tokens are

// Union defines all possible values a token can have associated with it
// Allow yylval to hold either an integer or a string
%union
{
	int dval;
	char * str;
}

// When %union is used to specify multiple value types, must declare the
// value type of each token for which values are used
// In this case, all values associated with tokens are to be strings
%type <str> INTEGER POWER VARIABLE

// Conditional expressions and expressions values are also string type
%type <str> expr

// Make grammar unambiguous
// Low to high precedence and associativity within a precedent rank
// https://en.cppreference.com/w/c/language/operator_precedence
%right '='
%right '?'
%left '+' '-'
%left '*' '/'
%precedence '!'		// Unary bitwise not; No associativity b/c it is unary
%right POWER		// ** exponent operator

%start calc

%%

calc :
	calc expr '\n'		{ my_free($2); gen_tac_empty_else(); }
	|
	;

expr :
	INTEGER				{ $$ = $1; }
	| VARIABLE        	{ $$ = lc($1); track_user_var(lc($1)); }
	| VARIABLE '=' expr	{ $$ = lc($1); gen_tac_assign(lc($1), $3); my_free($3); }
	| expr '+' expr		{ $$ = gen_tac_expr($1, "+", $3); my_free($1); my_free($3); }
	| expr '-' expr		{ $$ = gen_tac_expr($1, "-", $3); my_free($1); my_free($3); }
	| expr '*' expr		{ $$ = gen_tac_expr($1, "*", $3); my_free($1); my_free($3); }
	| expr '/' expr		{ $$ = gen_tac_expr($1, "/", $3); my_free($1); my_free($3); }
	| '!' expr			{ $$ = gen_tac_expr(NULL, "!", $2); my_free($2); }		// Bitwise-not in calc lang
	| expr POWER expr	{ $$ = gen_tac_expr($1, "**", $3); my_free($1); my_free($3);}
	| '(' expr ')'		{ $$ = $2; }					// Will give syntax error for unmatched parens
	| '(' expr ')' '?' { gen_tac_if($2); } '(' expr ')'
						{
							$$ = $7;
							do_gen_else++;	// Keep track of how many closing elses are need for
							my_free($2);	// nested if/else cases
						}
	;

%%

// Used for debugging
// Print out token being freed
void my_free(char * ptr)
{
	if(ptr == NULL)
	{
		yyerror("Tried to free null pointer!");
	}
	else
	{
		// printf("Freed token: %s\n", ptr);
		free(ptr);
	}

	return;
}

// Convert a string to lower case
// Use to this to help enforce variable names being case insensitive
char* lc(char *str)
{
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		str[i] = tolower(str[i]);
	}

	return str;
}

// For case where variable is being assigned an expression
void gen_tac_assign(char * var, char * expr)
{
	track_user_var(var);

	fprintf(tac_file, "%s = %s;\n", var, expr);

	gen_tac_assign_else(var);

	return;
}

// Generates and writes out string of three address code
// Returns temporary variable's name (that must be freed later)
char* gen_tac_expr(char * one, char * op, char * three)
{
	char tmp_var_name[13]; 	// temp var names: _t0123456789

	// Create the temp variable name
	sprintf(tmp_var_name, "_t%d", num_temp_vars);
	num_temp_vars++;

	if (one != NULL)
	{
		// Write out three address code
		fprintf(tac_file, "%s = %s %s %s;\n", tmp_var_name, one, op, three);
	}
	else	// Unary operator case
	{
		fprintf(tac_file, "%s = %s%s;\n", tmp_var_name, op, three);
	}

	return strdup(tmp_var_name);
}

// Print out the if part of the if/else statement
void gen_tac_if(char * cond_expr)
{
	fprintf(tac_file, "if(%s) {\n", cond_expr);

	return;
}

// Print out closing brace of if statement and the whole else statement
// else will be a variable being assigned to a value of zero
void gen_tac_assign_else(char * expr)
{
	for (; do_gen_else > 0; do_gen_else--)
	{
		fprintf(tac_file, "} else {\n%s = 0;\n}\n", expr);
	}

	return;
}

// If the result of the conditional expression is not being written to a variable
// the else part will be empty
void gen_tac_empty_else()
{
	for (; do_gen_else > 0; do_gen_else--)
	{
		fprintf(tac_file, "} else {\n}\n");
	}

	return;
}

// Records all first appearances of user variables for use in C code generation
// If variable is not being defined and hasn't been used before, add it to list of uninitialized variables
void track_user_var(char *var)
{
	// Check if variable has been recorded before
	int i;
	for(i = 0; i < num_user_vars; i++)
	{
		if(strcmp(user_vars[i], var) == 0)
		{
			return; // If the variable was already recorded, don't need to record it again
		}
	}

	// Check if variable is valid
	if(num_user_vars >= MAX_USR_NUM_VARS)
	{
		yyerror("Max number of user variables reached");
		exit(1);	// Exit since variable (and therefor the entire program) is not valid
	}
	else if (strlen(var) > MAX_USR_VAR_NAME_LEN)
	{
		yyerror("Variable name too long");
		exit(1); 	// Exit since variable (and therefor the entire program) is not valid
	}

	strcpy(user_vars[num_user_vars], var);
	num_user_vars++;

	return;
}

void yyerror(const char *s)
{
	printf("%s\n", s);
}

int main(int argc, char *argv[])
{
	// Open the input program file
	if (argc != 2)
	{
		yyerror("Need to provide input file");
		exit(1);
	}
	else
	{
		yyin = fopen(argv[1], "r");
		if(yyin == NULL)
		{
			yyerror("Couldn't open input file");
			exit(1);
		}
	}

	// Open the output file where the three address codes will be written
	char * frontend_tac_name = "Output/tac-frontend.txt";
	tac_file = fopen(frontend_tac_name, "w");

	if (tac_file == NULL)
	{
		yyerror("Couldn't create TAC file");
		exit(1);
	}
	
	char * bb_file_name = "Output/basic-block.txt";
	init_basic_block_file(bb_file_name);

	// Read in the input program and parse the tokens
	// Also rights out frontend TAC to file
	yyparse();

	// Close the files from initial TAC generation
	fclose(yyin);
	fclose(tac_file);
	close_basic_block_file(bb_file_name);

	return 0;
}
