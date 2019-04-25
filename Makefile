# Benjamin Steenkamer
# CPEG 621 Lab 3 - A Calculator Compiler Middle End
#
# TO DO:
# Add back C generation code for testing
# Compare Basic blocks to front end
# Implement SSA
#	Variable names
#	Phi insertion
# Sweep to remove empty basic blocks
# Verify all features
#	Test O4 vs normal speed
# Write report

calc: calc.l calc.y basic-block.c basic-block.h
	bison -d calc.y
	flex calc.l
	gcc -O4 -Wall lex.yy.c calc.tab.c basic-block.c -o calc

# Create calc.output for debugging
debug:
	bison -v calc.y

clean:
	rm -f calc.tab.* lex.yy.c calc.output calc
	rm -f Output/tac-frontend.txt Output/basic-block.txt
