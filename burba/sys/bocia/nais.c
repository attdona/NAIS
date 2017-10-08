#include "nais.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

// the protobuffer serialization buffer
static unsigned char protobuf[256];

const ProtobufCMessageDescriptor *md[EVENT_TYPE] = { &profile__descriptor,
		&config__descriptor, &ack__descriptor, &secret__descriptor,
		&command__descriptor, &event__descriptor };

#define FREE_PROTOB(obj) \
protobuf_c_message_free_unpacked ((ProtobufCMessage*)obj, NULL)

/**
 * @brief build a bocia payload from a protobuf object
 *
 * @param[in]  type packet type
 * @param[in]  pointer to a protobuf object
 * @param[out] payload pointer to the bocia packed array
 *
 * @return payload length
 */
int nais_marshall(proto_msg_t type, void* obj, unsigned char **payload);

void nais_free(void *obj) {
	FREE_PROTOB(obj);
}

int8_t nais_setup_header(proto_msg_t msg_type, int msg_len, uint8_t* buff) {
	int8_t hlen = 6; // min header length
	int8_t len_index = 5;
	int len = msg_len;

	buff[0] = RECORD_START;
	buff[1] = msg_type.type;
	buff[2] = 0;
	buff[3] = msg_type.dline;
	buff[4] = msg_type.flags;

	do {
		buff[len_index] = len % 128;
		len = len / 128;
		if (len > 0) {
			hlen++;
			buff[len_index] += 128;
		}
		len_index++;
	} while (len);

	buff[msg_len + hlen] = RECORD_END;
	return hlen;
}

int nais_marshall(proto_msg_t type, void* obj, unsigned char **payload) {

	int blen; // payload length
	int hlen; // header length
	unsigned char *buffptr;

	blen = protobuf_c_message_get_packed_size((const ProtobufCMessage*) obj);

	hlen = nais_setup_header(type, blen, protobuf);

	buffptr = protobuf + hlen;

	protobuf_c_message_pack((const ProtobufCMessage*) obj, buffptr);

	*payload = protobuf;
	return blen + hlen + 1;
}

proto_msg_t nais_unmarshall(unsigned char *data, void **obj) {
	unsigned char *cptr = data;
	int len = 0;
	int multiplier = 1;
	uint8_t type;

	proto_msg_t ptype;

	DEBUG("<-in-- ");

	ptype.type = *cptr;
	DEBUG("[%02x] ", *cptr);

	cptr++;
	DEBUG("[%02x] ", *cptr);

	// packet.sline is the response dline
	ptype.dline = *cptr;
	cptr++;
	DEBUG("[%02x] ", *cptr);

	// packet dline unused
	cptr++;
	DEBUG("[%02x] ", *cptr);

	ptype.flags = *cptr;

	do {
		cptr++;
		DEBUG("[%02x] ", *cptr);

		len += (*cptr & 127) * multiplier;
		multiplier *= 128;
	} while (*cptr & 128);

	cptr++;

#ifdef DEBUG
	for (uint16_t i = 0; i < len+1; i++) {
		if (i%16 == 0) {
			DEBUG("\n<-in-- ");
		}
		DEBUG("[%02x] ", cptr[i]);
	}

	DEBUG("\nmsg type: [%d] (len: %d)\n", ptype.type, len);
#endif

	type = ptype.type;
	if (ptype.type > EVENT_TYPE) {
		type = COMMAND_TYPE;
	}

	*obj = protobuf_c_message_unpack(md[type-1], NULL, len, cptr);

	if (*obj == 0) {
		DEBUG("recv malformed message\n");
	}

	return ptype;

}

int8_t is_nais_payload(unsigned char* buff, int len) {

	if (buff[0] == RECORD_START && buff[len - 1] == RECORD_END) {
		return 1;
	}

	DEBUG("ignoring pkt: [%.*s] (len: %d)\n", len, buff, len);
	return 0;
}
