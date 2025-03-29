CC = mpicc
DEBUG_FLAGS = -Wall -pedantic -g
RELEASE_FLAGS = -O3
DEPS = -lrscore -lrsrng -lm

srcs = src/*.c

all: rblocksim rblocksim_debug rblocksim_asan

rblocksim_asan:
	$(CC) $(DEBUG_FLAGS) $(srcs) -o rblocksim_asan $(DEPS) -fsanitize=address

rblocksim_debug: $(srcs)
	$(CC) $(DEBUG_FLAGS) $(srcs) -o rblocksim_debug $(DEPS)

rblocksim: $(srcs)
	$(CC) $(RELEASE_FLAGS) $(srcs) -o rblocksim $(DEPS)

$(srcs):
	$(srcs)

.PHONY: clean all rblocksim rblocksim_asan rblocksim_debug
clean:
	rm -f rblocksim rblocksim_debug rblocksim_asan
