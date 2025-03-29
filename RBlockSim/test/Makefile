CC = mpicc
DEBUG_FLAGS = -Wall -pedantic -g
RELEASE_FLAGS = -O3
DEPS = -lrscore -lrsrng -lm
WRAPS := rs_malloc rs_calloc rs_realloc rs_free
WRAP_CMD := $(foreach var,$(WRAPS),-Wl,--wrap=$(var))
srcs =*.c ../src/*.c
DEFS = -DTESTING

all: rblocksim_tests rblocksim_tests_asan

rblocksim_tests:
	$(CC) $(DEBUG_FLAGS) $(srcs) -o rblocksim_tests $(DEPS) $(WRAP_CMD) $(DEFS)

rblocksim_tests_asan:
	$(CC) $(DEBUG_FLAGS) $(OPTIONS) $(srcs) -o rblocksim_tests_asan $(DEPS) $(WRAP_CMD) $(DEFS) -fsanitize=address

.PHONY: clean all rblocksim_tests rblocksim_tests_asan
clean:
	rm -f rblocksim_tests rblocksim_tests_asan
