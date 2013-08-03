/*
 * pkt_buff.h
 *
 */

/*
 * packet buffer structure
 * 
 * head --> +--------------------------+
 *          | reserved for upper layer |
 * data --> +--------------------------+--+
 *          | data for current layer   |  |
 *          |                          |  +- len
 *          |                          |  |
 * tail --> +--------------------------+--+
 *          | reserved or truncated    |
 * end  --> +--------------------------+
 */

struct pkt_buff {
	unsigned char *data;
	unsigned int len;

	unsigned char *head;
	unsigned char *tail;
	unsigned char *end;
	unsigned int buff_len;
}__attribute__((packed));

