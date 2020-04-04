#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/avr_ioport.h>
#include <simavr/avr_spi.h>

#include "sd.h"

sd_t sd;

void sigint_handler(int sig) {
	fprintf(stderr, "Saving SD card to disk...\n");
	sd_free(&sd);
	fprintf(stderr, "Exiting cleanly!\n");
	exit(0);
}

// TODO: look into why GCC complains about this curly brace. It still works.
const struct sigaction sigint_action = {
	.sa_handler = &sigint_handler,
	.sa_mask = 0,
	.sa_flags = 0,
};

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Specify a .elf file and a disk image, in that order.\n");
		return 1;
	}

	avr_t *avr;
	elf_firmware_t firmware;
	if (elf_read_firmware(argv[1], &firmware)) {
		fprintf(stderr, "Error reading firmware\n");
		return 1;
	}
	if (!(avr = avr_make_mcu_by_name("atmega328p"))) {
		fprintf(stderr, "Error creating avr object\n");
		return 1;
	}
	avr_init(avr);
	avr_load_firmware(avr, &firmware);

	if (sd_init(&sd, avr, argv[2]) != 0) {
		fprintf(stderr, "Error initializing SD card: %s\n",
			strerror(errno));
		return 1;
	}
	avr_connect_irq(
		avr_io_getirq(avr, AVR_IOCTL_SPI_GETIRQ(0), SPI_IRQ_OUTPUT),
		sd.irq + SD_IRQ_MOSI);
	avr_connect_irq(
		sd.irq + SD_IRQ_MISO,
		avr_io_getirq(avr, AVR_IOCTL_SPI_GETIRQ(0), SPI_IRQ_INPUT));
	avr_connect_irq(
		// "digital pin 10" is actually PORTB pin 2
		avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 4),
		sd.irq + SD_IRQ_CS);

	sigaction(SIGINT, &sigint_action, NULL);
	sigaction(SIGTERM, &sigint_action, NULL);

	while (1) avr_run(avr);
}

