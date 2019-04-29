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
	int current_id;				// Number of times variable was defined

	// Used for phi function argument tracking
	int outer_if_phi_arg;						// Current phi argument (var id) created in given context;
	int inner_if_phi_arg;						// These are the "temporary phi args" before they are written
	int inner_else_phi_arg;						// to the main array when their context is exited;
	int outer_else_phi_arg;						// Values cleared when outer else is exited

	int phi_args_if_else[MAX_NUM_PHI_ARGS];		// Array of finalized phi args from inside if/else
	int num_phi_args_if_else;					// Num values in array above

	int outside_if_else_phi_arg;	// Tracks id of most recent write outside an if/else arg

} Var_info;

Var_info vars[MAX_NUM_VARS];
int num_vars = 0;
int if_else_context = OUTSIDE_IF_ELSE;
FILE *ssa_file_ptr;

// Find the variable entry in the variable info array
// If the variable is not in the array, return -1
int _ssa_get_var_index(char *var_name)
{
	int i;
	for(i = 0; i < num_vars; i++)
	{
		if(strcmp(var_name, vars[i].var_name) == 0)
		{
			return i;
		}
	}

	return -1;
}

// Record when a variable is written, either outside or inside if/else statement
// If it's the last write to that variables in the if/else context, it will need
// to be added to the if/else phi arguments list for use the next time it is read outside the if/else context
// _ssa_store_if_else_phi_args will handle this "exiting" storage at a later time
void _ssa_phi_arg_tracker(int var_index)
{
	// current_id has already been increased before calling this function,
	// so it will correspond to variable id that has been just written to

	switch (if_else_context)
	{
		case IN_OUTER_IF_AFTER_NEST:
		case IN_OUTER_IF:
			vars[var_index].outer_if_phi_arg = vars[var_index].current_id;
			break;
		case IN_INNER_IF:
			vars[var_index].inner_if_phi_arg = vars[var_index].current_id;
			break;
		case IN_INNER_ELSE:
			vars[var_index].inner_else_phi_arg = vars[var_index].current_id;
			break;
		case IN_OUTER_ELSE:
			vars[var_index].outer_else_phi_arg = vars[var_index].current_id;
			break;
		case OUTSIDE_IF_ELSE:
			vars[var_index].outside_if_else_phi_arg = vars[var_index].current_id;
			break;
		default:	// Should never get here
			printf("\nERROR Invalid context for phi argument tracking\n\n");
	}

	return;
}

// Called when variable is known to have been assigned value in a location or
// locations that is guaranteed to be run, so can consolidate some phi arguments
void _ssa_assigned_in_guaranteed_path(int index, int type)
{
	// For the outside if/else case, the new assignment overwrites ALL previous assignments,
	if(type == ASSIGNED_OUTSIDE)
	{
		vars[index].num_phi_args_if_else = 0;
		printf("%s_%d assigned outside if/else, cleared all other phi args\n",
			vars[index].var_name, vars[index].outside_if_else_phi_arg);
	}
	else if(type == ASSIGNED_INNER_IF_ELSE)
	{
		vars[index].outer_if_phi_arg = -1;
		printf("%s assigned in both inner if/else, clearing outer_if_phi_arg\n", vars[index].var_name);
	}
	else if(type == ASSIGNED_AFTER_NEST)
	{
		if(vars[index].inner_else_phi_arg != -1)	// Else arg is stored after the if in the array
		{
			// Remove the inner else phi argument from the args array, 
			// since it has been dominated by an assignment after the nest
			vars[index].inner_else_phi_arg = -1;
			vars[index].num_phi_args_if_else--;
		}

		if(vars[index].inner_if_phi_arg != -1)
		{
			// Remove the inner if phi argument from the args array, 
			// since it has been dominated by an assignment after the nest
			vars[index].inner_if_phi_arg = -1;
			vars[index].num_phi_args_if_else--;
		}
		printf("%s_%d assigned value (directly or by phi) after nested if/else, cleared phi arg(s) from nest\n", vars[index].var_name, vars[index].current_id);
	}
	else if(type == ASSIGNED_OUTER_IF_ELSE)
	{
		// If assigned in both outer if and else; The last two indexes in phi_args_if_else
		// will be outer_if and outer_else; They will cancel out all other phi args

		int num_args = vars[index].num_phi_args_if_else;
		int assigned_outer_if = vars[index].phi_args_if_else[num_args - 2];
		int assigned_outer_else = vars[index].phi_args_if_else[num_args - 1];

		vars[index].phi_args_if_else[0] = assigned_outer_if;
		vars[index].phi_args_if_else[1] = assigned_outer_else;

		vars[index].num_phi_args_if_else = 2;
		vars[index].outside_if_else_phi_arg = -1;

		printf("%s_%d and %s_%d assigned in both outer if and outer else, cleared all other phi args\n",
				vars[index].var_name, assigned_outer_if, vars[index].var_name, assigned_outer_else);
	}
	else
	{
		printf("\nERROR Unknown type sent to _ssa_assigned_in_guaranteed_path: %d\n\n", type);
	}

	return;
}

