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

/*
 * NAME
 *      diotest1.c
 *
 * DESCRIPTION
 *	Copy the contents of the input file to output file using direct read
 *	and direct write. The input file size is numblks*bufsize.
 *	The read and write calls use bufsize to perform IO. Input and output
 *	files can be specified through commandline and is useful for running
 *	test with raw devices as files.
 *
 * USAGE
 *	diotest1 [-b bufsize] [-n numblks] [-i infile] [-o outfile]
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/fcntl.h>
#include <errno.h>

#include "test.h"
#include "diotest_routines.h"
#include "safe_macros.h"

#define	BUFSIZE	8192
#define	NBLKS	20
#define	LEN	30
#define	TRUE 1

char *TCID = "diotest01";
int TST_TOTAL = 1;
static int fd1;
static int fd2;
static char infile[LEN];
static char outfile[LEN];

static void cleanup(void);
static void prg_usage(void);

/*
 * prg_usage: display the program usage.
*/
static void prg_usage(void)
{
	fprintf(stderr,
		"Usage: diotest1 [-b bufsize] [-n numblks] [-i infile] [-o outfile]\n");
	tst_brkm(TBROK, NULL, "usage");
}

/*
 * fail_clean: cleanup and exit.
*/
static void cleanup(void)
{
	if (fd1 > 0)
		close(fd1);
	if (fd2 > 0)
		close(fd2);
	unlink(infile);
	unlink(outfile);
}

int main(int argc, char *argv[])
{
	int bufsize = BUFSIZE;	/* Buffer size. Default 8k */
	int numblks = NBLKS;	/* Number of blocks. Default 20 */
	int fd, fd1, fd2;
	int i, n, offset;
	char *buf;

	/* Options */
	strcpy(infile, "infile");	/* Default input file */
	strcpy(outfile, "outfile");	/* Default outfile file */
	while ((i = getopt(argc, argv, "b:n:i:o:")) != -1) {
		switch (i) {
		case 'b':
			bufsize = atoi(optarg);
			if (bufsize <= 0) {
				fprintf(stderr, "bufsize must be > 0\n");
				prg_usage();
			}
			if (bufsize % 4096 != 0) {
				fprintf(stderr,
					"bufsize must be multiple of 4k\n");
				prg_usage();
			}
			break;
		case 'n':
			numblks = atoi(optarg);
			if (numblks <= 0) {
				fprintf(stderr, "numblks must be > 0\n");
				prg_usage();
			}
			break;
		case 'i':
			strcpy(infile, optarg);
			break;
		case 'o':
			strcpy(outfile, optarg);
			break;
		default:
			prg_usage();
		}
	}

	/* Test for filesystem support of O_DIRECT */
	fd = open(infile, O_DIRECT | O_RDWR | O_CREAT, 0666);
	if (fd < 0 && errno == EINVAL)
		tst_brkm(TCONF, NULL,
			 "O_DIRECT is not supported by this filesystem. %s",
			 strerror(errno));
	else if (fd < 0)
		tst_brkm(TBROK, NULL, "Couldn't open test file %s: %s",
			 infile, strerror(errno));
	close(fd);

	/* Open files */
	fd1 = SAFE_OPEN(NULL, infile, O_DIRECT | O_RDWR | O_CREAT, 0666);

	fd2 = SAFE_OPEN(cleanup, outfile, O_DIRECT | O_RDWR | O_CREAT, 0666);

	/* Allocate for buf, Create input file */
	buf = valloc(bufsize);
	if (!buf) {
		tst_resm(TFAIL, "valloc() failed: %s", strerror(errno));
		cleanup();
	}
	for (i = 0; i < numblks; i++) {
		fillbuf(buf, bufsize, (char)(i % 256));
		if (write(fd1, buf, bufsize) < 0) {
			tst_resm(TFAIL, "write infile failed: %s",
				 strerror(errno));
			cleanup();
		}
	}

	/* Copy infile to outfile using direct read and direct write */
	offset = 0;
	if (lseek(fd1, offset, SEEK_SET) < 0) {
		tst_resm(TFAIL, "lseek(infd) failed: %s", strerror(errno));
		cleanup();
	}
	while ((n = read(fd1, buf, bufsize)) > 0) {
		if (lseek(fd2, offset, SEEK_SET) < 0) {
			tst_resm(TFAIL, "lseek(outfd) failed: %s",
				 strerror(errno));
			cleanup();
		}
		if (write(fd2, buf, n) < n) {
			tst_resm(TFAIL, "write(outfd) failed: %s",
				 strerror(errno));
			cleanup();
		}
		offset += n;
		if (lseek(fd1, offset, SEEK_SET) < 0) {
			tst_resm(TFAIL, "lseek(infd) failed: %s",
				 strerror(errno));
			cleanup();
		}
	}

	if (filecmp(infile, outfile) != 0) {
		tst_resm(TFAIL, "file compare failed for %s and %s",
			 infile, outfile);
		cleanup();
	}

	cleanup();
	tst_resm(TPASS, "Test passed");
	tst_exit();
}
