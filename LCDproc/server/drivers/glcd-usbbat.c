/** \file server/drivers/glcd-usbbat.c
 * Driver for the USB-BAT 122x32 from elv.de.  Based on picolcdgfx
 * driver by Scott Meahrg.
 */

/*-
 * Copyright (c) 2009 Nicu Pavel <napvel@mini-box.com>
 * Copyright (c) 2012 Scott Meahrg <ssmeharg@gmail.com>
 * Copyright (c) 2014 c64emulator <john.doe@neverbox.com>
 *
 * This file is released under the GNU General Public License. Refer to the
 * COPYING file distributed with this package.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _POSIX_C_SOURCE	199309L

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <usb.h>

#include "lcd.h"
#define DEBUG
#include "report.h"
#include "glcd-low.h"

#define usbBAT_VENDOR  			0x18ef
#define usbBAT_DEVICE  			0xe01a

/*   Messageframe (Note: outputLen= 64 Byte -- inputLen= 8 Byte)
*
*   | HID-Report-ID |  Byte-Count 1) |     CMD-ID     |   CMD/Data 2)  |
*   +---------------+----------------+----------------+----------------+
*   |     (out:0x01)|       0x01-0x3E|                | 61 Byte        |
*   | 1 Byte        | 1 Byte         | 1 Byte         |                |
*   |      (in:0x02)|       0x01-0x06|                |  5 Byte        |
*   +---------------+----------------+----------------+----------------+
*
*     1) only CMD-ID and Datafields count
*     2) unused bytes are filled with zeroes (0x00)
*/
#define USBBAT_MAX_DATA_LEN_OUT		64
#define USBBAT_MAX_DATA_LEN_IN	 	8

/* USB-BAT Graphics Commands */
#define USBBAT_REPORTID_OUT		0x01
#define USBBAT_OUT_BACKLIGHT_ON		0xf1
#define USBBAT_OUT_BACKLIGHT_OFF	0xf2
#define USBBAT_OUT_KEY_POLL_MODE	0xf7
#define USBBAT_OUT_KEY_POLL_MODE_STD	0x01
#define USBBAT_OUT_KEY_POLL_MODE_EXT	0x02
#define USBBAT_OUT_INIT_DISPLAY		0xdc

#define USBBAT_REPORTID_IN		0x02
#define USBBAT_IN_KEY_STATE		0xf5

/* Display properties */
#define USBBAT_WIDTH			122
#define USBBAT_HEIGHT			32
#define USBBAT_CONTRLS			2

/* Default for user configurable properties */
#define USBBAT_DEF_KEYTIMEOUT		125
#define USBBAT_DEF_INVERTED		0

/* Error codes */
#define USB_ERROR_NONE      0
#define USB_ERROR_ACCESS    -1
#define USB_ERROR_NOTFOUND  -2
#define USB_ERROR_IO        -5
#define USB_ERROR_BUSY      -16

/* HID report types */
#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3

/** Private data for the usbbat connection type */
typedef struct glcd_usbbat_data {
	usb_dev_handle *lcd;
	unsigned char backlightstate;
	unsigned char inverted;
	int keytimeout;
	unsigned char *backingstore;
} CT_usbbat_data;

/* Prototypes */
void glcd_usbbat_blit(PrivateData *p);
unsigned char glcd_usbbat_pollkeys(PrivateData *p);
void glcd_usbbat_set_backlight(PrivateData *p, int state);
void glcd_usbbat_close(PrivateData *p);

