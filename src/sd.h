#ifndef SD_H
#define SD_H

#include <stddef.h>
#include <stdbool.h>

#include <simavr/sim_avr.h>

// Maximum number of bytes in a command or response
#define COMMAND_LENGTH 6

enum {
	SD_IRQ_MOSI,
	SD_IRQ_MISO,
	SD_IRQ_CS
};

enum {
	SD_STATE_BOOT,		// Card just started up.
	SD_STATE_SPI,		// Card is in SPI mode.
	SD_STATE_IDLE,
};

typedef struct sd_t {
	avr_irq_t *irq;
	void *mass;		// SD card block data, from mmap
	int state;
	uint32_t capacity;
	bool cs;		// True when CS is low; this is logical.
	unsigned char cmd[COMMAND_LENGTH]; // Command read in progress
	unsigned char cmd_idx;	// Which byte of command we will read next
	unsigned char send[COMMAND_LENGTH]; // Command write in progress
	unsigned char send_idx;
	unsigned char send_len; // The total number of bytes to send

	bool next_acmd;		// Is the next command application-specific?
				// i.e, was the last command CMD55
} sd_t;

// Initialize an SD card. You allocate the structure memory. The image_path is
// the file the SD card maps to. *You* must create this file before calling
// sd_init. Its size is the size of the SD card. A blank file can be created
// with `truncate -s 2G file.img`, for example. Creating a "sparse" file in this
// manner will use negligible disk space.
void sd_init(sd_t *sd, avr_t *avr, char *image_path);

// Free the memory used by the fields of an SD card structure. does not free the
// structure itself.
void sd_free(sd_t *);

// perform a power cycle
void sd_reset(sd_t *sd);

#endif // SD_H
