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
 * DATE STARTED : 10/21/2015
 *
 *  Create a virtual device (mouse), send events to /dev/uinput
 *  and check that the events are not received in /dev/input/eventX
 *  because the device is grabbed by another process
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
static void setup_read(void);
static int check_information(void);
static void cleanup(void);
static void find_event(DIR *d, char *namedir1);

static int fd;
static int fd2;
static struct uinput_user_dev uidev;
static struct input_event ev;
static char event[256];

char *TCID = "uinput02";

int main(int ac, char **av)
{
	int lc;
	int pid;

	tst_parse_opts(ac, av, NULL, NULL);

	for (lc = 0; lc < TEST_LOOPING(lc); ++lc) {

		setup();
		pid = tst_fork();
		setup_read();

		if (!pid)
			send_information();
		else {
			if (check_information())
				tst_resm(TFAIL, "Data received in %s", event);
			else
				tst_resm(TPASS, "No data received in %s",
								event);
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

	if (fd < 0 && errno == ENOENT)
		tst_brkm(TCONF, NULL, "unable to find and open uinput");
	else if (fd < 0)
		tst_brkm(TBROK, NULL, "open failed");

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");

	if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_RELBIT, REL_X) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
	if (ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)
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
	sleep(2);
}

void send_information(void)
{
	int nb;

	if (ioctl(fd2, EVIOCGRAB, 1))
		tst_brkm(TBROK, cleanup, "ioctl failed");
	tst_resm(TINFO, "The virtual device was grabbed");

	sleep(2);

	for (nb = 0; nb < NB_TEST; ++nb) {
		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_SYN;
		ev.code = 0;
		ev.value = 0;
		if (write(fd, &ev, sizeof(struct input_event)) < 0)
			tst_brkm(TBROK, cleanup, "write failed");
		usleep(100);
	}

	sleep(1);
	fd2 = SAFE_OPEN(NULL, event, O_RDONLY);
	cleanup();
}

void setup_read(void)
{
	DIR *d;
	char namedir1[1024];
	struct dirent *dp;

	memset(event, 0, sizeof(event));

	d = opendir("/sys/devices/virtual/input/");
	while ((dp = readdir(d))) {
		if (!fnmatch("input*", dp->d_name, 0)) {
			memset(namedir1, 0, sizeof(namedir1));
			strcat(namedir1, "/sys/devices/virtual/input/");
			strcat(namedir1, dp->d_name);
			if (closedir(d) < 0)
				tst_brkm(TBROK, NULL, "closedir failed");
			d = opendir(namedir1);
			find_event(d, namedir1);
			if (strlen(event) > 0)
				break;
		}
	}

	closedir(d);
	fd2 = SAFE_OPEN(NULL, event, O_RDONLY);
}

void find_event(DIR *d, char *namedir1)
{
	char namedir2[1024];
	char buf[256];
	struct dirent *dp;
	int fd3;
	int nb;

	while ((dp = readdir(d)))
		if (!fnmatch("event*", dp->d_name, 0)) {
			memset(namedir2, 0, sizeof(namedir2));
			strcpy(namedir2, namedir1);
			strcat(namedir2, "/name");
			fd3 = open(namedir2, O_RDONLY);
			if (fd3 > 0) {
				nb = read(fd3, &buf, sizeof(VIRTUAL_DEVICE));
				if (nb < 0)
					tst_brkm(TBROK, NULL, "read failed");
				close(fd3);
				if (!strncmp(buf, VIRTUAL_DEVICE,
					strlen(VIRTUAL_DEVICE))) {
					memset(event, 0, 256);
					strcat(event, "/dev/input/");
					strcat(event, dp->d_name);
					return;
				}
			}
		}
}

int check_information(void)
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

void cleanup(void)
{
	if (ioctl(fd, UI_DEV_DESTROY) < 0)
		tst_brkm(TBROK, NULL, "ioctl failed");
}
