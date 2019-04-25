#define MAX_USR_NUM_VARS 		128
#define MAX_USR_VAR_NAME_LEN	64
#define MAX_NESTED_IFS			2

int num_temp_vars;				// Number of temp vars in use
int num_user_vars;				// Number of user variables in use
int num_user_vars_wo_def;		// Number of user variables that didn't have declarations
char user_vars[MAX_USR_NUM_VARS][MAX_USR_VAR_NAME_LEN + 1];			// List of all unique user vars in proper
char user_vars_wo_def[MAX_USR_NUM_VARS][MAX_USR_VAR_NAME_LEN + 1];	// List of user vars used w/o definition

void init_c_code();
void track_user_var(char *var, int assigned);
void gen_c_code(char * input, char * output);
