/*
 * ip_p.h
 *
 *
 */

#ifndef IP_P_H
#define IP_P_H

#define	u_int	unsigned int
#define	u_short	unsigned short
#define	u_char	unsigned char
#define	u_long	unsigned long
#define	size_t	unsigned int

/*
#ifdef _X86_
#define __LITTLE_ENDIAN	1
#endif

#ifdef __BIG_ENDIAN
#error "Please implement, here"
#elif __LITTLE_ENDIAN
#define	htons(a) (u_short)(0x00FF & ((a) >> 8) | 0xFF00 & ((a) << 8))
#define	ntohs(a) htons(a)
#define	htonl(a) ((0x000000FF & ((u_int)(a) >> 24)) |	\
		  (0x0000FF00 & ((u_int)(a) >>  8)) |	\
		  (0x00FF0000 & ((u_int)(a) <<  8)) |	\
		  (0xFF000000 & ((u_int)(a) << 24)))
#define	ntohl(a) htonl(a)
#else
#error "Please specify an Endian"
#endif
*/

#define LIBNET_CKSUM_CARRY(x) \
    (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))

#define	IP_MAX	1500


/* 
 * Internet address (a structure for historical reasons) 
 *   
struct in_addr {
  ULONG s_addr;
};
*/

/* 
 * Socket address, internet style.
 *   
struct sockaddr_in {
  u_char  sin_len;
  u_char  sin_family;
  u_short sin_port;
  struct  in_addr sin_addr;
  char    sin_zero[8]; 
}; 
*/

/*
 * Structure of an internet header, naked of options.
 */
struct ip {
#ifdef BIGINDIAN
        u_char   ip_v:4,                 /* version */
                ip_hl:4;                /* header length */
#else
    u_char   ip_hl:4,                /* header length */
  		 ip_v:4;                 /* version */
#endif

        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
}__attribute__((packed));

/* for DIGA @yoshino */
#ifndef TNC_KERNEL_2_6
/*
 * Udp protocol header.
 * Per RFC 768, September, 1981.
 */
struct udphdr {
        u_short uh_sport;               /* source port */
        u_short uh_dport;               /* destination port */
        u_short uh_ulen;                /* udp length */
        u_short uh_sum;                 /* udp checksum */
}__attribute__((packed));
#endif



struct udp_packet {
	struct	udphdr	head;
	u_char	data[IP_MAX-sizeof(struct udphdr)];
}__attribute__((packed));

 
struct ip_packet {
	struct	ip head;
	struct	udp_packet	udp;
}__attribute__((packed));

 
struct udpsum {
	u_long	s_ipaddr;
	u_long	d_ipaddr;
	u_char	zero;
	u_char	proto;
	u_short	udp_len;
	struct	udp_packet	udp;
}__attribute__((packed));



#define	UDP_LEN		8
#define	IP_LEN		20

#define	UDP_HEADER_LEN	8
#define	IP_HEADER_LEN	20
#define	IPPROTO_UDP	17
#define	IPVERSION	4
#define	MAXTTL		255

#define       IP_MAXPACKET    65535           /* maximum packet size */

#define PORT_DOMAIN    53

