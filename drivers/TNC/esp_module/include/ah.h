/*
 * ah.h
 *
 */

struct ah {
	u_int8_t	ah_nxt;		/* Next Header */
	u_int8_t	ah_len;		/* Length of data, in 32bit */
	u_int16_t	ah_reserve;	/* Reserved for future use */
	u_int32_t	ah_spi;		/* Security parameter index */
	/* variable size, 32bit bound*/	/* Authentication data */
};

struct newah {
	u_int8_t	ah_nxt;		/* Next Header */
	u_int8_t	ah_len;		/* Length of data + 1, in 32bit */
	u_int16_t	ah_reserve;	/* Reserved for future use */
	u_int32_t	ah_spi;		/* Security parameter index */
	u_int32_t	ah_seq;		/* Sequence number field */
	/* variable size, 32bit bound*/	/* Authentication data */
};

struct ah_algorithm_state {
	struct secasvar *sav;
	void* foo;	/*per algorithm data - maybe*/
};


#define	AH_MAXSUMSIZE	16

int ah_output( PIPSEC, struct mbuf *, struct ipsecrequest *);
int ah_input( PIPSEC, struct mbuf * );
int ah4_calccksum( PIPSEC, struct mbuf *, caddr_t, size_t, struct secasvar * );
int ah_hmac_md5_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_hmac_md5_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_hmac_md5_result( struct ah_algorithm_state *, caddr_t );
int ah_hmac_sha1_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_hmac_sha1_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_hmac_sha1_result( struct ah_algorithm_state *, caddr_t );
int ah_sha224_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_sha224_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_sha224_result( struct ah_algorithm_state *, caddr_t );
int ah_sha256_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_sha256_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_sha256_result( struct ah_algorithm_state *, caddr_t );
#if 0
int ah_sha384_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_sha384_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_sha384_result( struct ah_algorithm_state *, caddr_t );
int ah_sha512_init( PIPSEC, struct ah_algorithm_state *, struct secasvar * );
void ah_sha512_loop( struct ah_algorithm_state *, caddr_t, size_t );
void ah_sha512_result( struct ah_algorithm_state *, caddr_t );
#endif
