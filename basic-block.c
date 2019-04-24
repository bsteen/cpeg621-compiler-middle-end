// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#include <stdio.h>
#include <stdlib.h>

FILE * bb_file_ptr;

void init_basic_block_file(char * file_name)
{
	bb_file_ptr = fopen(file_name, "w");
	
	if(bb_file_ptr == NULL)
	{
		printf("Couldn't open %s\n", file_name);
		exit(1);
	}
	
	fprintf(bb_file_ptr, "BB1:\n");
	
	return;
}

void close_basic_block_file(char * file_name)
{
	int error = fclose(bb_file_ptr);
	
	if(error != 0)
	{
		printf("Couldn't close %s\n", file_name);
	}
	
	return;
}