// Takes in a user variable name that is being READ from, determines if if a
// phi function needs to be inserted, and if it does, it will write it out to the SSA
// file
void _ssa_insert_phi_func(char *var_name)
{
	int index = _ssa_get_var_index(var_name);

	// First eliminate cases where phi function not actually needed
	if (index == -1)
	{
		printf("No phi for %s, read from for the first time, \n", var_name);
		return;
	}
	else if(vars[index].num_phi_args_if_else == 0)
	{
		// var still has same ID of last outside if/else assignment
		printf("No phi for %s_%d needed here (has no phi args)\n", var_name, vars[index].current_id);
		return;
	}
	else if(if_else_context == IN_INNER_IF)
	{
		// Don't need phi if variable read in inner if was assigned a value in
		// the inner or outer if before its read
		if(vars[index].outer_if_phi_arg != -1 || vars[index].inner_if_phi_arg != -1)
		{
			printf("No phi for INNER_IF %s_%d b/c prev assignment in guaranteed path\n", vars[index].var_name, vars[index].current_id);
			return;
		}
	}
	else if(if_else_context == IN_OUTER_IF && vars[index].outer_if_phi_arg != -1)
	{
		printf("No phi for %s_%d, already assigned in OUTER_IF and is before nested if/else\n",
				vars[index].var_name, vars[index].current_id);
		return;
	}
	else if(if_else_context == IN_OUTER_IF_AFTER_NEST && vars[index].outer_if_phi_arg != -1)
	{
		if(vars[index].inner_if_phi_arg == -1 && vars[index].inner_else_phi_arg == -1)
		{
			printf("No phi for %s_%d, already assigned somewhere in OUTER_IF and no conflict with nested if/else\n",
				vars[index].var_name, vars[index].current_id);
			return;
		}
	}

	// Start the phi func insertion
	vars[index].current_id++;	// Phi function counts as assignment => need new var ID
	fprintf(ssa_file_ptr, "\t%s_%d = phi(", var_name, vars[index].current_id);
	printf("WROTE OUT:\t%s_%d = phi(", var_name, vars[index].current_id);

	// Case where both inner if and else have var written to => need make them the
	// only args for the phi functions and remove them from phi args list
	// A "shortcut" phi
	if(if_else_context == IN_OUTER_IF_AFTER_NEST &&
	   vars[index].inner_if_phi_arg != -1 && vars[index].inner_else_phi_arg != -1)
	{
		int inner_if = vars[index].inner_if_phi_arg;
		int inner_else = vars[index].inner_else_phi_arg;
		fprintf(ssa_file_ptr, "%s_%d, %s_%d);\n", var_name, inner_if, var_name, inner_else);
		printf("%s_%d, %s_%d);\n", var_name, inner_if, var_name, inner_else);

		vars[index].inner_if_phi_arg = -1;		// Clear them early so they can't be used again
		vars[index].inner_else_phi_arg = -1;

		// Since the two inner if/else phi args are now joined, can remove them from the args list
		vars[index].num_phi_args_if_else -= 2;

		// Track the new variables that is the combination of the two inner if/elses
		_ssa_phi_arg_tracker(index);
		
		printf("%s assigned in both inner if/elses and read right after, shortcut phi created\n", var_name);
		return;
	}

	// Write out outside if/else phi argument first
	if(vars[index].outside_if_else_phi_arg != -1)
	{
		fprintf(ssa_file_ptr, "%s_%d, ", var_name, vars[index].outside_if_else_phi_arg);
		printf("%s_%d, ", var_name, vars[index].outside_if_else_phi_arg);
	}

	// Variable was written into either inner if, inner else, or both
	// Can't be sure, so include the outer_if_phi_arg in case it was only one path
	// and since outer_if_phi_arg hasn't been written to the array yet
	if(if_else_context == IN_OUTER_IF_AFTER_NEST && vars[index].outer_if_phi_arg != -1)
	{
		fprintf(ssa_file_ptr, "%s_%d, ", var_name, vars[index].outer_if_phi_arg);
		printf("%s_%d, ", var_name, vars[index].outer_if_phi_arg);
	}

	// Print out phi args from inside if/elses
	int i;
	for(i = 0; i < vars[index].num_phi_args_if_else - 1; i++)
	{
		fprintf(ssa_file_ptr, "%s_%d, ", var_name, vars[index].phi_args_if_else[i]);
		printf("%s_%d, ", var_name, vars[index].phi_args_if_else[i]);
	}

	// Close phi function (i = num_phi_args_if_else - 1)
	fprintf(ssa_file_ptr, "%s_%d);\n", var_name, vars[index].phi_args_if_else[i]);
	printf("%s_%d);\n", var_name, vars[index].phi_args_if_else[i]);

	// Track this new variable id that was assigned the phi function
	_ssa_phi_arg_tracker(index);

	if(if_else_context == OUTSIDE_IF_ELSE)
	{
		// When phi function is inserted outside an if/else statement, all the 
		// previous phi function arguments can be forgotten because a guaranteed
		// join has been reached with this new assignment to the phi function
		// Also, since phi functions aren't needed inside else statements, DON'T
		// need case here for an outer else guaranteed path
		_ssa_assigned_in_guaranteed_path(index, ASSIGNED_OUTSIDE);
	}
	else if(if_else_context == IN_OUTER_IF_AFTER_NEST)
	{
		// If assigned phi arg after nested if/else can clear temp and remove 
		// from array any phi arg that was generated in the nested if/else prior
		// Reaches here if assigned only in one of the nested if/else branches,
		// because it would have used the shortcut earlier if written in both
		if(vars[index].inner_else_phi_arg != -1 || vars[index].inner_if_phi_arg != -1)
		{
			_ssa_assigned_in_guaranteed_path(index, ASSIGNED_AFTER_NEST);
		}
	}

	return;
}

