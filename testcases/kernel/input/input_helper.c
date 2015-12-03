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
#define VIRTUAL_DEVICE "virtual-device-ltp"

#define VIRTUAL_DEVICE_REGEX "*virtual-device-ltp*"

static int check_device(void);

int setup_read(void)
{
	DIR *d;
	char path[256];
	char name[256];
	struct dirent *dp;
	int fd;
	int ret;

	ret = 0;
	fd = -1;
	d = opendir("/dev/input");

	while ((dp = readdir(d))) {
		if (fnmatch("event[0-9]*", dp->d_name, 0))
			continue;
		snprintf(path, sizeof(path), "/dev/input/%s", dp->d_name);
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			tst_resm(TINFO, "failed to open %s", path);
			break;
		}
		ret = ioctl(fd, EVIOCGNAME(256), name);
		if (ret < 0) {
			tst_resm(TINFO | TERRNO,
				"ioctl(%s, EVIOCGNAME(256), ...) failed",
				dp->d_name);
			break;
		}
		if (strcmp(name, VIRTUAL_DEVICE) == 0)
			break;
	}

	closedir(d);
	if (fd > 0 && ret >= 0)
		return fd;
	tst_brkm(TBROK, NULL, "unable to find the right event");
	return -1;
}

int open_uinput(void)
{
	int fd;

	fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0 && errno == ENOENT)
		fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (fd < 0 && errno == ENOENT)
		tst_brkm(TCONF, NULL, "unable to find and open uinput");
	if (fd < 0)
		tst_brkm(TERRNO, NULL, "open failed");

	return fd;
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

void send_rel_move(int fd, int x, int y)
{
	send_event(fd, EV_REL, REL_X, x);
	send_event(fd, EV_REL, REL_Y, y);
	send_event(fd, EV_SYN, 0, 0);
}

void create_device(int fd)
{
	struct uinput_user_dev uidev;
	int nb;

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, VIRTUAL_DEVICE);
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;

	SAFE_WRITE(NULL, 1, fd, &uidev, sizeof(uidev));
	SAFE_IOCTL(NULL, fd, UI_DEV_CREATE, NULL);

	nb = 20;
	do {
		usleep(100000);
		if (check_device())
			return;
		nb--;

	} while (nb > 0);

	destroy_device(fd);
	tst_brkm(TBROK, NULL, "failed to create device");

}

void setup_mouse_events(int fd)
{
	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_KEY);
	SAFE_IOCTL(NULL, fd, UI_SET_KEYBIT, BTN_LEFT);
	SAFE_IOCTL(NULL, fd, UI_SET_EVBIT, EV_REL);
	SAFE_IOCTL(NULL, fd, UI_SET_RELBIT, REL_X);
	SAFE_IOCTL(NULL, fd, UI_SET_RELBIT, REL_Y);
}

void destroy_device(int fd)
{
	SAFE_IOCTL(NULL, fd, UI_DEV_DESTROY, NULL);
	SAFE_CLOSE(NULL, fd);
}

/*
 * read will eventually return ENODEV because
 * the device will be destroyed by the other process
 * after having sent the informations
*/
int check_no_data_and_close_fd(int fd)
{
	int rd;
	struct input_event iev[64];

	rd = read(fd, iev, sizeof(struct input_event) * 64);

	SAFE_CLOSE(NULL, fd);
	return rd < 0 && errno == ENODEV;
}

static int check_device(void)
{
	FILE *file;
	char line[256];

	file = fopen("/proc/bus/input/devices", "r");
	if (!file)
		return 0;

	while (fgets(line, 256, file)) {

		if (fnmatch(VIRTUAL_DEVICE_REGEX, line, 0) == 0)
			return 1;
	}

	fclose(file);

	return 0;
}
