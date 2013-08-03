
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <assert.h>
#include <stdlib.h>

#include "main.h"
#include "ipsec_key.h"
#include "ioctl_msg.h"

#define ASSERT assert
#define BUFLEN 256		/* input buffer length to start with */
#define IP_ADDR_LEN 4


/* proto type */
int load_data(void *usedata, FILE *file);
int load_data2(void *usedata, FILE *file);
static int process_load_key(PIPSEC_INFO ipsec_info, char *key, char *data);
static int process_load_init_info(PINIT_INFO init_info, char *key, char *data);
int load_int(const char *data, int *x);
int load_bool(const char *data, int *x);
int load_ip_address(const char *data, struct in_addr *addr);
int load_net_address(char *data, struct in_addr *addr, struct in_addr *mask);
int load_ip_prefix(char *data, struct in_addr *addr, int *prefix_len);
int load_hex_table(const char *data, unsigned char *table, int maxlen, int *length);
int load_char_table(const char *data, char *table, int maxlen);
int get_hex_digit(const char c);
size_t mn_strlcpy(char *dst, const char *src, size_t size);


/* Load data from an ASCII file
 *
 * Gets the keyword and data data from each line and calls a user-specified
 * function to process the line
 *
 * INPUT VALUES:
 *
 *    ) Userdata may be anything, just passed to the processing function
 *    ) File must not be NULL, internal error, ASSERT used
 * M01) Not enough memory for buffer
 * M02) Blank lines
 * M03) Comment in the beginning of the line
 * M04) Comment not in the beginning of the line
 * M05) Configuration entry before a comment
 * M06) The line is longer than the default buffer size
 * M07) Spaces before the keyword
 *
 */

int load_data(void *usedata, FILE *file)
{
	PIPSEC_INFO ipsec_info = (PIPSEC_INFO)usedata;
	int i, result, len, flag, bufsize;
	char *buf, *key, *data, *temp;

	ASSERT(file != NULL);
	bufsize = BUFLEN;
	buf = (char*)malloc(bufsize);
	if (buf == NULL) {
	  printf(
			"load_data: Not enough memory for buffer "
			"(%d bytes)!\n", bufsize);
		return FALSE;
	}
	do {
		if (fgets(buf, bufsize, file) == NULL) break;
		len = strlen(buf);
		ASSERT(len <= bufsize - 1);
		while (buf[len - 1] != '\n') {
			bufsize *= 2;
			temp = (char*)realloc(buf, bufsize);
			if (temp == NULL) {
				printf(
					"load_data: Not enough memory for "
					"buffer (%d bytes)!\n", bufsize);
				free(buf);
				return FALSE;
			}
			buf = temp;
			if (fgets(buf + len, bufsize - len, file) == NULL) {
				break;
			}
			len = strlen(buf);
			ASSERT(len <= bufsize - 1);
			if (len < bufsize - 1 && buf[len - 1] != '\n') {
				printf( "load_data: Binary file "
					"containing string terminator or too "
					"long line\n");
				free(buf);
				return FALSE;
			}
		}
		if (buf[len - 1] != '\n') {
			printf(
				"load_data: Line is not terminated with a "
				"linefeed\n");
			free(buf);
			return FALSE;
		}
		buf[len - 1] = '\0'; /* remove trailing newline */
		data = NULL;
		flag = FALSE;
		for (i = 0; i < strlen(buf); i++) {
			if (buf[i] != ' ') break;
		}
		key = buf + i;
		for (i = 0; i < strlen(key); i++) {
			if (key[i] == '"') {
				flag = !flag;
			}
			if (key[i] == '#' && flag == FALSE) {
				key[i] = '\0';
				break;
			}
		}
		for (i = 0; i < strlen(key); i++) {
			if (key[i] == ' ' || key[i] == '\t') {
				key[i] = '\0';
				data = key + i + 1;
				break;
			}
		}
		if (strlen(key) != 0) {
			result = process_load_key(ipsec_info, key, data);
			if (result < 0) {
				printf(
					"load_data: key = %s, data = %s : ",
					key, data);
				printf( "result = %d\n", result);
				break;
			}
			if (result > 0) {
				free(buf);
				return TRUE;
			}
		}
	} while (1);
	free(buf);
	return FALSE;
}