// Called when an if/else context is being left
// Go through all the variables and store the phi argument values that were
// recorded in the current if/else context to the main phi argument array for
// that variable;// If possible, try to consolidate phi args
void _ssa_store_if_else_phi_args()
{
	int i;
	for (i = 0; i < num_vars; i++)
	{
		int id_to_store = -1;

		// Get the variable id to store in the main phi arg list and reset
		// variable for use in next context
		switch (if_else_context)
		{
			case IN_OUTER_IF_AFTER_NEST:	// Context 3
			case IN_OUTER_IF:				// Context 0
				id_to_store = vars[i].outer_if_phi_arg;
				break;
			case IN_INNER_IF:				// Context 1
				id_to_store = vars[i].inner_if_phi_arg;
				break;
			case IN_INNER_ELSE:				// Context 2
				id_to_store = vars[i].inner_else_phi_arg;
				break;
			case IN_OUTER_ELSE:				// Context 4
				id_to_store = vars[i].outer_else_phi_arg;
				break;
			default:	// Should never get here
				printf("\nERROR Invalid context for phi argument storing\n\n");
		}

		// If -1, means variable not written to inside if/else context (no storing needed)
		// Otherwise, store id to main phi args list
		if (id_to_store != -1)
		{
			int num_args = vars[i].num_phi_args_if_else;

			if(num_args >= MAX_NUM_PHI_ARGS)
			{
				printf("Exceeded max num phi args for %s (MAX_NUM_PHI_ARGS=%d)\n", vars[i].var_name, MAX_NUM_PHI_ARGS);
				exit(1);
			}

			vars[i].phi_args_if_else[num_args] = id_to_store;	// Copy to main array of args										// Reset for next context
			vars[i].num_phi_args_if_else++;
			printf("Recorded phi arg: %s_%d when leaving %d\n",
			vars[i].var_name, vars[i].phi_args_if_else[vars[i].num_phi_args_if_else - 1], if_else_context);
		}

		if(if_else_context == IN_INNER_ELSE)
		{
			// If assigned in both inner if and else, can clear the outer_if_phi_arg
			// since these two assignments will dominate the previous assignment (if there was one)
			if(vars[i].inner_else_phi_arg != -1 && vars[i].inner_if_phi_arg != -1)
			{
				_ssa_assigned_in_guaranteed_path(i, ASSIGNED_INNER_IF_ELSE);
			}
		}
		else if(if_else_context == IN_OUTER_ELSE)
		{
			// Consolidate phi args to just outer if/else phi args if they were both assigned
			// and no assignments done inner if and inner else (or inner if/else were already cleared)
			if(vars[i].outer_if_phi_arg != -1 && vars[i].outer_else_phi_arg != -1
			   && vars[i].inner_if_phi_arg == -1 && vars[i].inner_else_phi_arg == -1)
			{
				_ssa_assigned_in_guaranteed_path(i, ASSIGNED_OUTER_IF_ELSE);
			}
		}
	}

	return;
}

