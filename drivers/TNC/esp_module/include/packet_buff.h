/*
 * packet_buff.h
 *
 */

#ifndef PACKET_BUFF_H
#define PACKET_BUFF_H

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


/* pktb_put - add data to a buffer */
unsigned char *pktb_put(struct pkt_buff *pktb, unsigned int len);

/* pktb_push - add data to the start of a buffer */
unsigned char *pktb_push(struct pkt_buff *pktb, unsigned int len);

/* pktb_pull - remove data from the start of a buffer */
unsigned char * pktb_pull(struct pkt_buff *pktb, unsigned int len);

/* pktb_reserve - adjust headroom */
void pktb_reserve(struct pkt_buff *pktb, unsigned int len);

/* allocate packet buffer */
struct pkt_buff *pktb_alloc(ADAPTER *Adapter);

/* free packet buffer */
void pktb_free(ADAPTER *Adapter, struct pkt_buff *pktb);

#endif /* not PACKET_BUFF_H */
