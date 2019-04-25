// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <stdio.h>
#include <stdlib.h>
#include "basic-block.h"

FILE * bb_file_ptr;
int next_block_tag;
int outer_else_tag;
int is_nested;

void bb_init_file(char * file_name)
{
	bb_file_ptr = fopen(file_name, "w");

	if(bb_file_ptr == NULL)
	{
		printf("Couldn't open %s\n", file_name);
		exit(1);
	}

	fprintf(bb_file_ptr, "BB1:\n");
	next_block_tag = 2;
	outer_else_tag = -1;
	is_nested = 0;

	return;
}

// Prints out the TAC generated in the calc.y file into basic block file
void bb_print_tac(char *tac)
{
	fprintf(bb_file_ptr, "\t%s", tac);

	return;
}

// Prints out the next block tag in the sequence
// THEN, increases value for new next block tag
void _bb_print_next_blk_tag()
{
	fprintf(bb_file_ptr, "\nBB%d:\n", next_block_tag);
	next_block_tag++;

	return;
}

// Prints out the end of the basic block, which will contain an if/else
// statement that slits the flow (and ends the current base block)
void bb_print_if_else_block_end(char *if_stmt, int entering_nested_if)
{
	if(entering_nested_if)					// Just entering nested if/else
	{
		outer_else_tag = next_block_tag;		// Tag used previously is outer else's tag
		next_block_tag++;					// So next block doesn't reuse the else's tag
		is_nested = 1;
	}
	
	fprintf(bb_file_ptr, "\t%s", if_stmt);
	fprintf(bb_file_ptr, "\t\tgoto BB%d;\n", next_block_tag);
	fprintf(bb_file_ptr, "\t} else {\n\t\tgoto BB%d;\n\t}\n", next_block_tag + 1);

	_bb_print_next_blk_tag();

	return;
}

// Prints out the basic block that contains the else-statement's logic
void bb_print_else_block(char * var_name, int leaving_outer_if)
{
	// printf("%s: leaving_outer_if=%d nested=%d\n", var_name, leaving_outer_if, is_nested);
	
	if(is_nested && leaving_outer_if)	// Just exited outer if-statement of nested if/else structure
	{
		// DON'T need plus one to next_block_tag b/c next block will be outer else with special tag
		fprintf(bb_file_ptr, "\tgoto BB%d;\n", next_block_tag);	// goto at end of if-statement logic
		is_nested = 0;
		fprintf(bb_file_ptr, "\nBB%d:\n", outer_else_tag);	// Use outer else tag instead of next tag
	}
	else
	{
		// NEED next_block_tag + 1 to step over else block and get to statement after the else-statement
		fprintf(bb_file_ptr, "\tgoto BB%d;\n", next_block_tag + 1);	// goto at end of if logic
		_bb_print_next_blk_tag();
	}

	if(var_name != NULL)
	{
		fprintf(bb_file_ptr, "\t%s = 0;\n", var_name);
	}

	// Jump to first statement outside the completed else-statement
	fprintf(bb_file_ptr, "\tgoto BB%d;\n", next_block_tag);	// goto at end of else logic
	_bb_print_next_blk_tag();

	return;
}

void bb_close_file(char * file_name)
{
	int error = fclose(bb_file_ptr);

	if(error != 0)
	{
		printf("Couldn't close %s\n", file_name);
	}

	return;
}
