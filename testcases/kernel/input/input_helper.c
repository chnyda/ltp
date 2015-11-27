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

#include <linux/input.h>
#include <linux/uinput.h>
#include <fnmatch.h>
#include <errno.h>
#include "test.h"
#include "safe_macros.h"
#include "input_helper.h"

int setup_read(void)
{
	DIR *d;
	char namedir1[1024];
	char *event;
	struct dirent *dp;
	int fd2;

	event = NULL;

	d = opendir("/sys/devices/virtual/input/");
	while ((dp = readdir(d))) {
		if (!fnmatch("input[0-9]*", dp->d_name, 0)) {
			memset(namedir1, 0, sizeof(namedir1));
			strcat(namedir1, "/sys/devices/virtual/input/");
			strcat(namedir1, dp->d_name);
			if (closedir(d) < 0)
				tst_brkm(TBROK, NULL, "closedir failed");
			d = opendir(namedir1);
			event = find_event(d, namedir1);
			if (event)
				break;
		}
	}

	closedir(d);
	fd2 = SAFE_OPEN(NULL, event, O_RDONLY);
	free(event);
	return fd2;
}

char *find_event(DIR *d, char *namedir1)
{
	char namedir2[1024];
	char buf[256];
	char *event;
	struct dirent *dp;
	int fd3;
	int nb;

	event = malloc(sizeof(char) * 256);

	while ((dp = readdir(d))) {
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
					return event;
				}
			}
		}
	}
	free(event);
	return NULL;
}

int open_uinput(void)
{
	int fd;

	fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0 && errno == ENOENT)
		fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0 && errno == ENOENT)
		tst_brkm(TCONF, NULL, "unable to find and open uinput");
	else if (fd < 0)
		tst_brkm(TBROK, NULL, "open failed");

	return fd;
}

void send_rel_move(int fd, int x, int y)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_REL;
	ev.code = REL_X;
	ev.value = x;
	SAFE_WRITE(NULL, 1, fd, &ev, sizeof(struct input_event));

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_REL;
	ev.code = REL_Y;
	ev.value = y;
	SAFE_WRITE(NULL, 1, fd, &ev, sizeof(struct input_event));

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	SAFE_WRITE(NULL, 1, fd, &ev, sizeof(struct input_event));
}

void send_event(int fd, int event, int code, int value)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = event;
	ev.code = code;
	ev.value = value;
	SAFE_WRITE(NULL, 1, fd, &ev, sizeof(struct input_event));
}

void create_device(int fd)
{
	struct uinput_user_dev uidev;

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, VIRTUAL_DEVICE);
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;

	SAFE_WRITE(NULL, 1, fd, &uidev, sizeof(uidev));
	SAFE_IOCTL(NULL, fd, UI_DEV_CREATE, NULL);
}