/* Local functions */
static int
usbbat_read(usb_dev_handle * lcd, unsigned char *data, int size, int timeout)
{
	int bytesRcv;

	bytesRcv = usb_interrupt_read(lcd, USB_ENDPOINT_IN + 1, (char *) data, USBBAT_MAX_DATA_LEN_IN, timeout);

        if (data[0] == USBBAT_REPORTID_IN && data[1] == size && bytesRcv > 0) {
/*
		fprintf(stderr, "GLCD/usbbat: *** successfully receiving data ***\n");
		fprintf(stderr, "GLCD/usbbat: %d bytes expected, MSGlen: %d, bytesRcv: %d, received data: ",
                                 size, data[1], bytesRcv);
		for (int i = 0; i < USBBAT_MAX_DATA_LEN_IN; i ++) fprintf(stderr, " 0x%02x", data[i]);
		fprintf(stderr, "\n");
*/
		return bytesRcv;
	}
        else {
/*
                fprintf(stderr, "GLCD/usbbat: error receiving data\n");
 		fprintf(stderr, "GLCD/usbbat: %d bytes expected, MSGlen: %d, bytesRcv: %d, received data: ",
                                 size, data[1], bytesRcv);
		for (int i = 0; i < USBBAT_MAX_DATA_LEN_IN; i ++) fprintf(stderr, " 0x%02x", data[i]);
		fprintf(stderr, "\n");
*/
                return USB_ERROR_IO;
        }

}

static int
usbbat_write(usb_dev_handle * lcd, unsigned char *data)
{
	int bytesSent;
/*
	fprintf(stderr, "GLCD/usbbat: sending data: ");
	for (int i = 0; i < USBBAT_MAX_DATA_LEN_OUT; i ++) fprintf(stderr, " 0x%02x", data[i]);
	fprintf(stderr, "\n");
*/
	bytesSent = usb_interrupt_write(lcd, USB_ENDPOINT_OUT + 1, (char *) data, USBBAT_MAX_DATA_LEN_OUT, 1000);

	if (bytesSent != USBBAT_MAX_DATA_LEN_OUT) {
		if (bytesSent < 0) fprintf(stderr, "Error sending message: %s", usb_strerror());
		return USB_ERROR_IO;
	}

        return bytesSent;

}

static void
usbbat_clearPage(usb_dev_handle * lcd, int pg)
{

   	if (pg < 0 || pg > 7)
     		return;

	/* send command + data */
	unsigned char cmd[USBBAT_MAX_DATA_LEN_OUT] = {USBBAT_REPORTID_OUT};

        memset(cmd, 0x00, USBBAT_MAX_DATA_LEN_OUT);	// clear buffer

        cmd[0] = USBBAT_REPORTID_OUT;
        cmd[1] = 0x02;					// cmdlen = 2 byte (cmd & page)
        cmd[2] = 0xDA;					// cmd
        cmd[3] = pg;					// page number

 	usbbat_write(lcd, cmd);

}

/**
 * API: Initialize the connection type driver.
 * \param drvthis  Pointer to driver structure.
 * \retval 0       Success.
 * \retval <0      Error.
 */