#define IPPROTO_IP              0               /* dummy for IP */ 
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* group mgmt protocol */  
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */  
#define IPPROTO_IPIP            4               /* IP encapsulation in IP */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_ST              7               /* Stream protocol II */
#define IPPROTO_EGP             8               /* exterior gateway protocol */
#define IPPROTO_PIGP            9               /* private interior gateway */
#define IPPROTO_RCCMON          10              /* BBN RCC Monitoring */
#define IPPROTO_NVPII           11              /* network voice protocol*/
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_ARGUS           13              /* Argus */
#define IPPROTO_EMCON           14              /* EMCON */
#define IPPROTO_XNET            15              /* Cross Net Debugger */
#define IPPROTO_CHAOS           16              /* Chaos*/ 
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_MUX             18              /* Multiplexing */
#define IPPROTO_MEAS            19              /* DCN Measurement Subsystems */
#define IPPROTO_HMP             20              /* Host Monitoring */
#define IPPROTO_PRM             21              /* Packet Radio Measurement */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_TRUNK1          23              /* Trunk-1 */  
#define IPPROTO_TRUNK2          24              /* Trunk-2 */
#define IPPROTO_LEAF1           25              /* Leaf-1 */
#define IPPROTO_LEAF2           26              /* Leaf-2 */
#define IPPROTO_RDP             27              /* Reliable Data */
#define IPPROTO_IRTP            28              /* Reliable Transaction */
#define IPPROTO_TP              29              /* tp-4 w/ class negotiation */
#define IPPROTO_BLT             30              /* Bulk Data Transfer */
#define IPPROTO_NSP             31              /* Network Services */
#define IPPROTO_INP             32              /* Merit Internodal */ 
#define IPPROTO_SEP             33              /* Sequential Exchange */  
#define IPPROTO_3PC             34              /* Third Party Connect */
#define IPPROTO_IDPR            35              /* InterDomain Policy Routing */
#define IPPROTO_XTP             36              /* XTP */
#define IPPROTO_DDP             37              /* Datagram Delivery */
#define IPPROTO_CMTP            38              /* Control Message Transport */
#define IPPROTO_TPXX            39              /* TP++ Transport */
#define IPPROTO_IL              40              /* IL transport protocol */
#define IPPROTO_SIP             41              /* Simple Internet Protocol */
#define IPPROTO_SDRP            42              /* Source Demand Routing */
#define IPPROTO_SIPSR           43              /* SIP Source Route */
#define IPPROTO_SIPFRAG         44              /* SIP Fragment */
#define IPPROTO_IDRP            45              /* InterDomain Routing*/
#define IPPROTO_RSVP            46              /* resource reservation */
#define IPPROTO_GRE             47              /* General Routing Encap. */
#define IPPROTO_MHRP            48              /* Mobile Host Routing */
#define IPPROTO_BHA             49              /* BHA */
#define IPPROTO_ESP             50              /* SIPP Encap Sec. Payload */
#define IPPROTO_AH              51              /* SIPP Auth Header */
#define IPPROTO_INLSP           52              /* Integ. Net Layer Security */
#define IPPROTO_SWIPE           53              /* IP with encryption */
#define IPPROTO_NHRP            54              /* Next Hop Resolution */
/* 55-60: Unassigned */
#define IPPROTO_AHIP            61              /* any host internal protocol */
#define IPPROTO_CFTP            62              /* CFTP */
#define IPPROTO_HELLO           63              /* "hello" routing protocol */
#define IPPROTO_SATEXPAK        64              /* SATNET/Backroom EXPAK */
#define IPPROTO_KRYPTOLAN       65              /* Kryptolan */
#define IPPROTO_RVD             66              /* Remote Virtual Disk */
#define IPPROTO_IPPC            67              /* Pluribus Packet Core */
#define IPPROTO_ADFS            68              /* Any distributed FS */
#define IPPROTO_SATMON          69              /* Satnet Monitoring */
#define IPPROTO_VISA            70              /* VISA Protocol */
#define IPPROTO_IPCV            71              /* Packet Core Utility */
#define IPPROTO_CPNX            72              /* Comp. Prot. Net. Executive */
#define IPPROTO_CPHB            73              /* Comp. Prot. HeartBeat */
#define IPPROTO_WSN             74              /* Wang Span Network */
#define IPPROTO_PVP             75              /* Packet Video Protocol */
#define IPPROTO_BRSATMON        76              /* BackRoom SATNET Monitoring */
#define IPPROTO_ND              77              /* Sun net disk proto (temp.) */
#define IPPROTO_WBMON           78              /* WIDEBAND Monitoring */
#define IPPROTO_WBEXPAK         79              /* WIDEBAND EXPAK */
#define IPPROTO_EON             80              /* ISO cnlp */
#define IPPROTO_VMTP            81              /* VMTP */
#define IPPROTO_SVMTP           82              /* Secure VMTP */
#define IPPROTO_VINES           83              /* Banyon VINES */
#define IPPROTO_TTP             84              /* TTP */
#define IPPROTO_IGP             85              /* NSFNET-IGP */
#define IPPROTO_DGP             86              /* dissimilar gateway prot. */
#define IPPROTO_TCF             87              /* TCF */
#define IPPROTO_IGRP            88              /* Cisco/GXS IGRP */
#define IPPROTO_OSPFIGP         89              /* OSPFIGP */
#define IPPROTO_SRPC            90              /* Strite RPC protocol */
#define IPPROTO_LARP            91              /* Locus Address Resoloution */
#define IPPROTO_MTP             92              /* Multicast Transport */
#define IPPROTO_AX25            93              /* AX.25 Frames */
#define IPPROTO_IPEIP           94              /* IP encapsulated in IP */
#define IPPROTO_MICP            95              /* Mobile Int.ing control */
#define IPPROTO_SCCSP           96              /* Semaphore Comm. security */
#define IPPROTO_ETHERIP         97              /* Ethernet IP encapsulation */
#define IPPROTO_ENCAP           98              /* encapsulation header */
#define IPPROTO_APES            99              /* any private encr. scheme */
#define IPPROTO_GMTP            100             /* GMTP*/
/* 101-254: Partly Unassigned */
#define IPPROTO_PGM             113             /* PGM */
/* 255: Reserved */
/* BSD Private, local use, namespace incursion */
#define IPPROTO_DIVERT          254             /* divert pseudo-protocol */
#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

#define IPPROTO_IPV4            IPPROTO_IPIP    /* for compatibility */

u_short in_cksum(u_short *,int );
int libnet_in_cksum(u_short *, int);

#endif /* !IP_P_H */
