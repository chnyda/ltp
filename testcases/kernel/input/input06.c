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
static int fd2;

char *TCID = "input06";

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
			if (!check_information())
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
	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_REP);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, KEY_X);

	create_device(fd);
	send_event(fd, EV_REP, REP_DELAY, 25);
	send_event(fd, EV_REP, REP_PERIOD, 25);

}

static void send_information(void)
{
	send_event(fd, EV_KEY, KEY_X, 1);
	send_event(fd, EV_SYN, 0, 0);

	/* sleep to keep the key pressed for some time (auto-repeat) */
	usleep(1000);

	send_event(fd, EV_KEY, KEY_X, 0);
	send_event(fd, EV_SYN, 0, 0);

	cleanup();
}

static int check_event(struct input_event *iev, int event, int code, int value)
{
	return iev->type == event && iev->code == code && iev->value == value;
}

static int check_information(void)
{
	struct input_event iev[64];
	uint i;
	int nb;
	int rd;

	i = 0;
	nb = 0;

	rd = read(fd2, iev, sizeof(iev));

	while (iev[i].type != EV_KEY)
		i++;

	if (check_event(&iev[i], EV_KEY, KEY_X, 1))
		i = i + 1;

	while (!check_event(&iev[i], EV_KEY, KEY_X, 0)) {

		if (iev[i].type != EV_SYN && !check_event(&iev[i], EV_KEY, KEY_X, 2)) {
			tst_resm(TINFO, "Didn't receive EV_KEY KEY_X with value 2");
			break;
		}
		i++;
		nb++;

		if (i == rd / sizeof(struct input_event)) {
			i = 0;
			rd = read(fd2, iev, sizeof(iev));
			if (rd < 0)
				break;
		}
	}

	SAFE_CLOSE(NULL, fd2);

	return nb > 0 && check_event(&iev[i], EV_KEY, KEY_X, 0);
}

static void cleanup(void)
{
	destroy_device(fd);
}
