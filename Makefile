
all: hsm_tests_1 hsm_tests_2

hsm_tests_1: hsm.c hsm.h ./tests/tests_1.c Makefile
	gcc -Wall -g3 -O0 hsm.c ./tests/tests_1.c -o ./tests/hsm_tests_1 -DHSM_TEST

hsm_tests_2: hsm.c hsm.h ./tests/tests_2.c Makefile
	gcc -Wall -g3 -O0 hsm.c ./tests/tests_2.c -o ./tests/hsm_tests_2 -DHSM_TEST
