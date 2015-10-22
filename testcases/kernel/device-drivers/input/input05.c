/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

 /*
 * AUTHOR   : Cedric Hnyda
 * DATE STARTED : 11/16/2015
 *
 *  Create a virtual device (mouse), send events to /dev/uinput
 *  and check that the events are not received in /dev/input/eventX
 *  because it is not possible to sent such events
 *
 */

#include <linux/input.h>
#include <linux/uinput.h>
#include <fnmatch.h>

#include "test.h"
#include "safe_macros.h"
#include "lapi/fcntl.h"
#include "input_helper.h"

#define X_VALUE 10
#define Y_VALUE 10

#define NB_TEST 100

static void setup(void);
static void send_information(void);
static int check_information(void);
static void cleanup(void);

static int fd;
static int fd2;

char *TCID = "uinput05";

int main(int ac, char **av)
{
	int lc;
	int pid;

	tst_parse_opts(ac, av, NULL, NULL);

	for (lc = 0; lc < TEST_LOOPING(lc); ++lc) {

		setup();
		pid = tst_fork();
		fd2 = setup_read();

		if (!pid)
			send_information();
		else {
			if (check_information())
				tst_resm(TFAIL, "Data received in eventX");
			else
				tst_resm(TPASS, "No data received in eventX");
		}
	}
	tst_exit();
}

static void setup(void)
{
	struct uinput_user_dev uidev;

	tst_require_root();

	fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0 && errno == ENOENT)
		fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0)
		tst_brkm(TCONF, NULL, "unable to find and open uinput");
	else if (fd < 0)
		tst_brkm(TBROK, NULL, "open failed");

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, VIRTUAL_DEVICE);
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;

	if (write(fd, &uidev, sizeof(uidev)) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");

	if (ioctl(fd, UI_DEV_CREATE) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	sleep(1);
}

static void send_information(void)
{
	int nb;
	struct input_event ev;

	sleep(1);

	for (nb = 0; nb < NB_TEST; ++nb) {

		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_REL;
		ev.code = REL_X;
		ev.value = X_VALUE;
		if (write(fd, &ev, sizeof(struct input_event)) < 0)
			tst_brkm(TBROK, cleanup, "write failed");

		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_REL;
		ev.code = REL_Y;
		ev.value = Y_VALUE;
		if (write(fd, &ev, sizeof(struct input_event)) < 0)
			tst_brkm(TBROK, cleanup, "write failed");

		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_SYN;
		ev.code = 0;
		ev.value = 0;
		if (write(fd, &ev, sizeof(struct input_event)) < 0)
			tst_brkm(TBROK, cleanup, "write failed");
		usleep(100);

	}

	sleep(1);
	cleanup();
}

static int check_information(void)
{
	int nb;
	int rd;
	struct input_event iev[64];

	nb = 0;

	while (nb == 0) {
		rd = read(fd2, iev, sizeof(struct input_event) * 64);
		if (rd == 0)
			break;
		nb++;
	}

	return nb == 0;
}

static void cleanup(void)
{
	if (ioctl(fd, UI_DEV_DESTROY) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	SAFE_CLOSE(NULL, fd);
}
