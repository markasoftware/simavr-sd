#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/avr_ioport.h>
#include <simavr/avr_spi.h>

#include "sd.h"

int main() {
	avr_t *avr;
	elf_firmware_t firmware;
	elf_read_firmware("CardInfo.elf", &firmware);
	if (!(avr = avr_make_mcu_by_name("atmega328p"))) {
		fprintf(stderr, "Error creating avr object\n");
		return 1;
	}
	avr_init(avr);
	avr_load_firmware(avr, &firmware);

	sd_t sd;
	if (sd_init(&sd, avr, "CardInfo.img") != 0) {
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

	while (1) {
		avr_run(avr);
	}
}
