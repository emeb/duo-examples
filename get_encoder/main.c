/*
 * main.c - top level of get_encoder utility
 * 09-01-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

#define input_event_sec time.tv_sec
#define input_event_usec time.tv_usec

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

#ifndef EV_SYN
#define EV_SYN 0
#endif
#ifndef SYN_MAX
#define SYN_MAX 3
#define SYN_CNT (SYN_MAX + 1)
#endif
#ifndef SYN_MT_REPORT
#define SYN_MT_REPORT 2
#endif
#ifndef SYN_DROPPED
#define SYN_DROPPED 3
#endif

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

static volatile sig_atomic_t stop = 0;

static void interrupt_handler(int sig)
{
	printf("\ninterrupt!\n");
	stop = 1;
}

/*
 * top level
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;
	int verbose = 0;
    
	/* parse options */
	while((opt = getopt(argc, argv, "+vVh")) != EOF)
	{
		switch(opt)
		{
			case 'v':
				verbose = 1;
				break;
			
			case 'V':
				fprintf(stderr, "%s version %s\n", argv[0], swVersionStr);
				exit(0);
			
			case 'h':
			case '?':
				fprintf(stderr, "USAGE: %s [options] CHL\n", argv[0]);
				fprintf(stderr, "Version %s, %s %s\n", swVersionStr, bdate, btime);
				fprintf(stderr, "Options: -v enables verbose progress messages\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
		
	if(verbose)
		printf("Setup open device...\n");
	
	int fd[2];
	char *filename[] = 
	{
		"/dev/input/event1",
		"/dev/input/event0"
	};

	for(int j=0;j<2;j++)
	{
		if((fd[j] = open(filename[j], O_RDONLY)) < 0)
		{
			perror("get_encoder");
			exit(1);
		}
		else
		{
			printf("Opened %s as fd[%d] = %d\n", filename[j], j, fd[j]);
			ioctl(fd[j], EVIOCGRAB, (void*)1);
		}
	}
		
#if 0
	int version;
	unsigned short id[4];
	char name[256] = "Unknown";

	if (ioctl(fd, EVIOCGVERSION, &version)) {
		perror("evtest: can't get version");
		return 1;
	}

	printf("Input driver version is %d.%d.%d\n",
		version >> 16, (version >> 8) & 0xff, version & 0xff);

	ioctl(fd, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
		id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);
	printf("Testing ... (interrupt to exit)\n");
#endif

	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);
	
	struct input_event ev[64];
	int i, rd;
	fd_set rdfs;

	/* wait for events and process them */
	while (!stop) {
		/* add the rotary and button to the read set and wait */
		FD_ZERO(&rdfs);
		FD_SET(fd[0], &rdfs);
		FD_SET(fd[1], &rdfs);
		select(fd[1] + 1, &rdfs, NULL, NULL, NULL);
		if (stop)
			break;
		
		/* look at both */
		for(int j=0;j<2;j++)
		{
			/* if one is ready handle it */
			if(FD_ISSET(fd[j], &rdfs))
			{
				printf("fd[%d] is set\n", j);
				rd = read(fd[j], ev, sizeof(ev));

				if (rd < (int) sizeof(struct input_event)) {
					printf("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
					perror("\nevtest: error reading");
					return 1;
				}
				else
					printf("Got %d bytes = %ld events\n", rd, rd / sizeof(struct input_event));

				for (i = 0; i < rd / sizeof(struct input_event); i++) {
					unsigned int type, code;

					type = ev[i].type;
					code = ev[i].code;

					printf("Event %d: time %ld.%06ld, ", i, ev[i].input_event_sec, ev[i].input_event_usec);

					if(type != EV_SYN)
					{
						printf("type %d, code %d, ", type, code);
						if (type == EV_MSC && (code == MSC_RAW || code == MSC_SCAN))
							printf("value %02x\n", ev[i].value);
						else
							printf("value %d\n", ev[i].value);
					}
					else
						printf("EV_SYN\n");
				}
			}
		}
	}

	/* ungrab the devices */
	ioctl(fd[0], EVIOCGRAB, (void*)0);
	ioctl(fd[1], EVIOCGRAB, (void*)0);
	
	return 0;
}
