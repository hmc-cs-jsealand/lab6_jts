#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////

// GPIO FSEL Types
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2

#define GPFSEL   ((volatile unsigned int *) (gpio + 0))
#define GPSET    ((volatile unsigned int *) (gpio + 7))
#define GPCLR    ((volatile unsigned int *) (gpio + 10))
#define GPLEV    ((volatile unsigned int *) (gpio + 13))
#define INPUT  0
#define OUTPUT 1

// base address of timer registers
#define SYSTIMER	0x3F003000

// Physical addresses
#define BCM2836_PERI_BASE        0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define BLOCK_SIZE (4*1024)

// Pointer that will be memory mapped when pioInit() is called
volatile unsigned int *gpio; //pointer to base of gpio

// Timer pointer that will be memory mapped when piTimerInit() is called
volatile unsigned int *timer; //pointer to base of timer stuff


void pioInit() {
	int  mem_fd;
	void *reg_map;

	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}

	reg_map = mmap(
	  NULL,             //Address at which to start local mapping (null means don't-care)
      BLOCK_SIZE,       //Size of mapped memory block
      PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
      MAP_SHARED,       // This program does not have exclusive access to this memory
      mem_fd,           // Map to /dev/mem
      GPIO_BASE);       // Offset to GPIO peripheral

	if (reg_map == MAP_FAILED) {
      printf("gpio mmap error %d\n", (int)reg_map);
      close(mem_fd);
      exit(-1);
    }

	gpio = (volatile unsigned *)reg_map;
}

void piTimerInit() {
	int  mem_fd;
	void *reg_map;

	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}

	reg_map = mmap(
	  NULL,             //Address at which to start local mapping (null means don't-care)
      BLOCK_SIZE,       //Size of mapped memory block
      PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
      MAP_SHARED,       // This program does not have exclusive access to this memory
      mem_fd,           // Map to /dev/mem
      SYSTIMER);       // Offset to timer address

	if (reg_map == MAP_FAILED) {
      printf("gpio mmap error %d\n", (int)reg_map);
      close(mem_fd);
      exit(-1);
    }

	timer = (volatile unsigned *)reg_map;
}

// pinMode sets a pin to be an INPUT, OUTPUT, or other state
void pinMode(int pin, int state) {
	unsigned int offset = pin/10;
	unsigned int shift = (pin % 10) * 3;
	GPFSEL[offset] &= ~(7 << shift);
	GPFSEL[offset] |= (state << shift);
}

// digitalWrite writes a 0 or 1 to the passed in pin
void digitalWrite(int pin, int val) {
	unsigned int set = (pin < 32) ? 7 : 8;
	unsigned int clr = (pin < 32) ? 10 : 11;
	if (val == 1)
		gpio[set] = 1 << (pin % 32);
	else
		gpio[clr] = 1 << (pin % 32);
}

// digitalRead reads in a digital value from the passed in pin
int digitalRead(int pin) {
	int out;
	if (pin < 32)
		out = (GPLEV[0] >> pin) && 1;
	else
		out = (GPLEV[1] >> (pin - 32)) && 1;
	return out;
}

// delayMicros delays for micros microseconds
void delayMicros(int micros) {
	timer[4] = timer[1] + micros;
	timer[0] = 0b0010;
	while (!(timer[0] & 0b0010));
}