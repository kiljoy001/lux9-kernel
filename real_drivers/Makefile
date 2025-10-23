# Makefile for real hardware drivers
CC = gcc
CFLAGS = -Wall -Werror -I../kernel/include -I../kernel/9front-port -I../kernel/9front-pc64

# Build real drivers using actual 9front code as reference
all: ahci_driver.o ide_driver.o

# Extract and adapt real 9front AHCI driver code
ahci_driver.o: ahci_driver.c
	$(CC) $(CFLAGS) -c ahci_driver.c -o ahci_driver.o

# Extract and adapt real 9front IDE driver code  
ide_driver.o: ide_driver.c
	$(CC) $(CFLAGS) -c ide_driver.c -o ide_driver.o

# Clean build files
clean:
	rm -f *.o

.PHONY: all clean