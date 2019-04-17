# Benjamin Steenkamer
# CPEG 621 Lab 3 - A Calculator Compiler Middle End
#
calc: calc.l calc.y
	bison -d calc.y
	flex calc.l
	gcc -Wall lex.yy.c calc.tab.c -o calc

# Create calc.output for debugging
debug:
	bison -v calc.y

clean:
	rm -f calc.tab.* lex.yy.c calc.output calc
	rm -f Output/tac-frontend.txt
