CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
HELGRIND = valgrind --tool=helgrind

all: mmu_test

mmu_test: main.c mmu_mock.c segment_mock.c page_mock.c
	$(CC) $(CFLAGS) -o $@ $^

run: mmu_test
	./mmu_test

valgrind: mmu_test
	$(VALGRIND) ./mmu_test

helgrind: mmu_test
	$(HELGRIND) ./mmu_test

clean:
	rm -f mmu_test *.o