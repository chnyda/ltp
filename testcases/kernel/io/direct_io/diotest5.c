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
 *      diotest5.c
 *
 * DESCRIPTION
 *	The programs test buffered and direct IO with vector arrays using
 *	readv() and writev() calls.
 *	Test blocks
 *	[1] Direct readv, Buffered writev
 *	[2] Direct writev, Buffered readv
 *	[3] Direct readv, Direct writev
 *	The bufsize should be in n*4k size for direct readv, writev. The offset
 *	value marks the starting position in file from where to start the
 *	write and read. (Using larger offset, larger files can be tested).
 *	The nvector gives vector array size.  Test data file can be
 *	specified through commandline and is useful for running test with
 *	raw devices as a file.
 *
 * USAGE
 *      diotest5 [-b bufsize] [-o offset] [-i iterations]
 *			[-v nvector] [-f filename]
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <errno.h>

#include "diotest_routines.h"

#include "test.h"

char *TCID = "diotest05";
int TST_TOTAL = 3;

#define	BUFSIZE	4096
#define TRUE 1
#define LEN 30
#define	READ_DIRECT 1
#define	WRITE_DIRECT 2
#define	RDWR_DIRECT 3

static int bufsize = BUFSIZE;	/* Buffer size. Default 4k */
static int iter = 20;		/* Iterations. Default 20 */
static int nvector = 20;	/* Vector array. Default 20 */
static off64_t offset;	/* Start offset. Default 0 */
static char filename[LEN];	/* Test data file */
static int fd1 = -1;

static void setup(void);
static void cleanup(void);
static int runtest(int fd_r, int fd_w, int iter, off64_t offset);
static void prg_usage(void);

/*
 * runtest: Write the data in vector array to the file. Read the data
 *	from the file into another vectory array and verify. Repeat the test.
*/
static int runtest(int fd_r, int fd_w, int iter, off64_t offset)
{
	int i, bufsize = BUFSIZE;
	struct iovec *iov1, *iov2, *iovp;

	/* Allocate for buffers and data pointers */
	iov1 = (struct iovec *) valloc(sizeof(struct iovec) * nvector);
	if (iov1 == NULL) {
		tst_resm(TFAIL, "valloc() buf1 failed: %s", strerror(errno));
		return (-1);
	}

	iov2 = (struct iovec *) valloc(sizeof(struct iovec) * nvector);
	if (iov2 == NULL) {
		tst_resm(TFAIL, "valloc buf2 failed: %s", strerror(errno));
		return (-1);
	}
	for (i = 0, iovp = iov1; i < nvector; iovp++, i++) {
		iovp->iov_base = valloc(bufsize);
		if (iovp->iov_base == NULL) {
			tst_resm(TFAIL, "valloc for iovp->iov_base: %s",
				 strerror(errno));
			return (-1);
		}
		iovp->iov_len = bufsize;
	}
	for (i = 0, iovp = iov2; i < nvector; iovp++, i++) {
		iovp->iov_base = valloc(bufsize);
		if (iovp->iov_base == NULL) {
			tst_resm(TFAIL, "valloc, iov2 for iovp->iov_base: %s",
				 strerror(errno));
			return (-1);
		}
		iovp->iov_len = bufsize;
	}

	/* Test */
	for (i = 0; i < iter; i++) {
		vfillbuf(iov1, nvector, i);
		vfillbuf(iov2, nvector, i + 1);
		if (lseek(fd_w, offset, SEEK_SET) < 0) {
			tst_resm(TFAIL, "lseek before writev failed: %s",
				 strerror(errno));
			return (-1);
		}
		if (writev(fd_w, iov1, nvector) < 0) {
			tst_resm(TFAIL, "writev failed: %s", strerror(errno));
			return (-1);
		}
		if (lseek(fd_r, offset, SEEK_SET) < 0) {
			tst_resm(TFAIL, "lseek before readv failed: %s",
				 strerror(errno));
			return (-1);
		}
		if (readv(fd_r, iov2, nvector) < 0) {
			tst_resm(TFAIL, "readv failed: %s", strerror(errno));
			return (-1);
		}
		if (vbufcmp(iov1, iov2, nvector) != 0) {
			tst_resm(TFAIL, "readv/writev comparision failed");
			return (-1);
		}
	}

	/* Cleanup */
	for (i = 0, iovp = iov1; i < nvector; iovp++, i++)
		free(iovp->iov_base);
	for (i = 0, iovp = iov2; i < nvector; iovp++, i++)
		free(iovp->iov_base);
	free(iov1);
	free(iov2);
	return 0;
}

