
all: hsm_tests

hsm_tests: hsm.c hsm.h ./tests/tests.c
	gcc -Wall hsm.c ./tests/tests.c -o ./tests/hsm_tests 
