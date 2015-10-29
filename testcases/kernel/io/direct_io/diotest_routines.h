/*
 * Copyright (c) International Business Machines  Corp., 2002
 *  04/30/2002 Narasimha Sharoff nsharoff@us.ibm.com
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef DIOTEST_ROUTINES_H
#define DIOTEST_ROUTINES_H

#include "lapi/fcntl.h"

void fillbuf(char *buf, int count, char value);
void vfillbuf(struct iovec *iv, int vcnt, char value);
int filecmp(char *f1, char *f2);
int bufcmp(char *b1, char *b2, int bsize);
int vbufcmp(struct iovec *iv1, struct iovec *iv2, int vcnt);
int forkchldrn(int **pidlst, int numchld, int action, int (*chldfunc)());
int waitchldrn(int **pidlst, int numchld);
int killchldrn(int **pidlst, int numchld, int sig);

#endif /* DIOTEST_ROUTINES_H */