int load_data2(void *usedata, FILE *file)
{
	PINIT_INFO init_info = (PINIT_INFO)usedata;
	int i, result, len, flag, bufsize;
	char *buf, *key, *data, *temp;

	ASSERT(file != NULL);
	bufsize = BUFLEN;
	buf = (char*)malloc(bufsize);
	if (buf == NULL) {
	  printf(
			"load_data: Not enough memory for buffer "
			"(%d bytes)!\n", bufsize);
		return FALSE;
	}
	do {
		if (fgets(buf, bufsize, file) == NULL) break;
		len = strlen(buf);
		ASSERT(len <= bufsize - 1);
		while (buf[len - 1] != '\n') {
			bufsize *= 2;
			temp = (char*)realloc(buf, bufsize);
			if (temp == NULL) {
				printf(
					"load_data: Not enough memory for "
					"buffer (%d bytes)!\n", bufsize);
				free(buf);
				return FALSE;
			}
			buf = temp;
			if (fgets(buf + len, bufsize - len, file) == NULL) {
				break;
			}
			len = strlen(buf);
			ASSERT(len <= bufsize - 1);
			if (len < bufsize - 1 && buf[len - 1] != '\n') {
				printf( "load_data: Binary file "
					"containing string terminator or too "
					"long line\n");
				free(buf);
				return FALSE;
			}
		}
		if (buf[len - 1] != '\n') {
			printf(
				"load_data: Line is not terminated with a "
				"linefeed\n");
			free(buf);
			return FALSE;
		}
		buf[len - 1] = '\0'; /* remove trailing newline */
		data = NULL;
		flag = FALSE;
		for (i = 0; i < strlen(buf); i++) {
			if (buf[i] != ' ') break;
		}
		key = buf + i;
		for (i = 0; i < strlen(key); i++) {
			if (key[i] == '"') {
				flag = !flag;
			}
			if (key[i] == '#' && flag == FALSE) {
				key[i] = '\0';
				break;
			}
		}
		for (i = 0; i < strlen(key); i++) {
			if (key[i] == ' ' || key[i] == '\t') {
				key[i] = '\0';
				data = key + i + 1;
				break;
			}
		}
		if (strlen(key) != 0) {
			result = process_load_init_info(init_info, key, data);
			if (result < 0) {
				printf(
					"load_data: key = %s, data = %s : ",
					key, data);
				printf( "result = %d\n", result);
				break;
			}
			if (result > 0) {
				free(buf);
				return TRUE;
			}
		}
	} while (1);
	free(buf);
	return FALSE;
}

static int process_load_init_info(PINIT_INFO init_info, char *key, char *data)
{
  int tmp;
  int namelen;
 
  if(strcmp(key,"REAL_DEV") == 0) {
    if (load_hex_table(data, init_info->real_dev_name, DEV_NAME_LEN, 
		       (int*)&namelen) == TRUE) {
      assert(namelen >= 0);
      assert(namelen <= MAXKEYLEN);
      init_info->real_dev_name[namelen]= 0x00;
      return 0;
    }
    return -1;
  }
  
  if (strcmp(key, "NatTraversal") == 0) {
    if (load_int(data, (int*)&init_info->nat_info.enable) == TRUE) return 0;
    return -1;
  }

  if (strcmp(key, "UDP_OWN_PORT") == 0) {
    if (load_int(data, (int*)&tmp) == TRUE) {
      init_info->nat_info.own_port = (u_short)tmp;      
      return 0;
    }
    return -1;
  }

  if (strcmp(key, "UDP_REMORT_PORT") == 0) {
    if (load_int(data, (int*)&tmp) == TRUE) {
      init_info->nat_info.remort_port = (u_short)tmp;      
      return 0;
    }
    return -1;
  }
  
  if (strcmp(key, "END") == 0) return 1;
  return -1;
}


/* Process loading of the mn_data
 * Return values: -2: consistency error, -1: error, 0: ok, 1: end */
