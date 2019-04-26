// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

void bb_init_files(char * bb_file_name, char * ssa_file_name);
void bb_print_tac(char *tac);
void bb_print_if_else_block_end(char *if_stmt, int entering_nested_if);
void bb_print_else_block(char * var_name, int leaving_outer_if);
void bb_close_files(char * bb_file_name, char * ssa_file_name);