/*
 * prg_usage: Display the program usage
*/
static void prg_usage(void)
{
	fprintf(stderr,
		"Usage: diotest5 [-b bufsize] [-o offset] [ -i iteration] [ -v nvector] [-f filename]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, fd_r, fd_w;
	int fail_count = 0, total = 0, failed = 0;

	/* Options */
	sprintf(filename, "testdata-5.%ld", syscall(__NR_gettid));
	while ((i = getopt(argc, argv, "b:o:i:v:f:")) != -1) {
		switch (i) {
		case 'b':
			bufsize = atoi(optarg);
			if (bufsize <= 0) {
				fprintf(stderr, "bufsize must be > 0");
				prg_usage();
			}
			if (bufsize % 4096 != 0) {
				fprintf(stderr, "bufsize must be > 0");
				prg_usage();
			}
			break;
		case 'o':
			offset = atoll(optarg);
			if (offset <= 0) {
				fprintf(stderr, "offset must be > 0");
				prg_usage();
			}
			break;
		case 'i':
			iter = atoi(optarg);
			if (iter <= 0) {
				fprintf(stderr, "iterations must be > 0");
				prg_usage();
			}
			break;
		case 'v':
			nvector = atoi(optarg);
			if (nvector <= 0) {
				fprintf(stderr, "vector array must be > 0");
				prg_usage();
			}
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		default:
			prg_usage();
		}
	}

	setup();

	/* Testblock-1: Read with Direct IO, Write without */
	fd_w = open(filename, O_WRONLY | O_CREAT, 0666);
	if (fd_w < 0) {
		tst_brkm(TBROK, cleanup, "fd_w open failed for %s: %s",
			 filename, strerror(errno));
	}
	fd_r = open64(filename, O_DIRECT | O_RDONLY | O_CREAT, 0666);
	if (fd_r < 0) {
		tst_brkm(TBROK, cleanup, "fd_r open failed for %s: %s",
			 filename, strerror(errno));
	}
	if (runtest(fd_r, fd_w, iter, offset) < 0) {
		failed = TRUE;
		fail_count++;
		tst_resm(TFAIL, "Read with Direct IO, Write without");
	} else
		tst_resm(TPASS, "Read with Direct IO, Write without");

	unlink(filename);
	close(fd_r);
	close(fd_w);
	total++;

	/* Testblock-2: Write with Direct IO, Read without */

	fd_w = open(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);
	if (fd_w < 0) {
		tst_brkm(TBROK, cleanup, "fd_w open failed for %s: %s",
			 filename, strerror(errno));
	}
	fd_r = open64(filename, O_RDONLY | O_CREAT, 0666);
	if (fd_r < 0) {
		tst_brkm(TBROK, cleanup, "fd_r open failed for %s: %s",
			 filename, strerror(errno));
	}
	if (runtest(fd_r, fd_w, iter, offset) < 0) {
		failed = TRUE;
		fail_count++;
		tst_resm(TFAIL, "Write with Direct IO, Read without");
	} else
		tst_resm(TPASS, "Write with Direct IO, Read without");
	unlink(filename);
	close(fd_r);
	close(fd_w);
	total++;

	/* Testblock-3: Read, Write with Direct IO */
	fd_w = open(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);
	if (fd_w < 0) {
		tst_brkm(TBROK, cleanup, "fd_w open failed for %s: %s",
			 filename, strerror(errno));
	}

	fd_r = open64(filename, O_DIRECT | O_RDONLY | O_CREAT, 0666);
	if (fd_r < 0) {
		tst_brkm(TBROK, cleanup, "fd_r open failed for %s: %s",
			 filename, strerror(errno));
	}
	if (runtest(fd_r, fd_w, iter, offset) < 0) {
		failed = TRUE;
		fail_count++;
		tst_resm(TFAIL, "Read, Write with Direct IO");
	} else
		tst_resm(TPASS, "Read, Write with Direct IO");
	unlink(filename);
	close(fd_r);
	close(fd_w);
	total++;

	if (failed)
		tst_resm(TINFO, "%d/%d testblocks failed", fail_count, total);
	else
		tst_resm(TINFO,
			 "%d testblocks %d iterations with %d vector array completed",
			 total, iter, nvector);

	cleanup();

	tst_exit();
}

static void setup(void)
{

	tst_tmpdir();

	fd1 = open(filename, O_CREAT | O_EXCL, 0600);
	if (fd1 < 0) {
		tst_brkm(TBROK, cleanup, "Couldn't create test file %s: %s",
			 filename, strerror(errno));
	}
	close(fd1);

	/* Test for filesystem support of O_DIRECT */
	fd1 = open(filename, O_DIRECT, 0600);
	if (fd1 < 0 && errno == EINVAL)
		tst_brkm(TCONF, cleanup,
			 "O_DIRECT is not supported by this filesystem. %s",
			 strerror(errno));
	else if (fd1 < 0)
		tst_brkm(TBROK, cleanup, "Couldn't open test file %s: %s",
			 filename, strerror(errno));
	close(fd1);
}

static void cleanup(void)
{
	if (fd1 != -1)
		unlink(filename);

	tst_rmdir();
}
