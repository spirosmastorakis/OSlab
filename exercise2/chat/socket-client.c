/*
 * socket-client.c
 * Simple TCP/IP communication using sockets
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 * Spyridon (Spiros) Mastorakis
 * George Matikas
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <crypto/cryptodev.h>

#include "socket-common.h"

/* Insist until all of the data has been read */
ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                if (ret == 0) {
                	printf("Server went away. Exiting...\n");
                	return 0;
                }
                if (ret < 0) {
                	perror("read from server failed");
                        return ret;
                }
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}

/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

int main(int argc, char *argv[])
{
	int fd, sd, port;
	char buf[256],temp[256],key[KEY_SIZE+1],iv[BLOCK_SIZE+1];
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in sa;
	struct session_op sess;
	struct crypt_op cryp;
	
	memset(&sess, 0, sizeof(sess));
	memset(&cryp, 0, sizeof(cryp));
	
	if (argc != 3) {
		printf("Usage: %s [hostname] [port]\n", argv[0]);
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]); /* Needs better error checking */

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	printf("Created TCP socket\n");
	
	/* Look up remote hostname on DNS */
	if (!(hp = gethostbyname(hostname))) {
		perror("DNS lookup failed");
		exit(1);
	}

	/* Connect to remote TCP port */
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	printf("Connecting to remote host... ");
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("connect");
		exit(1);
	}
	printf("Connected.\n");
	
	fd = open("/dev/cryptodev1", O_RDWR);
	if (fd < 0) {
		perror("open(/dev/cryptodev1)");
		return 1;
	}
	
	strcpy(iv,IV);
	strcpy(key,KEY);
	
	/*
	 * Get crypto session for AES128
	 */
	sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;
	sess.key = key;

	if (ioctl(fd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}
	
	cryp.ses = sess.ses;
	cryp.iv = iv;

	/* Read answer and write it to standard output */
	for (;;) {
        
        	printf("You: ");
        	gets(buf);
		
		if ( strcmp(buf,"/exit")==0 ) break;
		
		cryp.src = buf;
		cryp.dst = temp;
		cryp.len = sizeof(buf);
		cryp.op = COP_ENCRYPT;
		
		if (ioctl(fd, CIOCCRYPT, &cryp)) {
			perror("ioctl(CIOCCRYPT)");
			return 1;
		}
        
        	if (insist_write(sd, temp, 256) != 256) {
				perror("write to server failed");
				break;
	    	}
        	
		if (insist_read(sd, buf, 256) != 256) break;
		
		cryp.src = buf;
		cryp.dst = temp;
		cryp.len = sizeof(buf);
		cryp.op = COP_DECRYPT;
		
		if (ioctl(fd, CIOCCRYPT, &cryp)) {
			perror("ioctl(CIOCCRYPT)");
			return 1;
		}
		
		printf("Remote: %s\n",temp);
	}
	
	if (ioctl(fd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}
	if (close(fd) < 0) {
		perror("close(fd)");
		return 1;
	}
	if (close(sd) < 0)
		perror("close");
	
	return 0;
}
