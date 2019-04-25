# Benjamin Steenkamer
# CPEG 621 Lab 3 - A Calculator Compiler Middle End
#
# TO DO:
# Implement SSA
#	Variable names
#	Phi insertion
# Sweep to remove empty basic blocks
# Verify all features
#	Test O4 vs normal speed
# Write report

# Generate calculator compiler middle end with basic blocks and SSA form
calc: calc.l calc.y calc.h basic-block.c basic-block.h c-code.c c-code.h
	bison -d calc.y
	flex calc.l
	gcc -O4 -Wall lex.yy.c calc.tab.c c-code.c basic-block.c -o calc

# Create calc.output for debugging
debug:
	bison -v calc.y

# Compile the outputted C code for the front-end TAC and basic block TAC
ccode: Output/tac-frontend.c Output/tac-basic-block.c
	gcc -o Output/prog-tacf Output/tac-frontend.c -lm
	gcc -o Output/prog-tacbb Output/tac-basic-block.c -lm

# Same as above, but with warnings (will see unused labels)
ccodew: Output/tac-frontend.c Output/tac-basic-block.c
	gcc -Wall -o Output/prog-tacf Output/c-frontend.c -lm
	gcc -Wall -o Output/prog-tacbb Output/c-basic-block.c -lm

clean:
	rm -f calc.tab.* lex.yy.c calc.output calc
	rm -f Output/*.txt Output/*.c Output/prog-*