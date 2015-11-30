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
 *  Create a virtual device (mouse), send empty events to /dev/uinput
 *  and check that the events are not received in /dev/input/mice
 *
 */

#include <linux/input.h>

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

char *TCID = "input04";

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
				tst_resm(TPASS,
					"No data received in /dev/input/mice");
			else
				tst_resm(TFAIL,
					"Data received /dev/input/mice");
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

	for (nb = 0; nb < NB_TEST; ++nb)
		send_rel_move(fd, 0, 0);
}

static int check_information(void)
{
	int rv;
	int rd;
	int fd2;
	char buf[3];
	struct timeval timeout;
	fd_set set;

	fd2 = SAFE_OPEN(NULL, "/dev/input/mice", O_RDWR);

	do {
		FD_ZERO(&set);
		FD_SET(fd2, &set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;
		memset(buf, 0, 3);
		rv = select(fd2 + 1, &set, NULL, NULL, &timeout);
		if (rv < 0)
			tst_brkm(TBROK, NULL, "select failed");

		if (rv > 0) {
			rd = read(fd2, buf, 3);

			if (rd == 3 && buf[1] == 0
				&& buf[2] == 0 && buf[0] == 40) {
				tst_resm(TINFO, "Received empty movements");
				rv = 1;
				break;
			}
		}
	} while (rv > 0);

	SAFE_CLOSE(NULL, fd2);
	return rv == 0;
}

static void cleanup(void)
{
	destroy_device(fd);
}
