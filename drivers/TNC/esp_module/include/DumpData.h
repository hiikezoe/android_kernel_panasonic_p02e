/*
 * バイナリダンプ出力モジュール用ヘッダ
 *
 */

#ifndef DUMPDATA_H
#define DUMPDATA_H

void DumpData(char* str, u_char* buff, int len);

#define RAND_seed(buf, len)		do { } while(0)
#define RAND_bytes(buf, len)	do {			\
    struct timeval tv; \
    unsigned long long v; \
    do_gettimeofday(&tv); \
    v = tv.tv_sec * 1000 * 1000 * 10 + tv.tv_usec * 10; \
    memcpy(buf, &v, len);  \
} while (0)
#define RAND_bytes2(buf, len)	do {			\
    struct timeval tv; \
    unsigned long long v; \
    do_gettimeofday(&tv); \
    v = tv.tv_sec * 1000 * 1000 * 10 + tv.tv_usec * 10; \
    memcpy(buf, &v, 8);  \
    memcpy(buf+8, &v, len-8); \
} while (0)

#endif /* DUMPDATA_H */
