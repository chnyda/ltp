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
static void cleanup(void);

static int fd;
static int fd2;

char *TCID = "input05";

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
			cleanup();
		} else {
			if (check_no_data(fd2))
				tst_resm(TPASS, "No data received in eventX");
			else
				tst_resm(TFAIL, "Data received in eventX");
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

	create_device(fd);
}

static void send_information(void)
{
	int nb;

	for (nb = 0; nb < NB_TEST; ++nb)
		send_rel_move(fd, X_VALUE, Y_VALUE);
}

static void cleanup(void)
{
	destroy_device(fd);
}