static int process_load_key(PIPSEC_INFO ipsec_info, char *key, char *data)
{
  struct set_ipsec *set = &ipsec_info->set;

	if (strcmp(key, "IPSecMode") == 0) {
		if (load_int(data, (int*)&set->mode) == TRUE) return 0;
		return -1;
	}

	if (strcmp(key, "IPSecProtocol") == 0) {
		if (load_int(data, (int*)&set->protocol) == TRUE) return 0;
		return -1;
	}

	if (strcmp(key, "Enc_Algorithm") == 0) {
		if (load_int(data, (int*)&set->ESP_algo) == TRUE) return 0;
		return -1;
	}

	if (strcmp(key, "Auth_Algorithm") == 0) {
		if (load_int(data, (int*)&set->AH_algo) == TRUE) return 0;
		return -1;
	}

	if (strcmp(key, "KeySetMode") == 0) {
		if (load_int(data, (int*)&set->key_mode) == TRUE) return 0;
		return -1;
	}

	if (strcmp(key, "LifeTime") == 0) {
	  if (load_int(data, (int*)&set->lifeTime) == TRUE) return 0;
	  return -1;
	}


	if (strcmp(key, "EncKey") == 0) {
	  if (load_hex_table(data, set->key.enc_key_val, MAXKEYLEN, 
			     (int*)&set->key.enc_key_len) == TRUE) {
	    assert(set->key.enc_key_len >= 0);
	    assert(set->key.enc_key_len <= MAXKEYLEN);
	    return 0;
	  }
	  return -1;
	}


	if (strcmp(key, "AuthKey") == 0) {
	  if (load_hex_table(data, set->key.auth_key_val, MAXKEYLEN, 
			     (int*)&set->key.auth_key_len) == TRUE) {
	    assert(set->key.auth_key_len >= 0);
	    assert(set->key.auth_key_len <= MAXKEYLEN);
	    return 0;
	  }
	  return -1;
	}

	if (strcmp(key, "SPI") == 0) {
		if (load_int(data, &set->key.spi) == TRUE) {
		  return 0;
		}
		return -1;
	}

	if (strcmp(key, "KeyDirection") == 0) {
		if (load_int(data, &set->direction) == TRUE){
		  return 0;
		}
		return -1;
	}


	if (strcmp(key, "InnerSrcIP") == 0) {
	  if (load_ip_address(data, (struct in_addr*)&set->src_ip) == TRUE) 
	    return 0; 
	  return -1;
	}

	if (strcmp(key, "InnerDstIP") == 0) {
	  if (load_ip_address(data, (struct in_addr*)&set->dst_ip) == TRUE) 
	    return 0; 
	  return -1;
	}

	if (strcmp(key, "InnerIPMask") == 0) {
	  if (load_ip_address(data, (struct in_addr*)&set->ip_mask) == TRUE) 
	    return 0; 
	  return -1;
	}

	if (strcmp(key, "OuterSrcIP") == 0) {
	  if (load_ip_address(data, (struct in_addr*)&set->tun_src) == TRUE) 
	    return 0; 
	  return -1;
	}

	if (strcmp(key, "OuterDstIP") == 0) {
	  if (load_ip_address(data, (struct in_addr*)&set->tun_dst) == TRUE) 
	    return 0; 
	  return -1;
	}

	if (strcmp(key, "END") == 0) return 1;
	return -1;

}




/* Load integer
 *
 * Integers starting with:
 * - "0x" are considered to be hexadecimal.
 * - "0" are considered to be octal.
 * - anything else is considered to be decimal.
 *
 * INPUT VALUES:
 *
 *    ) data must not be NULL
 *    ) x must not be NULL
 * M08) Hexadecimal number
 * M09) Octal number
 * M10) Decimal number
 * M11) Something else
 */

int load_int(const char *data, int *x)
{
	const char *pos = data;

	ASSERT(data != NULL);
	ASSERT(x != NULL);

	*x = 0;

	while (*pos == ' ' || *pos == '\t')
		pos++;

	if (strncmp(pos, "0x", 2) == 0) {
		if (sscanf(pos, "%x", x) != 1) return FALSE;
	} else if (strncmp(pos, "0", 1) == 0) {
		if (sscanf(pos, "%o", x) != 1) return FALSE;
	} else {
		if (sscanf(pos, "%d", x) != 1) return FALSE;
	}

	return TRUE;
}

/* Load boolean
 *
 * Boolean can be either "TRUE" or "FALSE"
 *
 * INPUT VALUES:
 *
 *    ) data must not be NULL
 *    ) x must not be NULL
 * M12) TRUE
 * M13) FALSE
 * M14) Something else
 */

int load_bool(const char *data, int *x)
{
	const char *pos = data;

	ASSERT(data != NULL);
	ASSERT(x != NULL);
	*x = FALSE;

	while (*pos == ' ' || *pos == '\t')
		pos++;

	if (strcmp(pos, "TRUE") == 0) {
		*x = TRUE;
		return TRUE;
	}
	if (strcmp(pos, "FALSE") == 0) {
		return TRUE;
	}
	return FALSE;
}

/* Load ip address
 *
 * INPUT VALUES:
 *
 * IP address is in the IPv4 format, a.b.c.d
 *
 *    ) data must not be NULL
 *    ) addr must not be NULL
 * M15) Valid IP address
 * M16) IP byte is not a number
 * M17) IP byte higher than 255
 * M18) IP byte lower than 0
 * M19) No separating dot
 * M20) Too few IP bytes
 * M21) Too many IP bytes
 * M22) Something else
 */