// Called after leaving the outer else
// Reset all the temporary phi args back to -1 for next if/else statement
void _ssa_clear_temp_if_else_phi_args()
{
	int i;
	for (i = 0; i < num_vars; i++)
	{
		vars[i].outer_if_phi_arg = -1;
		vars[i].inner_if_phi_arg = -1;
		vars[i].inner_else_phi_arg = -1;
		vars[i].outer_else_phi_arg = -1;
	}

	return;
}

// Takes in var name and returns malloc'd string containing it's new name in the
// form var_name + "_#"; If it's being assigned a value, need to create
// new name for var (increase number at end by one); If it is being read from,
// use last recorded number
char* _ssa_rename_var(char *var_name, int assigned)
{
	char new_var_name[MAX_USR_VAR_NAME_LEN + 16];
	strcpy(new_var_name, var_name);

	int index = _ssa_get_var_index(var_name);

	// If var name wasn't found, add it to the array
	if(index == -1)
	{
		if(num_vars < MAX_NUM_VARS)
		{
			// Create new entry for the variable and initialize it's phi tracking values
			strcpy(vars[num_vars].var_name, var_name);
			vars[num_vars].current_id = 0;
			vars[num_vars].outer_if_phi_arg = -1;
			vars[num_vars].inner_if_phi_arg = -1;
			vars[num_vars].inner_else_phi_arg = -1;
			vars[num_vars].outer_else_phi_arg = -1;
			vars[num_vars].num_phi_args_if_else = 0;
			vars[num_vars].outside_if_else_phi_arg = 0;	// Every var starts defined as var_0 with 0 as value

			index = num_vars;
			num_vars++;
		}
		else
		{
			printf("Max number of variables in program exceeded (MAX_NUM_VARS=%d)\n", MAX_NUM_VARS);
			exit(1);
		}
	}

	char ending[16];

	// When a variable is assigned a new value, it must be given a new, unique name
	if(assigned)
	{
		vars[index].current_id++;						// Increase to get new, unique ID
		sprintf(ending, "_%d", vars[index].current_id);	// Create the new ID to be appended

		_ssa_phi_arg_tracker(index);					// Need to track assignment
	}
	else
	{
		sprintf(ending, "_%d", vars[index].current_id);
	}

	strcat(new_var_name, ending);
	// printf("Renamed %s to %s\n", var_name, new_var_name);

	return strdup(new_var_name);
}

