// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#define IN_OUTER_IF 	0
#define	IN_OUTER_ELSE	1
#define IN_INNER_IF 	3
#define IN_INNER_ELSE	4
#define OUTSIDE_IF_ELSE -1

void ssa_init_file(char *ssa_file_name);
void ssa_if_else_context_tracker(int context);
void ssa_process_tac(char *tac_line);
void ssa_print_line(char *line);
void ssa_close_file(char *ssa_file_name);