int load_ip_address(const char *data, struct in_addr *addr)
{
	int i, tmp;
	unsigned int ip;
	const char *pos;

	ASSERT(data != NULL);
	ASSERT(addr != NULL);
	pos = data;
	while (*pos == ' ' || *pos == '\t')
		pos++;
	ip = 0;
	for (i = 0; ; i++) {
		if (sscanf(pos, "%d", &tmp) != 1) {
			printf(
				"load_ip_address: Unable to parse IP byte "
				"from string '%s'\n", pos);
			return FALSE;
		}
		if (tmp > 255) {
			printf(
				"load_ip_address: Invalid IP byte %d higher "
				"than 255\n", tmp);
			return FALSE;
		}
		if (tmp < 0) {
			printf(
				"load_ip_address: Invalid IP byte %d lower "
				"than 0\n", tmp);
			return FALSE;
		}
		ASSERT(tmp >= 0);
		ASSERT(tmp <= 255);
		ip = (ip << 8) | tmp;
		pos = pos + strspn(pos, "0123456789");
		if (*pos == '\0' || *pos == ' ' || *pos == '\t') {
			if (i == IP_ADDR_LEN - 1) break;
			printf(
				"load_ip_address: Separating dot not found\n");
			return FALSE;
		}
		if (*pos == '.') {
			if (i == IP_ADDR_LEN - 1) {
				printf(
					"load_ip_address: Too many IP "
					"bytes, only %u expected\n",
					IP_ADDR_LEN);
				return FALSE;
			}
			pos++;
		} else {
			printf(
				"load_ip_address: IP byte is not a number\n");
			return FALSE;
		}
	}
	addr->s_addr = htonl(ip);
	return TRUE;
}

/**
 * load_ip_prefix:
 * @char: Any IP address with or without a mask. Valid input string is any
 *        of the following: 
 *          a.b.c.d  
 *          a.b.c.d/n   (0 <= n <= 32)
 * @addr:
 * @prefix_len:
 */
int load_ip_prefix(char *data, struct in_addr *addr, int *prefix_len)
{
	char *pos;

	pos = strchr(data, '/');
	if (pos == NULL) {
		if (load_ip_address(data, addr) != TRUE) {
			printf(
				"load_net_address: invalid IP address\n");
			return FALSE;
		}
		*prefix_len = 32;
	} else {
		*pos = '\0';
		if (load_ip_address(data, addr) != TRUE) {
			printf(
				"load_ip_prefix: invalid IP address\n");
			return FALSE;
		}
		pos++;
		if (!(sscanf(pos, "%d", prefix_len) == 1 &&
		    *prefix_len >= 0 && *prefix_len <= 32)) {
			printf(
				"load_ip_prefix: invalid network prefix\n");
			return FALSE;
		}
	}
	return TRUE;
}
/* Load hex table
 *
 * Reads hex table into the array and optionally sets the length read
 *
 * INPUT VALUES:
 *
 *    ) data must not be NULL
 *    ) table must not be NULL
 * M23) Valid hex table
 * M24) Invalid odd character
 * M25) Invalid even character
 * M26) Incomplete hex byte
 * M27) Too long hex value
 * M28) Length is NULL
 * M29) Something else
 */

int load_hex_table(const char *data, unsigned char *table, int maxlen,
		   int *length)
{
	int i, tmp, res;
	char c;

	ASSERT(data != NULL);
	ASSERT(table != NULL);
	if (length != NULL) {
		*length = 0;
	}
	for (i = 0; i < maxlen; i++) {
		table[i] = '\0';
	}
	while (*data == ' ' || *data == '\t') data++;
	if (strncmp(data, "0x", 2) == 0) {
		data += 2;
	} else if (*data == '"') {
		res = load_char_table(data, (char *)table, maxlen);
		if (length != NULL && res == TRUE) {
			table[maxlen - 1] = '\0';
			*length = strlen((char *)table);
		}
		return res;
	}
	for (i = 0; i < maxlen; i++) {
		do {
			c = *data++;
		} while (c == ' ' || c == '\t');
		if (c == '\0') break;
		tmp = get_hex_digit(c);
		if (tmp == -1) {
			printf(
				"load_hex_table: invalid character '%c'\n",
				c);
			return FALSE;
		}
		res = tmp * 16;
		c = *data++;        /* no spaces allowed in one hex byte */
		if (c == '\0') {
			printf(
				"load_hex_table: incomplete hex byte\n");
			return FALSE;
		}
		tmp = get_hex_digit(c);
		if (tmp == -1) {
			printf(
				"load_hex_table: invalid character '%c'\n",
				c);
			return FALSE;
		}
		res += tmp;
		table[i] = res;
	}
	if (i == maxlen) {
		do {
			c = *data++;
		} while (c == ' ' || c == '\t');
		if (c != '\0') {
			printf(
				"load_hex_table: too long hex value\n");
			return FALSE;
		}
	}
	if (length != NULL) {
		*length = i;
	}
	return TRUE;
}

