/*
 * esp.h
 *
 */

struct newesp {
	u_int32_t	esp_spi;	/* ESP */
	u_int32_t	esp_seq;	/* Sequence number */
	/*variable size*/		/* (IV and) Payload data */
	/*variable size*/		/* padding */
	/*8bit*/			/* pad size */
	/*8bit*/			/* next header */
	/*8bit*/			/* next header */
	/*variable size, 32bit bound*/	/* Authentication data */
}__attribute__((packed));


struct esptail {
	u_int8_t	esp_padlen;	/* pad length */
	u_int8_t	esp_nxt;	/* Next header */
	/*variable size, 32bit bound*/	/* Authentication data (new IPsec)*/
}__attribute__((packed));


struct esp_algorithm_state {
	struct secasvar *sav;
	void* foo;	/*per algorithm data - maybe*/
}__attribute__((packed));





struct esp_algorithm {
	size_t padbound;	/* pad boundary, in byte */
	int ivlenval;		/* iv length, in byte */
	int keymin;	/* in bits */
	int keymax;	/* in bits */
	int (*schedlen)(const struct esp_algorithm *);
	const char *name;
	int (*decrypt)(struct mbuf *, size_t,
		struct secasvar *, const struct esp_algorithm *, int);
	int (*encrypt) (struct mbuf *, size_t, 	struct secasvar *,
			const struct esp_algorithm *, int);
	/* not supposed to be called directly */
	int (*schedule)(const struct esp_algorithm *, struct secasvar *);
	int (*blockdecrypt)(u_long *, void*, int );
	int (*blockencrypt)(u_long *, void*, int );
}__attribute__((packed));


int esp_output( PIPSEC, struct mbuf *, u_char *, struct ipsecrequest *);
int esp_input( PIPSEC, struct mbuf * );
int esp_cbc_decrypt( struct mbuf *, size_t, struct secasvar *,
		       const struct esp_algorithm *,int);
int esp_auth( PIPSEC, struct mbuf *, size_t, size_t, struct secasvar *, u_char *);
const struct esp_algorithm *esp_algorithm_lookup(int);
int esp_schedule(PIPSEC, const struct esp_algorithm *, struct secasvar *);
int esp_cbc_encrypt(struct mbuf *,size_t,struct secasvar *,
		       const struct esp_algorithm *,int);