int
glcd_usbbat_init(Driver *drvthis)
{
	PrivateData *p = (PrivateData *) drvthis->private_data;
	CT_usbbat_data *ct_data;

	struct usb_bus *busses, *bus;
	struct usb_device *dev;
	char driver[1024];
	char product[1024];
	char manufacturer[1024];
	char serialnumber[1024];
        /* send command + data */
        unsigned char cmd[USBBAT_MAX_DATA_LEN_OUT] = {USBBAT_REPORTID_OUT};
	int ret;

	report(RPT_INFO, "GLCD/usbbat: intializing");

	/* Set up connection type low-level functions */
	p->glcd_functions->blit = glcd_usbbat_blit;
	p->glcd_functions->close = glcd_usbbat_close;
	p->glcd_functions->poll_keys = glcd_usbbat_pollkeys;
	p->glcd_functions->set_backlight = glcd_usbbat_set_backlight;

	/* Allocate memory structures */
	ct_data = (CT_usbbat_data *) calloc(1, sizeof(CT_usbbat_data));
	if (ct_data == NULL) {
		report(RPT_ERR, "GLCD/usbbat: error allocating connection data");
		return -1;
	}
	p->ct_data = ct_data;

	/* Fix display size to 122x32 */
	p->framebuf.layout = FB_TYPE_VPAGED;
	p->framebuf.px_width = USBBAT_WIDTH;
	p->framebuf.px_height = USBBAT_HEIGHT;

	/* Since the display is fixed to 122x32 we have to recalculate. */
	p->framebuf.size = (USBBAT_HEIGHT / 8) * USBBAT_WIDTH;

	ct_data->backingstore = malloc(p->framebuf.size);
	if (ct_data->backingstore == NULL) {
		report(RPT_ERR, "GLCD/usbbat: unable to allocate backing store");
		return -1;
	}

	/* framebuf is initialized with 0x00. Initialize the backingstore with
	 * 0xFF so the first call to _blit will draw the entire screen.
	 * */
	memset(ct_data->backingstore, 0xFF, p->framebuf.size);

	/* Get key timeout */
	ct_data->keytimeout = drvthis->config_get_int(drvthis->name,
						      "usbbat_KeyTimeout", 0,
						      USBBAT_DEF_KEYTIMEOUT);

	/* Get inverted option */
	if (drvthis->config_get_bool(drvthis->name, "usbbat_Inverted", 0, USBBAT_DEF_INVERTED))
		ct_data->inverted = 0xFF;
	else
		ct_data->inverted = 0;

	ct_data->lcd = NULL;

	report(RPT_DEBUG, "GLCD/usbbat: scanning for USB-BAT 122x32...");

	/* The libusb 0.1 way */
	/* Try to find usbbat device */
	usb_init();
	usb_find_busses();
	usb_find_devices();
	busses = usb_get_busses();

	for (bus = busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == usbBAT_VENDOR) &&
			    (dev->descriptor.idProduct == usbBAT_DEVICE)) {
				report(RPT_DEBUG,
				       "GLCD/usbbat: found USB-BAT on bus %s device %s",
				       bus->dirname, dev->filename);

				ct_data->lcd = usb_open(dev);

				ret = usb_get_driver_np(ct_data->lcd, 0, driver, sizeof(driver));

				if (ret == 0) {
					report(RPT_DEBUG,
					       "GLCD/usbbat: interface 0 already claimed by '%s'",
					       driver);
					report(RPT_DEBUG,
					       "GLCD/usbbat: attempting to detach driver...");
					if (usb_detach_kernel_driver_np(ct_data->lcd, 0) < 0) {
						report(RPT_ERR,
						       "GLCD/usbbat: usb_detach_kernel_driver_np() failed!");
						return -1;
					}
				}

				usb_set_configuration(ct_data->lcd, 1);
				//usleep(100);
				nanosleep((struct timespec[]){{0, 100000000}}, NULL);

				if (usb_claim_interface(ct_data->lcd, 0) < 0) {
					report(RPT_ERR,
					       "GLCD/usbbat: usb_claim_interface() failed!");
					return -1;
				}

				usb_set_altinterface(ct_data->lcd, 0);
				usb_get_string_simple(ct_data->lcd, dev->descriptor.iProduct,
						      product, sizeof(product));
				usb_get_string_simple(ct_data->lcd, dev->descriptor.iManufacturer,
						      manufacturer, sizeof(manufacturer));
				usb_get_string_simple(ct_data->lcd, dev->descriptor.iSerialNumber,
						      serialnumber, sizeof(serialnumber));

				report(RPT_INFO,
				       "GLCD/usbbat: Manufacturer='%s' Product='%s' SerialNumber='%s'",
				       manufacturer, product, serialnumber);

				memset(cmd,0x00, USBBAT_MAX_DATA_LEN_OUT);
				cmd[0] = USBBAT_REPORTID_OUT;
				cmd[1] = 0x02;
				cmd[2] = USBBAT_OUT_KEY_POLL_MODE;
				cmd[3] = USBBAT_OUT_KEY_POLL_MODE_STD;
				cmd[4] = 0x00;

 				usbbat_write(ct_data->lcd, cmd);

				debug(RPT_DEBUG, "GLCD/usbbat: init() done");

				return 0;
			}
		}
	}

	report(RPT_ERR, "GLCD/usbbat: could not find a USB-BAT");
	return -1;
}

/**
 * API: Write the framebuffer to the display
 * \param p  Pointer to glcd driver's private date structure.
 */