// Updates the currently known context of the if/else statement
// If the code has just left an if/else statement, need to store all the recorded
// phi arguments to the main array of each variable
// Information tracked by this is used by both phi argument generation and phi function insertion
void ssa_if_else_context_tracker(int new_context)
{
	if(new_context == IN_OUTER_IF && if_else_context == OUTSIDE_IF_ELSE)
	{
		printf("Entering outer if\n");
	}
	else if(new_context == IN_INNER_IF && if_else_context == IN_OUTER_IF)
	{
		printf("Entered an inner if\n");
	}
	else if(new_context == IN_INNER_ELSE && if_else_context == IN_INNER_IF)
	{
		printf("Exited inner if, storing phi args\n");
		_ssa_store_if_else_phi_args();
	}
	else if(new_context == IN_OUTER_IF && if_else_context == IN_INNER_ELSE)
	{
		// Special case
		printf("Exited inner else, storing phi args\n");
		_ssa_store_if_else_phi_args();				// Still store phi args from INNER_ELSE
		if_else_context = IN_OUTER_IF_AFTER_NEST;	// New state after the next if/else
		return;
	}
	else if(new_context == IN_OUTER_ELSE)
	{
		// Going from outer if that didn't contain nested if/else OR from outer if that did
		// contain a nested if/else
		if(if_else_context == IN_OUTER_IF || if_else_context == IN_OUTER_IF_AFTER_NEST)
		{
			printf("Exited outer if, storing phi args\n");
			_ssa_store_if_else_phi_args();
		}
		else
		{
			printf("\nERROR Entering UNKNOWN state: current=%d new=%d\n\n", if_else_context, new_context);
		}
	}
	else if(new_context == OUTSIDE_IF_ELSE && if_else_context == IN_OUTER_ELSE)
	{
		printf("Exited outer else, storing phi args and then clearing all temps\n");
		_ssa_store_if_else_phi_args();
		_ssa_clear_temp_if_else_phi_args();
	}
	else	// Should never get here
	{
		printf("\nERROR Entering UNKNOWN state: current=%d new=%d\n\n", if_else_context, new_context);
	}

	if_else_context = new_context;	// Update context value
	return;
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
	char buffer[(MAX_USR_VAR_NAME_LEN * 4) + 64];	// Have room for 3 user vars in renamed form
	strcpy(buffer, tac_line);

	// If-statement case
	if(strstr(buffer, "if(") != NULL)
	{
		strtok(buffer, " \t()");		// Skip over if part
		char * cond = strtok(NULL, " \t()");

		// !var or !# wouldn't appear in conditional; would always be saved to temp first,
		// then temp var would be in conditional
		if(cond[0] == '_' || cond[0] < 'A')	// Don't need to do anything with temps or vars
		{
			printf("temp or const in if: %s", tac_line);
			ssa_print_line(tac_line);
		}
		else
		{
			_ssa_insert_phi_func(cond);
			char * new_cond = _ssa_rename_var(cond, 0);

			printf("WROTE OUT:\tif(%s) {\n", new_cond);
			fprintf(ssa_file_ptr, "\tif(%s) {\n", new_cond);

			free(new_cond);
		}
	}
	else	// Assignment cases: a = (!)b; a = b op c;
	{
		char token_array[5][MAX_USR_VAR_NAME_LEN + 16];		// Max of 5 tokens in a line
		char assigned_user_var[MAX_USR_VAR_NAME_LEN + 1];	// User variable assigned this line
		strcpy(assigned_user_var, "");

		int token_count = 0;
		char *temp_token = strtok(buffer, " \t;\n");

		// Index all tokens into the array
		while(temp_token != NULL)
		{
			strcpy(token_array[token_count], temp_token);
			temp_token = strtok(NULL, " \t;\n");
			token_count++;
		}

		// Index:    0    1     2    3      4
		// Case "diff_var = same_var op same_var" don't need a phi for
		// for the inner same_var since the outer will already insert one
		int phi_not_needed_idx2 = 0;
		if(token_count == 5 && (strcmp(token_array[2], token_array[4]) == 0))
		{
			// Don't need phi for the inner same_var
			phi_not_needed_idx2 = 1;
		}

		// Traverse token array from right to left so assignment token is processed at the end
		int i;
		for(i = token_count - 1; i >= 0; i--)
		{
			if(token_array[i][0] == '!')	// Unary operator case (!user_var, !temp, or !#)
			{
				if(token_array[i][1] == '_' || token_array[i][1] < 'A')	// Don't need to change constant or temp
				{
					continue;
				}
				else	// Case: !user_var
				{
					temp_token = token_array[i] + 1; 		// Move past the "!"

					_ssa_insert_phi_func(temp_token);
					char *new_name = _ssa_rename_var(temp_token, 0);

					strcpy(token_array[i] + 1, new_name);	// ! operator will still be intact

					free(new_name);
				}
			}
			else if(token_array[i][0] == '=' || token_array[i][0] < '0') // Operator case (=, +, -, *, /, **)
			{
				continue;
			}
			else	// Variable (_t# or user_var) or constant case
			{
				if(token_array[i][0] == '_' || token_array[i][0] < 'A')	// Don't need to do anything with temps or vars
				{
					continue;
				}
				else
				{
					// User variable being read from, MAY need to insert phi function
					char * new_name;
					if(i > 0)
					{
						if(i == 2 && phi_not_needed_idx2)
						{
							// Case "diff_var = same_var op same_var", don't need phi
							// for same_var at index 2 since index 4 same_var already inserted one
							printf("Extra phi not needed: diff_var = same_var op same_var\n");
						}
						else
						{
							_ssa_insert_phi_func(token_array[i]);
						}

						new_name = _ssa_rename_var(token_array[i], 0);
					}
					else // The first token in a TAC line is being assigned a value (no phi needed)
					{
						strcpy(assigned_user_var, token_array[i]);		// Record the user variable that was assigned
						new_name = _ssa_rename_var(token_array[i], 1);
						// THE RENAME FUNCTION WILL AUTOMATICALLY CALL _ssa_phi_arg_tracker
						// TO RECORD THIS ASSIGNMENT BEFORE THE NEW NAME IS GENERATED
					}

					strcpy(token_array[i], new_name);
					free(new_name);
				}
			}
		}

		char new_line[(MAX_USR_VAR_NAME_LEN + 16) * 4];
		strcpy(new_line, "\t");

		for (i = 0; i < token_count - 1; i++)
		{
			strcat(new_line, token_array[i]);
			strcat(new_line, " ");
		}

		strcat(new_line, token_array[token_count - 1]);	// Add last token
		strcat(new_line, ";\n");						// and close TAC line

		printf("WROTE OUT: %s", new_line);
		fprintf(ssa_file_ptr, "%s", new_line);

		// Check if value was assigned in a "guaranteed path" If so, can clear
		// unneeded phi args before next time phi function is needed
		// String will be empty if assigned var was temp variable
		if(strcmp(assigned_user_var, "") != 0)
		{
			// Index guaranteed to not be -1 since it was assigned value
			int index = _ssa_get_var_index(assigned_user_var);
			
			if(if_else_context == OUTSIDE_IF_ELSE)
			{
				// If assigned value outside if/else, clear unneeded phi args
				// Need to do this at end of function to not interfere with potential self assignment
				// needing phi functions
				_ssa_assigned_in_guaranteed_path(index, ASSIGNED_OUTSIDE);
			}
			else if(if_else_context == IN_OUTER_IF_AFTER_NEST)
			{
				// Consolidate phi args if variable was assigned value after nested if/else
				// This value would dominate the value(s) assigned in the nest
				if(vars[index].inner_else_phi_arg != -1 || vars[index].inner_if_phi_arg != -1 )
				{
					_ssa_assigned_in_guaranteed_path(index, ASSIGNED_AFTER_NEST);
				}
			}
		}
	}

	return;
}

// Just prints out the line passed in from the basic block generation
void ssa_print_line(char *line)
{
	fprintf(ssa_file_ptr, line);

	return;
}

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
