/* modified version of Brian "Beej Jorgensen" Hall's talker.c
** gpiotalker.c 

 **	Compile the code below as a regular user by doing clang -o
 **	gpiotalker gpiotalker.c. Then copy gpiotalker to /usr/local/bin
 ** 	and /sbin/chown root._gpiotalker /usr/local/bin/gpiotalker ;
 **	/bin/chmod 755 /usr/local/bin/gpiotalker.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/gpio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#define SERVERPORT "4950"       // the port users will be connecting to
#define SERVER "listener.my.domain"	// the computer we're sending to
#define THERM_PIN 21
#define MAX_TEMP 74

int main()
{
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
     	/* BEGIN an added section */
        int sum;
        int devfd = open("/dev/gpio0", O_RDWR);
        int pins[] = {16, 12, 7, 8, 25, 24, 23};
        int i;
        struct gpio_pin_op op;
        daemon(1,1);
        /* END an added section */
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        if ((rv = getaddrinfo(SERVER, SERVERPORT, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
        }
        // loop through all the results and make a socket
        for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
                        perror("talker: socket");
                        continue;
                }
                break;
        }
        if (p == NULL) {
                fprintf(stderr, "talker: failed to create socket\n");
                return 2;
        }
 	/* BEGIN an added section */
        while (1) {
                sum = 0;
                for (i = 0; i < 7; i++) {
                        memset(&op, 0, sizeof(op));
                        op.gp_pin = pins[i];
                 ioctl(devfd, GPIOPINREAD, &op);
                        sum += (1 << i) * op.gp_value;
                }
        /* END an added section */
                if ((numbytes = sendto(sockfd, &sum, sizeof sum, 0,
                                 p->ai_addr, p->ai_addrlen)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                }
        /* BEGIN an added section */
                memset(&op, 0, sizeof(op));
                op.gp_pin = THERM_PIN;
                if (sum >= MAX_TEMP) {
                        op.gp_value = GPIO_PIN_HIGH;
                } else {
                        op.gp_value = GPIO_PIN_LOW;
                }
                if (ioctl(devfd, GPIOPINWRITE, &op))
                        fprintf(stderr, "pin write error\n");
        /* END added section */        
                sleep(10);
        }
        freeaddrinfo(servinfo);
        close(sockfd);
        return 0;
}
