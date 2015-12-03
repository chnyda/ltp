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
 *  Create a virtual device, activate auto-repeat and
 *  and check that auto repeat is working
 *
 */

#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/kd.h>

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
static int caps;

char *TCID = "input06";

int main(int ac, char **av)
{
	int lc;
	int pid;

	tst_parse_opts(ac, av, NULL, NULL);

	for (lc = 0; lc < TEST_LOOPING(lc); ++lc) {

		setup();
		pid = tst_fork();

		if (!pid) {
			send_information();
		} else {
			if (check_information())
				tst_resm(TFAIL,
					"Wrong data received in eventX");
			else
				tst_resm(TPASS, "Data received in eventX");
		}
	}
	tst_exit();
}

static void setup(void)
{
	tst_require_root();

	fd = open_uinput();

	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_KEY);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, KEY_X);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, KEY_Y);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, KEY_ENTER);

	create_device(fd);
}

static void send_information(void)
{
	send_event(fd, EV_KEY, KEY_X, 10);
	send_event(fd, EV_SYN, 0, 0);

	/* sleep to keep the key pressed for some time (auto-repeat) */
	sleep(1);

	send_event(fd, EV_KEY, KEY_Y, 10);
	send_event(fd, EV_KEY, KEY_ENTER, 10);
	send_event(fd, EV_SYN, 0, 0);

	cleanup();
}

static int check_information(void)
{
	int rd;
	uint i;
	char buf[128];

	i = 1;
	rd = read(STDIN_FILENO, buf, sizeof(buf));
	if (rd < 1)
		return 1;

	if (buf[0] == 'X')
		caps = 32;
	while (buf[i] != 'y' - caps) {

		if (buf[i] != 'x' - caps) {
			tst_resm(TINFO, "Unexpected input %c expected %c",
				buf[i], 'x' - caps);
			return 1;
		}

		i++;
	}

	if (i <= 1) {
		tst_resm(TINFO, "autorepeat didn't work");
		return 1;
	}

	if (i != strlen(buf) - 2) {
		tst_resm(TINFO,
			"the buffer is filled with unwanted chararacters");
		return 1;
	}

	return 0;
}

static void cleanup(void)
{
	destroy_device(fd);
}
