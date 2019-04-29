// Benjamin Steenkamer
// CPEG 621 Lab 3 - A Calculator Compiler Middle End

#define OUTSIDE_IF_ELSE 		-1
#define IN_OUTER_IF 			0
#define IN_INNER_IF 			1
#define IN_INNER_ELSE			2
#define IN_OUTER_IF_AFTER_NEST 	3
#define	IN_OUTER_ELSE			4


#define ASSIGNED_OUTSIDE			10
#define ASSIGNED_OUTER_IF_ELSE		11

void ssa_init_file(char *ssa_file_name);
void ssa_if_else_context_tracker(int context);
void ssa_process_tac(char *tac_line);
void ssa_print_line(char *line);
void ssa_close_file(char *ssa_file_name);
