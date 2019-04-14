main: main.c mx/vector.c mx/string.c mx/common.c
	clang -Wall -O2 -g -o main main.c mx/vector.c mx/string.c mx/common.c

serial: serial.c mx/vector.c mx/string.c mx/common.c
	clang -Wall -O2 -g -o serial serial.c mx/vector.c mx/string.c mx/common.c
