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
 * DATE STARTED : 18/11/2015
 *
 *  Create a virtual device, activate auto-repeat and
 *  and check that auto repeat is working
 *
 */

#include <linux/input.h>
#include <linux/uinput.h>
#include <fnmatch.h>

#include "test.h"
#include "safe_macros.h"
#include "lapi/fcntl.h"

#define NB_TEST 100
#define VIRTUAL_DEVICE "virtual-device-ltp"

static void setup(void);
static void send_information(void);
static int check_information(void);
static void cleanup(void);

static int fd;
static struct uinput_user_dev uidev;
static struct input_event ev;
static char event[256];

char *TCID = "uinput01";

int main(int ac, char **av)
{
	int lc;
	int pid;

	tst_parse_opts(ac, av, NULL, NULL);

	for (lc = 0; lc < TEST_LOOPING(lc); ++lc) {

		setup();
		pid = tst_fork();

		if (!pid)
			send_information();
		else {
			if (check_information())
				tst_resm(TFAIL, "Wrong data received in %s",
								event);
			else
				tst_resm(TPASS, "Data received in %s", event);
		}
	}
	tst_exit();
}

void setup(void)
{
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

	if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_EVBIT, EV_REP) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_KEYBIT, KEY_X) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_KEYBIT, KEY_Y) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_KEYBIT, KEY_ENTER) < 0)
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

void send_information(void)
{
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = KEY_X;
	ev.value = 10;
	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		tst_brkm(TBROK, cleanup, "write failed");

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		tst_brkm(TBROK, cleanup, "write failed");
	usleep(100);

	sleep(1);

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = KEY_Y;
	ev.value = 10;
	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		tst_brkm(TBROK, cleanup, "write failed");

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = KEY_ENTER;
	ev.value = 1;
	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		tst_brkm(TBROK, cleanup, "write failed");

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		tst_brkm(TBROK, cleanup, "write failed");
	cleanup();
}

int check_information(void)
{
	int rd;
	uint i;
	char buf[128];
	
	i = 0;
	rd = read(STDIN_FILENO, buf, sizeof(buf));
	if (rd < 1)
		return 1;
	while (buf[i] != 'y') {
		if (buf[i] != 'x')
			return 1;
		i++;
	}

	if (i <= 1)
		return 1;

	while (i < strlen(buf)) {
		if (buf[i] == 'x')
			return 1;
		i++;
	}
	
	return 0;
}

void cleanup(void)
{
	if (ioctl(fd, UI_DEV_DESTROY) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	SAFE_CLOSE(NULL, fd);
}
