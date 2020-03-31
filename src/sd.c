#include <stddef.h>
#include <assert.h>
#include <sys/mman.h>

#include <simavr/sim_irq.h>

#include "sd.h"

#define true 1
#define false 0

#ifdef SD_DEBUG
#include <stdio.h>
#define DEBUG(msg, ...) fprintf(stderr, "SD_DEBUG: " msg "\n", ##__VA_ARGS__);
#else
#define DEBUG(msg, ...)
#endif

const char *irq_names[] = {
	[SD_IRQ_MOSI] = "SD_IRQ_MOSI",
	[SD_IRQ_MISO] = "SD_IRQ_MISO",
	[SD_IRQ_CS] = "SD_IRQ_CS"
};

// Reset the card. Like a hard power cycle.
void sd_reset(sd_t *sd) {
	DEBUG("Reset!");
	// do NOT reset sd->cs -- that's controlled by interrupts
	sd->cmd_idx = 0;
	sd->send_idx = 0;
	sd->send_len = 0;
	sd->state = SD_STATE_BOOT;
	sd->next_acmd = false;
}

// reset the card and send an empty byte over SPI. For critical errors that
// occur while processing a byte (i.e, most of them).
static void error_reset(sd_t *sd) {
	sd_reset(sd);
	avr_raise_irq(sd->irq + SD_IRQ_MISO, 0x00);
}

static void enqueue_r1(sd_t *sd) {
	sd->send[0] = 0b00000000;
	sd->send_len = 1;
}

static void enqueue_idle_r1(sd_t *sd) {
	sd->send[0] = 0b00000001;
	sd->send_len = 1;
}

static void enqueue_r2(sd_t *sd) {
	sd->send[0] = 0b00000000;
	sd->send[1] = 0b00000000;
	sd->send_len = 2;
}

static void enqueue_r3(sd_t *sd) {
	enqueue_r1(sd);
	// set OCR as to emulate a standard-speed SD card that supports any
	// voltage.
	sd->send[1] = 0b10000001;
	sd->send[2] = 0b11111111;
	sd->send[3] = 0b00000000;
	sd->send[4] = 0b00000000;
	sd->send_len = 5;
}

// ACK after every data block written
static void enqueue_data_response(sd_t *sd) {
	sd->send[0] = 0b00000101;
	sd->send_len = 1;
}

static void enqueue_illegal_command(sd_t *sd) {
	// TODO: idle bit
	sd->send[0] = 0b0000100;
	sd->send_len = 1;
}

// ocr: 0x00
//      

// Called after a complete command was received. Analyzes the command and places
// an appropriate response in the output buffer. Also changes the SD card object
// as necessary (eg, changing state).
static void enqueue_response(sd_t *sd) {
	// TODO: verify CRC, basic format of received message.
	/* if (sd->cmd[0] & (1 << 7) || sd->cmd[0] ^ (1 << 6)) */

	sd->send_idx = 0;
	unsigned char command_index = sd->cmd[0] & 0b00111111;
	// TODO: make sure thing scan only happen during the correct states. Eg,
	// we can't do a read while in the boot state!
	if (!sd->next_acmd) {
		DEBUG("Received normal command %d", command_index);
		switch (command_index) {
		case 0:
			sd_reset(sd);
			sd->state = SD_STATE_SPI;
			enqueue_idle_r1(sd);
			break;
		case 13:
			// SEND_STATUS
			enqueue_r2(sd);
			break;
		case 55:
			sd->next_acmd = true;
			enqueue_r1(sd);
			break;
		case 58:
			enqueue_r3(sd);
			break;
		default:
			enqueue_illegal_command(sd);
		}
	} else {
		DEBUG("Received ACMD command %d", command_index);
		sd->next_acmd = false;
		switch (command_index) {
		case 41:
			enqueue_r1(sd);
			// part of bootup process
			if (sd->state == SD_STATE_SPI) {
				sd->state = SD_STATE_IDLE;
			}
			break;
		default:
			enqueue_illegal_command(sd);
		}
	}

}

static void spi_hook(avr_irq_t *irq, uint32_t value, void *param) {
	sd_t *sd = (sd_t *)param;

	// ignore unless chip selected
	if (!sd->cs) return;

	// ignore empty bytes in-between commands
	if (value ^ 0xFF || sd->cmd_idx) {
		// If we were in the middle of something, reset the bloody card!
		// Those maggots!
		if (sd->send_idx < sd->send_len) {
			return error_reset(sd);
		}
		sd->cmd[sd->cmd_idx] = value;
		sd->cmd_idx++;
	}

	if (sd->cmd_idx == COMMAND_LENGTH) {
		// Since all commands are longer than all responses, 
		sd->cmd_idx = 0;
		enqueue_response(sd);
		// this 1-byte delay *is* necessary. You can't be sending the
		// response to a command *while* it's being sent!
		return avr_raise_irq(sd->irq + SD_IRQ_MISO, 0xFF);
	}

	// finally, send a byte if we have one.
	if (sd->send_idx < sd->send_len) {
		DEBUG("Send byte: %d", sd->send[sd->send_idx]);
		avr_raise_irq(sd->irq + SD_IRQ_MISO, sd->send[sd->send_idx++]);
	} else {
		avr_raise_irq(sd->irq + SD_IRQ_MISO, 0xFF);
	}
}

static void cs_hook(avr_irq_t *irq, uint32_t value, void *param) {
	sd_t *sd = (sd_t *)param;
	sd->cs = !value;

	if (value) {
		DEBUG("Chip deselected");
	} else {
		DEBUG("Chip selected");
	}
}

void sd_init(sd_t *sd, avr_t *avr, char *image_path) {
	sd->irq = avr_alloc_irq(&avr->irq_pool, 0, 3, irq_names);
	avr_irq_register_notify(sd->irq + SD_IRQ_MOSI, &spi_hook, sd);
	avr_irq_register_notify(sd->irq + SD_IRQ_CS, &cs_hook, sd);

	// because of this, it's important that the SD card is initialized and
	// connected before the first avr_run()
	sd->cs = false;
	sd_reset(sd);
}
