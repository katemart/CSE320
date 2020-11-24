#include "debug.h"
#include "protocol.h"
#include "csapp.h"

int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data) {
	ssize_t bytes_sent;
	/* store vals (that need conversion) in network byte order */
	hdr->size = htons(hdr->size);
	hdr->timestamp_sec = htonl(hdr->timestamp_sec);
	hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);
	/* attempt to send data */
	bytes_sent = rio_writen(fd, hdr, sizeof(JEUX_PACKET_HEADER));
	if(bytes_sent < 0) {
		debug("rio_writen header error");
		return -1;
	}
	/* check if packet has payload */
	if(hdr->size > 0) {
		bytes_sent = rio_writen(fd, data, hdr->size);
		if(bytes_sent < 0) {
			debug("rio_writen payload error");
			return -1;
		}
	}
	return 0;
}

int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp) {
	ssize_t bytes_rcvd;
	/* attempt to receive data */
	bytes_rcvd = rio_readn(fd, hdr, sizeof(JEUX_PACKET_HEADER));
	if(bytes_rcvd <= 0 || bytes_rcvd != sizeof(JEUX_PACKET_HEADER)) {
		debug("rio_readn header error\n");
		return -1;
	}
	/* read vals (that need conversion) in host byte order */
	hdr->size = ntohs(hdr->size);
	hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
	hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);
	/* check if packet has payload */
	if(hdr->size > 0) {
		*payloadp = malloc(hdr->size);
		if(*payloadp == NULL) {
			debug("rio_readn payload malloc error");
			return -1;
		}
		bytes_rcvd = rio_readn(fd, *payloadp, hdr->size);
		if(bytes_rcvd <= 0) {
			debug("rio_readn payload error");
			return -1;
		}
	}
	return 0;
}
