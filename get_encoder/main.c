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
#include <poll.h>

#define input_event_sec time.tv_sec
#define input_event_usec time.tv_usec

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
	struct pollfd pfds[2];
	pfds[0].fd = fd[0];
	pfds[0].events = POLLIN;
	pfds[1].fd = fd[1];
	pfds[1].events = POLLIN;

	/* wait for events and process them */
	int count = 0;	// tracking time
	while(!stop)
	{
		/* check if event queues have data - timeout of 0 is nonblocking */
		poll(pfds, 2, 0);
		count++;
		if (stop)
			break;
		
		/* look at both */
		for(int j=0;j<2;j++)
		{
			/* if one is ready handle it */
			if(pfds[j].revents & POLLIN)
			{
				printf("fd[%d] is set, count = %d\n", j, count);
				count = 0;
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
