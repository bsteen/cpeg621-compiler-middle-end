// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <stdio.h>
#include <stdlib.h>
#include "basic-block.h"
#include "calc.h"
#include "ssa.h"

FILE * bb_file_ptr;
int next_block_tag;
int outer_else_tag;
int is_nested;

// Initialize the basic block file
// Write out the basic block first label in BB and SSA files
void bb_init_files(char * bb_file_name, char * ssa_file_name)
{
	bb_file_ptr = fopen(bb_file_name, "w");

	if(bb_file_ptr == NULL)
	{
		printf("Couldn't open %s\n", bb_file_name);
		exit(1);
	}

	ssa_init_file(ssa_file_name);

	fprintf(bb_file_ptr, "BB1:\n");	// First statement is the leader of the first block
	ssa_print_line("BB1:\n");

	next_block_tag = 2;
	outer_else_tag = -1;
	is_nested = 0;

	return;
}

// Prints out the TAC generated in the calc.y file into basic block file
// Sends line to SSA module for printing in SSA's output file
void bb_print_tac(char *tac)
{
	char buffer[MAX_USR_VAR_NAME_LEN * 4];
	sprintf(buffer, "\t%s", tac);

	fprintf(bb_file_ptr, buffer);
	ssa_process_tac(buffer);

	return;
}

// Prints out the next block tag in the sequence
// THEN, increases value for new next block tag
// Sends line to SSA module for printing in SSA's output file
void _bb_print_next_blk_tag()
{
	char buffer[MAX_USR_VAR_NAME_LEN * 4];
	sprintf(buffer, "\nBB%d:\n", next_block_tag);

	fprintf(bb_file_ptr, buffer);
	ssa_print_line(buffer);

	next_block_tag++;

	return;
}

// Prints out the end of the basic block, which will contain an if/else
// statement that splits the flow (and ends the current basic block)
// Sends lines to SSA module for printing in SSA's output file
void bb_print_if_else_block_end(char *if_stmt, int entering_nested_if)
{
	if(entering_nested_if)					// Just entering nested if/else
	{
		outer_else_tag = next_block_tag;	// Tag used previously is outer else's tag
		next_block_tag++;					// So next block doesn't reuse the else's tag
		is_nested = 1;
	}

	char buffer[MAX_USR_VAR_NAME_LEN * 4];	// Enough room for variable and gotos

	sprintf(buffer, "\t%s", if_stmt);	// Print out if statement
	fprintf(bb_file_ptr, buffer);
	ssa_process_tac(buffer);

	sprintf(buffer, "\t\tgoto BB%d;\n", next_block_tag);
	fprintf(bb_file_ptr, buffer);
	ssa_print_line(buffer);

	sprintf(buffer, "\t} else {\n\t\tgoto BB%d;\n\t}\n", next_block_tag + 1);
	fprintf(bb_file_ptr, buffer);
	ssa_print_line(buffer);

	_bb_print_next_blk_tag();

	return;
}

// Prints out the basic block that contains the else-statement's logic
// Sends lines to SSA module for printing in SSA's output file
void bb_print_else_block(char * var_name, int leaving_outer_if)
{
	// printf("%s: leaving_outer_if=%d nested=%d\n", var_name, leaving_outer_if, is_nested);

	char buffer[MAX_USR_VAR_NAME_LEN * 4];

	if(is_nested && leaving_outer_if)	// Just exited outer if-statement of nested if/else structure
	{
		// DON'T need plus one to next_block_tag b/c next block will be outer else with SPECIAL TAG
		sprintf(buffer, "\tgoto BB%d;\n", next_block_tag);	// goto at end of if-statement logic
		fprintf(bb_file_ptr, buffer);
		ssa_print_line(buffer);

		sprintf(buffer, "\nBB%d:\n", outer_else_tag);	// Use outer else tag instead of next tag
		fprintf(bb_file_ptr, buffer);
		ssa_print_line(buffer);

		is_nested = 0;	// No longer in the next if/else statements
	}
	else
	{
		// NEED next_block_tag + 1 to step over else block tag and get to statement after the else-statement
		sprintf(buffer, "\tgoto BB%d;\n", next_block_tag + 1); 	// goto at end of if logic
		fprintf(bb_file_ptr, buffer);
		ssa_print_line(buffer);

		_bb_print_next_blk_tag();
	}

	if(var_name != NULL) // Print else assignment to 0; If NULL, no value assigned to conditional result
	{
		sprintf(buffer, "\t%s = 0;\n", var_name);
		fprintf(bb_file_ptr, buffer);
		ssa_print_line(buffer);
	}

	// Jump to first statement outside the completed else-statement
	sprintf(buffer, "\tgoto BB%d;\n", next_block_tag);
	fprintf(bb_file_ptr, buffer);	// goto at end of else logic
	ssa_print_line(buffer);

	_bb_print_next_blk_tag();

	return;
}

void bb_close_files(char *bb_file_name, char *ssa_file_name)
{
	int error = fclose(bb_file_ptr);

	if(error != 0)
	{
		printf("Couldn't close %s\n", bb_file_name);
	}

	ssa_close_file(ssa_file_name);

	return;
}
