/*
 * Mini xargs implementation for busybox
 * Only "-prt" options are supported in this version of xargs.
 *
 * (C) 2002 by Vladimir Oleynik <dzo@simtreas.ru>
 *
 * Special thanks Mark Whitley for stimul to rewrote :)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "busybox.h"


/*
   This function have special algorithm.
   Don`t use fork and include to main!
*/
static void xargs_exec(char * const * args)
{
	int p;
	int common[4];  /* shared vfork stack */

	common[0] = 0;
	if ((p = vfork()) >= 0) {
		if (p == 0) {
			/* vfork -- child */
			execvp(args[0], args);
			common[0] = errno; /* set error to shared stack */
			_exit(1);
		} else {
			/* vfork -- parent */
			wait(NULL);
			if(common[0]) {
				errno = common[0];
				perror_msg_and_die("%s", args[0]);
			}
		}
	} else {
		perror_msg_and_die("vfork");
	}
}

int xargs_main(int argc, char **argv)
{
	char *file_to_act_on;
	char **args;
	int  i, a;
	char flg_vi       = 0;    /* verbose |& interactive */
	char flg_no_empty = 0;

	while ((a = getopt(argc, argv, "prt")) > 0) {
		switch(a) {
			case 'p':
				flg_vi |= 3;
				break;
			case 't':
				flg_vi |= 1;
				break;
			case 'r':
				flg_no_empty = 1;
				break;
			default:
				show_usage();
		}
	}

	a = argc - optind;
	argv += optind;
	if(a==0) {
		/* default behavior is to echo all the filenames */
		*argv = "/bin/echo";
		a++;
	}
	/* allocating pointers for execvp: a*arg, arg from stdin, NULL */
	args = xcalloc(a + 3, sizeof(char *));

	/* Store the command to be executed (taken from the command line) */
	for (i = 0; i < a; i++)
		args[i] = *argv++;

	/* Now, read in one line at a time from stdin, and store this 
	 * line to be used later as an argument to the command */
	while ((file_to_act_on = get_line_from_file(stdin)) != NULL) {
		/* eat the newline off the filename. */
		chomp(file_to_act_on);
		if(file_to_act_on[0] != 0 || flg_no_empty == 0) {
			args[a] = file_to_act_on[0] ? file_to_act_on : NULL;
			if(flg_vi) {
				for(i=0; args[i]; i++) {
					if(i)
						fputc(' ', stderr);
					fputs(args[i], stderr);
				}
				fprintf(stderr, "%s", ((flg_vi & 2) ? " ?..." : "\n"));
			}
			if((flg_vi & 2) == 0 || ask_confirmation() != 0 ) {
				xargs_exec(args);
			}
		}
		/* clean up */
		free(file_to_act_on);
	}
#ifdef CONFIG_FEATURE_CLEAN_UP
	free(args);
#endif
	return 0;
}