/* Load character table (string)
 *
 * NOTE! The routine loads actually a table of characters.
 * If the string is as long as the buffer, the string will not be
 * null-terminated! The user of the routine has to make sure that the
 * string is null-terminated if required.
 *
 * INPUT VALUES:
 *
 *    ) data must not be NULL
 *    ) table must not be NULL
 * M30) Valid string
 * M31) 2 x '"' in string
 * M32) only one '"' in string
 * M33) '"' in the middle string
 * M34) '\n' in string
 * M35) '\r' in string
 * M36) '\\' in string
 * M37) '\"' in string
 * M38) '\oct' in string
 * M39) Too long string
 */

int load_char_table(const char *data, char *table, int maxlen)
{
	int i, j, startpos;
	char temp[5];
	int val, write, len;
	char ch;
	int start, stop;

	ASSERT(data != NULL);
	ASSERT(table != NULL);
	for (i = 0 ; i < maxlen ; i++) {
		table[i] = '\0';
	}
	write = 0;
	start = FALSE;
	stop = FALSE;
	startpos = 0;
	while (data[startpos] == ' ' || data[startpos] == '\t')
		startpos++;
	len = strlen(data) - startpos;
	for (i = startpos; i <= len; i++) {
		val = data[i];
		if (val == '\0') {
			if (start == TRUE && stop == TRUE) break;
			printf(
				"load_char_table: Unexpected end of string\n");
			return FALSE;
		}
		if (val == '"') {
			if (start == FALSE) {
				start = TRUE;
				continue;
			} else if (stop == FALSE) {
				stop = TRUE;
				continue;
			} else {
				printf(
					"load_char_table: Unexpected '\"'\n");
				return FALSE;
			}
		} else {
			if (start == FALSE) {
				printf(
					"load_char_table: Unrecognized "
					"character '%c'\n",
					val);
				return FALSE;
			}
		}
		if (val == '\\') {
			if (i == len - 1) {
				printf(
					"load_char_table: String longer "
					"than expected\n");
				return FALSE;
			}
			i++;
			if (data[i] == '0') {
				ch = '\0';
			} else if (data[i] == 'n') {
				ch = '\n';
			} else if (data[i] == 'r') {
				ch = '\r';
			} else if (data[i] == '\\') {
				ch = '\\';
			} else if (data[i] == '"') {
				ch = '"';
			} else {
				if (i >= len - 3) {
					printf(
						"load_char_table: String "
						"longer than expected\n");
					return FALSE;
				}
				for (j = 0 ; j < 3 ; j++) {
					ch = data[i];
					i++;
					if (ch < '0' || ch > '9') {
						printf(
							"load_char_table: "
							"Invalid octal "
							"number\n");
						return FALSE;
					}
					temp[j] = ch;
				}
				i--;
				temp[3] = '\0';
				if (sscanf(temp, "%03o", &val) != 1) {
					printf(
						"load_char_table: Invalid "
						"octal number\n");
					return FALSE;
				}
				ch = val;
			}
		} else {
			ch = val;
		}
		if (write == maxlen) {
			printf(
				"load_char_table: String longer than "
				"expected\n");
			return FALSE;
		}
		table[write] = ch;
		write++;
	}
	if (write < maxlen) {
		table[write] = '\0';
	}
	return TRUE;
}


int get_hex_digit(const char c)
{
        if (c >= '0' && c <= '9') return (c - '0');
        if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
        return -1;
}

/**
 * mn_strlcpy:
 * @dst: destination
 * @src: source
 * @size: maximum size of @dst
 *
 * Copy up to size-1 characters from @src to @dst and always NUL terminate
 * @dst string (as long as @size is not 0).
 *
 * Returns strlen(src); if this is >= @size, truncation occurred
 */
size_t mn_strlcpy(char *dst, const char *src, size_t size)
{
	char *d = dst;
	const char *s = src;
	size_t n;

	if (size == 0)
		return strlen(src);

	n = size - 1;
	while (n > 0) {
		if ((*d++ = *s++) == '\0')
			break;
		n--;
	}

	if (n == 0) {
		/* truncate and search the end of src */
		*d = '\0';
		while (*s != '\0')
			s++;
	}

	return (s - src);
}

