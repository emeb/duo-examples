/*
 * encoder.h - non-blocking interface to encoder/button drivers
 * 09-02-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include "encoder.h"

static char *filenames[] = 
{
	"/dev/input/event1",
	"/dev/input/event0"
};
static int fd[2];

/*
 * open the devices
 */
uint8_t encoder_init(void)
{
	for(int j=0;j<2;j++)
	{
		if((fd[j] = open(filenames[j], O_RDONLY)) < 0)
		{
			fprintf(stderr, "Error opening device %s\n", filenames[j]);
			return 1;
		}
		else
		{
			//fprintf("Opened %s as fd[%d] = %d\n", filenames[j], j, fd[j]);
			ioctl(fd[j], EVIOCGRAB, (void*)1);
		}
	}
		
	return 0;
}

/*
 * nonblocking check for input
 */
uint8_t encoder_poll(int16_t *enc_val, uint8_t *enc_btn)
{
	struct input_event ev[64];
	int i, rd;
	struct pollfd pfds[2];
	pfds[0].fd = fd[0];
	pfds[0].events = POLLIN;
	pfds[1].fd = fd[1];
	pfds[1].events = POLLIN;
	uint8_t result = 0;
	
	/* init return values */
	*enc_val = 0;
	*enc_btn = 0;
	
	/* check if event queues have data - timeout of 0 is nonblocking */
	poll(pfds, 2, 0);
	
	/* look at both */
	for(int j=0;j<2;j++)
	{
		/* if one is ready handle it */
		if(pfds[j].revents & POLLIN)
		{
			//printf("fd[%d] is set, count = %d\n", j, count);
			rd = read(fd[j], ev, sizeof(ev));

			if(rd < (int) sizeof(struct input_event))
			{
				//printf("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
				//perror("\nevtest: error reading");
				return -1;
			}
			//else
			//	printf("Got %d bytes = %ld events\n", rd, rd / sizeof(struct input_event));

			for(i = 0; i < rd / sizeof(struct input_event); i++)
			{
				//printf("Event %d: time %ld.%06ld, ", i, ev[i].input_event_sec, ev[i].input_event_usec);

				if(ev[i].type != EV_SYN)
				{
					//printf("type %d, code %d, value = %d\n", ev[i].type, ev[i].code, ev[i].value);
					if((ev[i].type == 1) && (ev[i].code == 103))
					{
						/* button press, release or repeat */
						*enc_btn |= 1<<ev[i].value;
						result++;
					}
					else if(ev[i].type == 2)
					{
						/* encoder rotation */
						*enc_val += ev[i].value;
						result++;
					}
				}
				//else
				//	printf("EV_SYN\n");
			}
		}
	}
	
	return result;
}

/*
 * clean up
 */
void encoder_deinit(void)
{
	/* ungrab the devices */
	ioctl(fd[0], EVIOCGRAB, (void*)0);
	ioctl(fd[1], EVIOCGRAB, (void*)0);
	
	/* free fds */
	close(fd[0]);
	close(fd[1]);
}
