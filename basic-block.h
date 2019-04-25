// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

void bb_init_file(char *file_name);
void bb_print_tac(char *tac);
void bb_print_if_else_block_end(char *if_stmt, int entering_nested_if);
void bb_print_else_block(char * var_name, int leaving_outer_if);
void bb_close_file(char *file_name);
