# Benjamin Steenkamer
# CPEG 621 Lab 3 - A Calculator Compiler Middle End
#
# TO DO:
# Try to remove unnecessary phi functions or arguments
#	Case where reading inside if when it was already defined inside if
# 		Many ifelseb variable y
# 		Can use solution from below problem to solve this (do before start of phi insertion)
#	Case where it is assgined value in both outer if and else
#		What if was assigned a value inside an inner if?
#	Case where it is being read after inner if/else where it was assigned two values
#		and was defined before entire if else => only needs inner if/else args for phi in this context,
# 		but shouldn't forget outer until written to on guaranteed path
#		This new phi assignment outside the inner if/should also remove those phi values since they are now joined
#			variable x in test0
#			variable c in ifelse2
#			var x in nested assign1
# 	Case where assigned in previous if, assigned in another if and then read right after
#		should not need phi if no assignment done in inner if else
#		y in uneededphi2, x in uneededphi4
# Remove unneeded basic blocks
# Verify all features
# Write report

# Generate calculator compiler middle end with basic blocks and SSA form
calc: calc.l calc.y calc.h basic-block.c basic-block.h c-code.c c-code.h ssa.c ssa.h
	bison -d calc.y
	flex calc.l
	gcc -O4 -Wall lex.yy.c calc.tab.c c-code.c basic-block.c ssa.c -o calc

# Create calc.output for debugging
debug:
	bison -v calc.y

# Compile the outputted C code for the front-end TAC and basic block TAC
ccode: Output/c-frontend.c Output/c-basic-block.c
	gcc -o Output/prog-tacf Output/c-frontend.c -lm
	gcc -o Output/prog-tacbb Output/c-basic-block.c -lm

# Same as above, but with warnings (will see unused labels)
ccodew: Output/tac-frontend.c Output/tac-basic-block.c
	gcc -Wall -o Output/prog-tacf Output/c-frontend.c -lm
	gcc -Wall -o Output/prog-tacbb Output/c-basic-block.c -lm

clean:
	rm -f calc.tab.* lex.yy.c calc.output calc
	rm -f Output/*.txt Output/*.c Output/prog-*
