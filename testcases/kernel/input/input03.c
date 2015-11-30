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
 *  and check that the events are well received in /dev/input/mice
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
#define X_VALUE 10
#define Y_VALUE 10

static void setup(void);
static void send_information(void);
static int check_information(void);
static void cleanup(void);

static int fd;
static int fd2;

char *TCID = "input03";

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
			cleanup();
		} else {
			if (check_information())
				tst_resm(TFAIL, "Wrong data received");
			else
				tst_resm(TPASS,
					"Data received in /dev/input/mice");
		}
	}
	tst_exit();
}

static void setup(void)
{
	tst_require_root();

	fd = open_uinput();

	setup_mouse_events(fd);

	create_device(fd);
}

static void send_information(void)
{
	int nb;

	for (nb = 0; nb < NB_TEST; ++nb) {

		send_rel_move(fd, X_VALUE, Y_VALUE);
		usleep(100);
	}
}

static int check_information(void)
{
	int nb;
	int fail;
	int rd;
	char buf[3];

	fd2 = SAFE_OPEN(NULL, "/dev/input/mice", O_RDONLY);
	fail = 0;
	nb = 0;

	while (nb < NB_TEST && !fail) {
		memset(buf, 0, 3);
		rd = read(fd2, buf, 3);

		if (rd < 3)
			fail = 1;
		if (buf[1] != X_VALUE || buf[2] != -Y_VALUE || buf[0] != 40)
			fail = 1;
		nb++;
	}

	SAFE_CLOSE(NULL, fd2);
	return fail;
}

static void cleanup(void)
{
	destroy_device(fd);
}
