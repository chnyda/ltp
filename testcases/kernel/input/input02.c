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
#include "input_helper.h"

#define NB_TEST 100

static void setup(void);
static void send_information(void);
static int check_information(void);
static void cleanup(void);

static int fd;
static int fd2;

char *TCID = "input02";

int main(int ac, char **av)
{
	int lc;
	int pid;

	tst_parse_opts(ac, av, NULL, NULL);

	for (lc = 0; lc < TEST_LOOPING(lc); ++lc) {

		setup();
		pid = tst_fork();
		fd2 = setup_read();

		if (!pid) {
			send_information();
		} else {
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
	tst_require_root();

	fd = open_uinput();

	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_KEY);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, BTN_LEFT);
	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_REL);
	SAFE_IOCTL(NULL, fd, UI_SET_RELBIT, REL_X);
	SAFE_IOCTL(NULL, fd, UI_SET_RELBIT, REL_Y);

	create_device(fd);
	sleep(1);
}

static void send_information(void)
{
	int nb;

	SAFE_IOCTL(NULL, fd2, EVIOCGRAB, 1);
	tst_resm(TINFO, "The virtual device was grabbed");

	sleep(1);

	for (nb = 0; nb < NB_TEST; ++nb) {
		send_rel_move(fd, 10, 1);
		usleep(100);
	}

	cleanup();
}

static int check_information(void)
{
	int rd;
	struct input_event iev[64];

	rd = read(fd2, iev, sizeof(struct input_event) * 64);

	SAFE_CLOSE(NULL, fd2);
	return rd > 0;
}

static void cleanup(void)
{
	SAFE_IOCTL(NULL, fd, UI_DEV_DESTROY, NULL);
	SAFE_CLOSE(NULL, fd);
}
