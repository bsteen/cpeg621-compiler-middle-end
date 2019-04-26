// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "ssa.h"

typedef struct var_info
{
	char var_name[MAX_USR_VAR_NAME_LEN + 1];
	int num_defined;	// Number of times variable was defined (first use counts as one "definition")
} Var_info;

Var_info vars[MAX_NUM_VARS];
int num_vars = 0;
FILE *ssa_file_ptr;

// Open the file that will contain the complete SSA form output
void ssa_init_file(char *ssa_file_name)
{
	ssa_file_ptr = fopen(ssa_file_name, "w");

	if(ssa_file_ptr == NULL)
	{
		printf("Couldn't open %s\n", ssa_file_name);
		exit(1);
	}
}

// Just prints out the line based in from the basic block generation
void ssa_print_line(char *line)
{
	fprintf(ssa_file_ptr, line);

	return;
}

// Find the variable entry in the variable info array
// If the variable is not in the array, add it and send back new index
int _get_var_index(char *var_name)
{
	int i;
	for(i = 0; i < num_vars; i++)
	{
		if(strcmp(var_name, vars[i].var_name) == 0)
		{
			return i;
		}
	}

	if(num_vars < MAX_NUM_VARS)
	{
		strcpy(vars[num_vars].var_name, var_name);
		vars[num_vars].num_defined = 1;
		num_vars++;

		return num_vars - 1;
	}
	else
	{
		printf("Max number of variables in program exceeded (MAX_NUM_VARS=%d)\n", MAX_NUM_VARS);
		exit(1);
	}
}

// Takes in a user variable name that is being READ from, determines if if a
// phi function needs to be inserted, and it does, it will write it out to the SSA
// file with the adjusted variable name for the assignment and phi function arguments
void _ssa_insert_phi(char *var_name)
{
	// When inserted, it also needs to update the var assignment counter
	// Case where assigned value inside if/else then read or written to in another if else later
	// Clear phi function variables when phi inserted into a guaranteed to run location

	return;
}

// Takes in var name and returns malloc'd string containing it's new name
// New name is old + "_#", determined by amount of previous assignments
char *_ssa_rename_var(char *var_name)
{
	char *new_var_name = malloc(sizeof(char) * (MAX_USR_VAR_NAME_LEN + 16));

	strcpy(new_var_name, var_name);	// FOR TESTING

	/*
	// Ignore temp vars, they don't need to be renamed

	// Don't need to touch: "}", "} else {", "BB#:", "goto BB", newlines
	if(strstr(line, "}") != NULL || strstr(line, ":") != NULL
	   || strstr(line, "goto BB") != NULL || strcmp(line, "\n") == 0)
	{
		printf("Nothing to do: %s", line);
		fprintf(saa_file_ptr, line);
	}
	else if(strstr(line, "if(") != NULL) // If statement case
	{
		char *cond = strtok(line, " ()");

		if(cond[0] < 'A') // Ignore constants
		{

		}
		else
		{
			printf("Nothing to do:\tif(%s) {\n", cond);
			fprintf(saa_file_ptr, "\tif(%s) {\n", cond);
		}
	}
	else
	{
		// Normal assignment case
		// Ignore constants
	}


	// Can't just be last x assignments for phi args (could be defined several times inside)
	// Go back and find last assignment in each branch of if/else + plus last assignment before if/else

	// If the variable was first defined inside an if-statement (x0) and then read outside it,
	// Would need phi function and would choose between self and if value (would be x1 = phi(x0, x1))
	*/

	return new_var_name;
}

// Takes the front end TAC code that passed through the basic block generation
// and processes it for SSA form; this includes inserting phi functions and
// renaming variables
// General operation order:
// 		insert phi function if needed
// 		rename vars
//		print out line to SSA file
void ssa_process_tac(char *tac_line)
{
	// Tokenize TAC input
	char buffer[MAX_USR_VAR_NAME_LEN * 4];
	strcpy(buffer, tac_line);

	// If-statement case
	if(strstr(buffer, "if(") != NULL)
	{
		strtok(buffer, " \t()");		// Skip over if part
		char * cond = strtok(NULL, " \t()");

		if(cond[0] == '_' || cond[0] < 'A')	// Don't need to do anything with temps or vars
		{
			// printf("temp or const in if: %s", tac_line);
			ssa_print_line(tac_line);
		}
		else
		{
			_ssa_insert_phi(cond);
			char * new_cond = _ssa_rename_var(cond);
			
			// printf("Wrote out: \tif(%s) {\n", new_cond);
			fprintf(ssa_file_ptr, "\tif(%s) {\n", new_cond);
			
			free(new_cond);
		}
	}
	else	// Assignment cases: a = (!)b; a = b op c;
	{	
		char new_line[MAX_USR_VAR_NAME_LEN * 4];
		strcpy(new_line, "\t");		// Must COPY in first, not strcat (first index may not be NULL)
		
		char *token = strtok(buffer, " \t;\n");
		int token_counter = 1;
		
		while(token != NULL)
		{		
			if(token[0] == '!')	// Unary operator case (!var name)
			{
				token++; 		// Move past the !
				_ssa_insert_phi(token);
				char * new_name =  _ssa_rename_var(token);

				strcat(new_line, "!");		// Re-add the ! operator
				strcat(new_line, new_name);
				
				free(new_name);
			}
			else if(token[0] == '=' || token[0] < '0') // Operator case (=, +, -, *, /, **)
			{
				strcat(new_line, " ");
				strcat(new_line, token);
				strcat(new_line, " ");
			}
			else	// Variable (temp or user) or constant case
			{
				if(token[0] == '_' || token[0] < 'A')	// Don't need to do anything with temps or vars
				{
					strcat(new_line, token);
				}
				else
				{
					// The first token in a TAC line is being assigned a value
					// Don't need to insert phi function in this case
					if(token_counter > 1)
					{
						_ssa_insert_phi(token);
					}
					
					char * new_name =  _ssa_rename_var(token);
					strcat(new_line, new_name);
					
					free(new_name);
				}
			}
			
			token = strtok(NULL, " \t;\n");
			token_counter++;
		}
		
		strcat(new_line, ";\n");
		// printf("Wrote out: %s", new_line);
		fprintf(ssa_file_ptr, "%s", new_line);
	}

	return;
}

// Close file with the completed SSA form output
void ssa_close_file(char *ssa_file_name)
{
	int error = fclose(ssa_file_ptr);

	if(error != 0)
	{
		printf("Couldn't close %s\n", ssa_file_name);
	}

	return;
}