/* Framebuffer
*
* The USB-BAT uses a LCD module of 122x32 pixel (DM12232-05YT)
* This display area is tiled/paged over 2 controllers (PT6520) with 4 tiles/pages
* each. Each tile has 8x61 pixel, each data byte representing
* a 1-bit wide vertical line/column of the tile.
*
* The display can be updated at a tile granularity.
*
*         Chip 1           Chip 2
*   |0     E0      60|61    E1     121|
* --+----------------+----------------+
*  0|                |                |
*   |  Tile0/Page0   |  Tile0/Page1   |
*  7|                |                |
* --+----------------+----------------+
*   |  Tile1/Page2   |  Tile1/Page3   |
*   +----------------+----------------+
*   |  Tile2/Page4   |  Tile2/Page5   |                                     .
*   +----------------+----------------+
* 31|  Tile3/Page6   |  Tile3/Page7   |
* --+----------------+----------------+
*/

void
glcd_usbbat_blit(PrivateData *p)
{
	CT_usbbat_data *ct_data = (CT_usbbat_data *) p->ct_data;

	/* send command + data */
	unsigned char cmd[USBBAT_MAX_DATA_LEN_OUT] = {USBBAT_REPORTID_OUT};

	bool blank;
	int offset;
	unsigned char cs, tile, page, col;		/* controller and page */

	for (cs = 0; cs < USBBAT_CONTRLS; cs++) { // display controller chip select
		for (tile = 0; tile < (USBBAT_HEIGHT/8); tile++) {
			blank = true;
                        page = tile*2 +  cs;
			offset = tile * USBBAT_WIDTH + cs * (USBBAT_WIDTH / USBBAT_CONTRLS); // 61 bytes
			if (memcmp((p->framebuf.data) + offset,
                           (ct_data->backingstore) + offset,
                           (USBBAT_WIDTH / USBBAT_CONTRLS)) == 0) {
				continue; // not even one pixel was changed since last transfer
                         }

//			fprintf(stderr, "GLCD/usbbat: cs: %d, tile: %d, page: %d, offset: %d\n", cs, tile, page, offset);

                	memset(cmd,0x00, USBBAT_MAX_DATA_LEN_OUT);	// clear commandbuffer

                	cmd[0] = USBBAT_REPORTID_OUT;
                	cmd[1] = (USBBAT_WIDTH / USBBAT_CONTRLS)+1;	//cmdlen=62 byte
                	cmd[2] = 0xD0 + page;				// cmd + page number

       			for (col = 0; col < (USBBAT_WIDTH / USBBAT_CONTRLS); col++) {
				if (*((p->framebuf.data) + offset + col) > 0) blank = false;
				cmd[3 + col] = *((p->framebuf.data) + offset + col) ^ ct_data->inverted;
			}

         		if (blank)
                       		usbbat_clearPage(ct_data->lcd, page);
                	else
                        	usbbat_write(ct_data->lcd, cmd);
		}

	}

	memcpy(ct_data->backingstore, p->framebuf.data, p->framebuf.size);
}

/**
 * API: Release low-level resources.
 */
void
glcd_usbbat_close(PrivateData *p)
{
	if (p->ct_data != NULL) {
		CT_usbbat_data *ct_data = (CT_usbbat_data *) p->ct_data;

		if (ct_data->lcd != NULL) {
			usb_release_interface(ct_data->lcd, 0);
			usb_close(ct_data->lcd);
		}

		if (ct_data->backingstore != NULL)
			free(ct_data->backingstore);

		free(p->ct_data);
		p->ct_data = NULL;
	}
}
/**
 * API: Poll for any pressed keys. Converts the bitmap of keys pressed into a
 * scancode (1-6) for each pressed key.
 */

