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

#ifndef INPUT_HELPER_H
#define INPUT_HELPER_H

#define VIRTUAL_DEVICE "virtual-device-ltp"

#include <sys/types.h>
#include <dirent.h>

char *find_event(DIR *d, char *namedir1);
int setup_read(void);
void send_rel_move(int fd, int x, int y);
void send_event(int fd, int event, int code, int value);
int open_uinput(void);
void create_device(int fd);

#endif /* INPUT_HELPER_H */
