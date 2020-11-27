#include "debug.h"
#include "protocol.h"
#include "csapp.h"

int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data) {
	ssize_t bytes_sent;
	/* attempt to send data */
	bytes_sent = rio_writen(fd, hdr, sizeof(JEUX_PACKET_HEADER));
	if(bytes_sent < 0) {
		debug("rio_writen header error");
		return -1;
	}
	/* convert original size to host byte order to send payload */
	uint16_t p_size = ntohs(hdr->size);
	/* check if packet has payload */
	if(p_size > 0) {
		bytes_sent = rio_writen(fd, data, p_size);
		if(bytes_sent < 0) {
			debug("rio_writen payload error");
			return -1;
		}
	}
	debug("SENDING TYPE %u, ID %u, ROLE %u, SIZE %u, SEC %u, NSEC %u", hdr->type, hdr->id,
		hdr->role, hdr->size, hdr->timestamp_sec, hdr->timestamp_nsec);
	return 0;
}

int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp) {
	ssize_t bytes_rcvd;
	/* attempt to receive data */
	bytes_rcvd = rio_readn(fd, hdr, sizeof(JEUX_PACKET_HEADER));
	if(bytes_rcvd <= 0 || bytes_rcvd != sizeof(JEUX_PACKET_HEADER)) {
		debug("rio_readn header error");
		return -1;
	}
	/* convert original size to host byte order to send payload */
	uint16_t p_size = ntohs(hdr->size);
	/* check if packet has payload */
	if(p_size > 0) {
		*payloadp = malloc(p_size);
		if(*payloadp == NULL) {
			debug("rio_readn payload malloc error");
			return -1;
		}
		bytes_rcvd = rio_readn(fd, *payloadp, p_size);
		if(bytes_rcvd <= 0) {
			debug("rio_readn payload error");
			free(payloadp);
			return -1;
		}
	}
	debug("RECEIVING TYPE %u, ID %u, ROLE %u, SIZE %u, SEC %u, NSEC %u", hdr->type, hdr->id,
		hdr->role, hdr->size, hdr->timestamp_sec, hdr->timestamp_nsec);
	return 0;
}
