.PHONY: clean

# shen: 	backend.c
# 	cc -Wall -ansi -o shen backend.c

shen: 	interpreter.c
	cc -g -Wall -ansi -o shen interpreter.c


clean:
	rm shen
