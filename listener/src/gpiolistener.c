/* modified version of Brian "Beej Jorgensen" Hall's listener.c
** gpiolistener.c
** will later add code to see if signal from talker is stale
*/
#include <time.h>	// for checking for stale signal
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/gpio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#define MYPORT "4950"   // the port users will be connecting to
// BEGIN a section I added
#define NUM_PINS 7
#define ALERT_PIN 21
#define STALE_PIN 20
#define UPPER_TEMP 75
#define LOWER_TEMP 40
#define MAX_REPRESENT_TEMP 127
#define MIN_REPRESENT_TEMP 0
#define WAIT_FOR_TALKER 20
// END a section I added
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc, char *argv[])
{
	/* ADDED section to declare variable for checking stale signal */
	struct timeval tv = { .tv_sec = WAIT_FOR_TALKER, .tv_usec = 0 };
	time_t curr = time(NULL);
	time_t prev = curr - 86400;	/* make sure prev is stale */
	/* end declarations for stale signal checking */
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
 	// in the next line I changed INET6 to INET
        char s[INET_ADDRSTRLEN];
        // BEGIN a section I added
        struct gpio_pin_op op;
        int devfd;
        int i, ch;
        int temperature;
        const char *errstr;
        int pins[] = {16, 12, 7, 8, 25, 24, 23};
        int upper_temp, lower_temp;
        if (daemon(1,0) == -1)
                exit(1);
        if ((devfd = open("/dev/gpio0", O_RDWR)) == -1)
                exit(1);
        if (chdir("/var/empty") == -1)
                exit(1);
        upper_temp = UPPER_TEMP;
        lower_temp = LOWER_TEMP;
        while ((ch = getopt(argc, argv, "l:u:")) != -1) {
		switch (ch) {
                case 'u':
                        upper_temp = strtonum(optarg, MIN_REPRESENT_TEMP,
                            MAX_REPRESENT_TEMP, &errstr);
                        if (errstr != NULL)
                                upper_temp = UPPER_TEMP;
                        break;
                case 'l':
                        lower_temp = strtonum(optarg, MIN_REPRESENT_TEMP,
                            MAX_REPRESENT_TEMP, &errstr);
                        if (errstr != NULL)
                                lower_temp = LOWER_TEMP;
                        break;
                default:
                        break;
                }
        }
        argc -= optind;
        argv += optind;
        // END a section I added
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP
        if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
        }
        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
                        perror("listener: socket");
                        continue;
                }
		fcntl(sockfd, O_NONBLOCK);	/* ADDED. think needed so can continue if no
							signal received from Talker */
                if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                        close(sockfd);
                        perror("listener: bind");
                        continue;
                }
                break;
        }
        if (p == NULL) {
                /* commenting out print */
                /* fprintf(stderr, "listener: failed to bind socket\n"); */
                return 2;
        }
        freeaddrinfo(servinfo);
        // BEGIN a section I modified to remove print statements
        // and send the temperature as data instead of a string
        while(1) {
                addr_len = sizeof their_addr;
		
		/* ADDED. timeout, don't hang if receive from talker fails */
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		
		/* CHANGED. don't want exit if fails */
		if ((recvfrom(sockfd, &temperature, sizeof temperature, 0,
                    (struct sockaddr *)&their_addr, &addr_len)) != -1) {
			prev = curr;	/* only reset previous time value if we didn't timeout */
		}
        // END a section I modified
        /* BEGIN section I added to write to the GPIO pins.
	   We bitwise compare the temperature to 64, 32,
           16, 8, 4, 2, 1 and write to the corresponding pin
        */
		/* turn on stale pin if data is stale */
		if (curr - prev > WAIT_FOR_TALKER) {
			/* ADDED. refactor, put in function later */
			memset(&op, 0, sizeof(op));
                        op.gp_pin = STALE_PIN;
                        op.gp_value = GPIO_PIN_HIGH;
                        ioctl(devfd, GPIOPINWRITE, &op);
		} else { 
			memset(&op, 0, sizeof(op));
                        op.gp_pin = STALE_PIN;
                        op.gp_value = GPIO_PIN_HIGH;
                        ioctl(devfd, GPIOPINWRITE, &op);
		}
		
		/* light up LEDs to display temperature */
                for (i = 0; i < NUM_PINS; i++) {
                        memset(&op, 0, sizeof(op));
                        op.gp_pin = pins[i];
                        if (temperature & (1 << i)) {
                                op.gp_value = GPIO_PIN_HIGH;
                        } else {
                                op.gp_value = GPIO_PIN_LOW;
                        }
			ioctl(devfd, GPIOPINWRITE, &op); /* don't forget to actually write to the pin */
                }
                // light up alert PIN if outside desired range
                if (temperature  >= upper_temp || temperature <= lower_temp) {
                        memset(&op, 0, sizeof(op));
                        op.gp_pin = ALERT_PIN;
                        op.gp_value = GPIO_PIN_HIGH;
                        ioctl(devfd, GPIOPINWRITE, &op);
                } else {
                        memset(&op, 0, sizeof(op));
                        op.gp_pin = ALERT_PIN;
                 op.gp_value = GPIO_PIN_LOW;
                        ioctl(devfd, GPIOPINWRITE, &op);
                }
		curr = time(NULL);
                sleep(WAIT_FOR_TALKER);
        }
        // END a section I added
        close(sockfd);
        return 0;
}