/*
*	we use keyscan mode 1 (display detects key internal and sends info about pressed key)
*	Byte 5 contains keystate
*    Keystate	Description
*    	0x01	Key of encoder wheel pressed
*    	0x02	Key A pressed
*    	0x04	Key B pressed
*    	0x10	key of encoder long pressed 	// ignored
*    	0x20	Key A long pressed		// ignored
*    	0x40	Key B long pressed		// ignored
*
* By default the keys are assigned as follows:
*   1: Up, 2: Down, 3: Left, 4: Right, 5: Enter, 6: Escape.
*
*/

unsigned char
glcd_usbbat_pollkeys(PrivateData *p)
{
	CT_usbbat_data *ct_data = (CT_usbbat_data *) p->ct_data;
	unsigned char rv = 0;
	int ret, i;
	unsigned char rx_packet[USBBAT_MAX_DATA_LEN_IN];

	ret = usbbat_read(ct_data->lcd, rx_packet, 5, ct_data->keytimeout);

	if ((ret > 0) && (rx_packet[2] == USBBAT_IN_KEY_STATE)) {
/*
                fprintf(stderr, "GLCD/usbbat: *** key press detected ***\n");
                for (int i = 0; i < USBBAT_MAX_DATA_LEN_IN; i ++) fprintf(stderr, " 0x%02x", rx_packet[i]);
                fprintf(stderr, "\n");
*/
		for (i = 0; i < 3; i++) { //check if button pressed
			if (rx_packet[3] & (1 << i)) {

				switch (i) {	// note: only one/first key is detected
					case 0:
//                				fprintf(stderr, "GLCD/usbbat: *** key press detected: wheel button (enter button) ***\n");
						rv = 5;		/* Enter */
						break;
					case 1:
//                				fprintf(stderr, "GLCD/usbbat: *** key press detected: A-key (toggle backlight) ***\n");
//                				fprintf(stderr, "GLCD/usbbat: *** toggle backlight before: %d\n", ct_data->backlightstate);
						glcd_usbbat_set_backlight(p, !ct_data->backlightstate);
//                				fprintf(stderr, "GLCD/usbbat: *** toggle backlight after: %d\n", ct_data->backlightstate);
						//rv = 7;		/* Backlight On/Off */
						break;
					case 2:
//                				fprintf(stderr, "GLCD/usbbat: *** key press detected: B-key (Escape button) ***\n");
						rv = 6;		/* Menu/Escape */
						break;
				}
			}
		}

		// check if encoder wheel was turned
		if (rx_packet[5] == 0xff) {
//			fprintf(stderr, "GLCD/usbbat: *** key press detected: wheel left (Down button) ***\n");
                        rv = 2;		/* down */
                }
		else if (rx_packet[5] == 0x01) {
//			fprintf(stderr, "GLCD/usbbat: *** key press detected: wheel right (Up button) ***\n");
			 rv = 1;	/* up */
		}
	}
	else rv = -1;
//	fprintf(stderr, "GLCD/usbbat: PollKey return: %d\n", rv);
	return rv;
}


/**
 * API: Set the backlight brightness.
 */
void
glcd_usbbat_set_backlight(PrivateData *p, int state)
{

//	fprintf(stderr, "GLCD/usbbat: set backlight to %d\n", state);

	CT_usbbat_data *ct_data = (CT_usbbat_data *) p->ct_data;
	unsigned char cmd[USBBAT_MAX_DATA_LEN_OUT];

        memset(cmd, 0x00, USBBAT_MAX_DATA_LEN_OUT);	// clear buffer

       	cmd[0] = USBBAT_REPORTID_OUT;
       	cmd[1] = 0x02;     				//cmdlen=2 byte
	cmd[2] = (state == BACKLIGHT_ON) ? USBBAT_OUT_BACKLIGHT_ON : USBBAT_OUT_BACKLIGHT_OFF;	// cmd
	cmd[3] = 0x00;
/*
	fprintf(stderr, "GLCD/usbbat: set backlight \n");
	for (int i = 0; i < USBBAT_MAX_DATA_LEN_OUT; i ++) fprintf(stderr, " 0x%02x", cmd[i]);
	fprintf(stderr, "\n");
*/
	ct_data->backlightstate = (char) state;

	usbbat_write(ct_data->lcd, cmd);
}

