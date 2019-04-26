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

void ssa_rename_vars(char *tac_line)
{
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
	return;
}

// Just prints out the line based in from the basic block generation
void ssa_print_line(char *line)
{
	fprintf(ssa_file_ptr, line);
	
	return;
}

// Takes the front end TAC code that passed through the basic block generation
// and processes it for SSA form; this includes inserting phi functions and 
// renaming variables
void ssa_process_tac(char *tac_line)
{
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
