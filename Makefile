# Benjamin Steenkamer
# CPEG 621 Lab 3 - A Calculator Compiler Middle End
#
# TO DO:
#	Basic blocks
# Implement Basic blocks
# Implement SSA
#	Variable names
#	Phi insertion
# Verify all features
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
