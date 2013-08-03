#ifndef _CPU_PAST_STAT_H
#define _CPU_PAST_STAT_H

#define CPUNUM (4)
#define FREQBUFFNUM	(25*60*20)	/* 25Hz 20min T.B.D */
#define LOADBUFFNUM	(25*60*20)	/* 25Hz 20min T.B.D */
#if 0
#define CPUSTATDBG(fmt, args...) printk(fmt, ## args)
#else
#define CPUSTATDBG(fmt, args...)
#endif

struct cpu_past_stat_load_buff_t {
	signed char buff[CPUNUM];
};

struct cpu_past_stat_load_t {
	bool is_loop;
	bool is_update;
	int num;
	int end;
	struct timespec *ts;
	struct cpu_past_stat_load_buff_t *load;
};

struct cpu_past_stat_freq_t {
	bool is_loop;
	bool is_update;
	int num;
	int end;
	struct timespec *ts;
	unsigned char *cpu;
	unsigned int *freq;	//TODO change char
};

extern bool cpu_past_stat_is_read;
extern bool cpu_past_stat_is_enable;
extern spinlock_t cpu_past_stat_freq_lock;
extern struct cpu_past_stat_freq_t cpu_past_stat_freq;

extern void cpu_past_stat_freq_rec(unsigned char, unsigned int);

#endif /* _CPU_PAST_STAT_H */
