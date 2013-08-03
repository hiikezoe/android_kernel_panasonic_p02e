/*
 *  drivers/cpufreq/cpufreq_ondemand.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#ifdef CONFIG_CPU_PAST_STAT
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/cpu_past_stat.h>
#endif /* CONFIG_CPU_PAST_STAT */

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(10)
#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define DEF_SAMPLING_DOWN_FACTOR		(1)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL	(3)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(11)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)
#define MIN_FREQUENCY_DOWN_DIFFERENTIAL		(1)
#define DEFAULT_FREQ_BOOST_TIME                 (10000)

u64 freq_boosted_time;
u64 input_freq_boosted_time;

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

#define POWERSAVE_BIAS_MAXLEVEL			(1000)
#define POWERSAVE_BIAS_MINLEVEL			(-1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
static
#endif
struct cpufreq_governor cpufreq_gov_ondemand = {
       .name                   = "ondemand",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	unsigned int rate_mult;
	int cpu;
	unsigned int sample_type:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info);
static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct workqueue_struct *input_wq;
static struct workqueue_struct *boost_wq;

static DEFINE_PER_CPU(struct work_struct, dbs_refresh_work);
static DEFINE_PER_CPU(struct work_struct, dbs_boost_work);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int down_differential;
	unsigned int ignore_nice;
	unsigned int sampling_down_factor;
	int          powersave_bias;
	unsigned int io_is_busy;
	unsigned int boosted;
	unsigned int freq_boost_time;
	unsigned int boostfreq;
	unsigned int input_boosted;
	unsigned int input_freq_boost_time;
	unsigned int input_boostfreq;
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
	.ignore_nice = 0,
	.powersave_bias = 0,
	.boosted = 0,
	.freq_boost_time = DEFAULT_FREQ_BOOST_TIME,
	.boostfreq = 1512000,
	.input_boosted = 0,
	.input_freq_boost_time = DEFAULT_FREQ_BOOST_TIME,
	.input_boostfreq = 1512000,
};

#ifdef CONFIG_CPU_PAST_STAT
bool cpu_past_stat_is_read = false;
bool cpu_past_stat_is_enable = false;
static bool cpu_past_stat_is_init = false;
static struct proc_dir_entry *cpu_past_stat_proc_ent = NULL;
static struct cpu_past_stat_load_t cpu_past_stat_load = {
	.is_loop = false,
	.is_update = false,
	.end = -1,
	.ts = NULL,
	.load = NULL,
};
struct cpu_past_stat_freq_t cpu_past_stat_freq = {
	.is_loop = false,
	.is_update = false,
	.end = -1,
	.ts = NULL,
	.cpu = NULL,
	.freq = NULL,
};
spinlock_t cpu_past_stat_freq_lock;
#endif /* CONFIG_CPU_PAST_STAT */

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
					  unsigned int freq_next,
					  unsigned int relation)
{
	unsigned int freq_req, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	int freq_reduc;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
						   policy->cpu);

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}

static int ondemand_powersave_bias_setspeed(struct cpufreq_policy *policy,
					    struct cpufreq_policy *altpolicy,
					    int level)
{
	if (level == POWERSAVE_BIAS_MAXLEVEL) {
		/* maximum powersave; set to lowest frequency */
		__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->min : policy->min,
			CPUFREQ_RELATION_L);
		return 1;
	} else if (level == POWERSAVE_BIAS_MINLEVEL) {
		/* minimum powersave; set to highest frequency */
		__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->max : policy->max,
			CPUFREQ_RELATION_H);
		return 1;
	}
	return 0;
}

static void ondemand_powersave_bias_init_cpu(int cpu)
{
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
}

static void ondemand_powersave_bias_init(void)
{
	int i;
	for_each_online_cpu(i) {
		ondemand_powersave_bias_init_cpu(i);
	}
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_ondemand Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(down_differential, down_differential);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);
show_one(boostpulse, boosted);
show_one(boosttime, freq_boost_time);
show_one(boostfreq, boostfreq);
show_one(input_boostpulse, input_boosted);
show_one(input_boosttime, input_freq_boost_time);
show_one(input_boostfreq, input_boostfreq);

static ssize_t show_powersave_bias
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dbs_tuners_ins.powersave_bias);
}

static ssize_t store_boosttime(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.freq_boost_time = input;
	return count;
}


static ssize_t store_boostpulse(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	unsigned long val;
	int i;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	for_each_online_cpu(i) {
		queue_work_on(i, boost_wq, &per_cpu(dbs_boost_work, i));
	}

	return count;
}

static ssize_t store_boostfreq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.boostfreq = input;
	return count;
}

static ssize_t store_input_boosttime(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.input_freq_boost_time = input;
	return count;
}


static ssize_t store_input_boostpulse(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	unsigned long val;
	int i;  // TouchBoost

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	dbs_tuners_ins.input_boosted = 1;
	input_freq_boosted_time = ktime_to_us(ktime_get());

	// Begin TouchBoost changes
	for_each_online_cpu(i) {
	  queue_work_on(i, input_wq, &per_cpu(dbs_refresh_work, i));
	}
	// End TouchBoost changes
	return count;
}

static ssize_t store_input_boostfreq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.input_boostfreq = input;
	return count;
}
/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updaing
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from ondemand
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
	int cpu;

	dbs_tuners_ins.sampling_rate = new_rate
				     = max(new_rate, min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);

		if (!delayed_work_pending(&dbs_info->work)) {
			mutex_unlock(&dbs_info->timer_mutex);
			continue;
		}

		next_sampling  = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->work.timer.expires;


		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->timer_mutex);
			cancel_delayed_work_sync(&dbs_info->work);
			mutex_lock(&dbs_info->timer_mutex);

			schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work,
						 usecs_to_jiffies(new_rate));

		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	update_sampling_rate(input);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_down_differential(struct kobject *a, struct attribute *b,
		const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input >= dbs_tuners_ins.up_threshold ||
			input < MIN_FREQUENCY_DOWN_DIFFERENTIAL) {
		return -EINVAL;
	}

	dbs_tuners_ins.down_differential = input;

	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input  = 0;
	int bypass = 0;
	int ret, cpu, reenable_timer, j;
	struct cpu_dbs_info_s *dbs_info;

	struct cpumask cpus_timer_done;
	cpumask_clear(&cpus_timer_done);

	ret = sscanf(buf, "%d", &input);

	if (ret != 1)
		return -EINVAL;

	if (input >= POWERSAVE_BIAS_MAXLEVEL) {
		input  = POWERSAVE_BIAS_MAXLEVEL;
		bypass = 1;
	} else if (input <= POWERSAVE_BIAS_MINLEVEL) {
		input  = POWERSAVE_BIAS_MINLEVEL;
		bypass = 1;
	}

	if (input == dbs_tuners_ins.powersave_bias) {
		/* no change */
		return count;
	}

	reenable_timer = ((dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MAXLEVEL) ||
				(dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MINLEVEL));

	dbs_tuners_ins.powersave_bias = input;
	if (!bypass) {
		if (reenable_timer) {
			/* reinstate dbs timer */
			for_each_online_cpu(cpu) {
				if (lock_policy_rwsem_write(cpu) < 0)
					continue;

				dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

				for_each_cpu(j, &cpus_timer_done) {
					if (!dbs_info->cur_policy) {
						pr_err("Dbs policy is NULL\n");
						goto skip_this_cpu;
					}
					if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
						goto skip_this_cpu;
				}

				cpumask_set_cpu(cpu, &cpus_timer_done);
				if (dbs_info->cur_policy) {
					/* restart dbs timer */
					dbs_timer_init(dbs_info);
				}
skip_this_cpu:
				unlock_policy_rwsem_write(cpu);
			}
		}
		ondemand_powersave_bias_init();
	} else {
		/* running at maximum or minimum frequencies; cancel
		   dbs timer as periodic load sampling is not necessary */
		for_each_online_cpu(cpu) {
			if (lock_policy_rwsem_write(cpu) < 0)
				continue;

			dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

			for_each_cpu(j, &cpus_timer_done) {
				if (!dbs_info->cur_policy) {
					pr_err("Dbs policy is NULL\n");
					goto skip_this_cpu_bypass;
				}
				if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
					goto skip_this_cpu_bypass;
			}

			cpumask_set_cpu(cpu, &cpus_timer_done);

			if (dbs_info->cur_policy) {
				/* cpu using ondemand, cancel dbs timer */
				mutex_lock(&dbs_info->timer_mutex);
				dbs_timer_exit(dbs_info);

				ondemand_powersave_bias_setspeed(
					dbs_info->cur_policy,
					NULL,
					input);

				mutex_unlock(&dbs_info->timer_mutex);
			}
skip_this_cpu_bypass:
			unlock_policy_rwsem_write(cpu);
		}
	}

	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(down_differential);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(powersave_bias);
define_one_global_rw(boostpulse);
define_one_global_rw(boosttime);
define_one_global_rw(boostfreq);
define_one_global_rw(input_boostpulse);
define_one_global_rw(input_boosttime);
define_one_global_rw(input_boostfreq);

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&down_differential.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&powersave_bias.attr,
	&io_is_busy.attr,
	&boostpulse.attr,
	&boosttime.attr,
	&boostfreq.attr,
	&input_boostpulse.attr,
	&input_boosttime.attr,
	&input_boostfreq.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "ondemand",
};

/************************** sysfs end ************************/

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
	if (dbs_tuners_ins.powersave_bias)
		freq = powersave_bias_target(p, freq, CPUFREQ_RELATION_H);
	else if (p->cur == p->max)
		return;

	__cpufreq_driver_target(p, freq, dbs_tuners_ins.powersave_bias ?
			CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);
}

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	/* Extrapolated load of this CPU */
	unsigned int load_at_max_freq = 0;
	unsigned int max_load_freq;
	/* Current load across this CPU */
	unsigned int cur_load = 0;

	struct cpufreq_policy *policy;
	unsigned int j;
#ifdef CONFIG_CPU_PAST_STAT
	struct timespec ts;
	int i;
	static struct timespec ts_st;
#endif /* CONFIG_CPU_PAST_STAT */

	this_dbs_info->freq_lo = 0;
	policy = this_dbs_info->cur_policy;

	if (dbs_tuners_ins.boosted ) {
		if (ktime_to_us(ktime_get()) - freq_boosted_time >=
					dbs_tuners_ins.freq_boost_time) {
			dbs_tuners_ins.boosted = 0;
		}
	}
	if (dbs_tuners_ins.input_boosted ) {
		if (ktime_to_us(ktime_get()) - input_freq_boosted_time >=
					dbs_tuners_ins.input_freq_boost_time) {
			dbs_tuners_ins.input_boosted = 0;
		}
	}
	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

#ifdef CONFIG_CPU_PAST_STAT
	if (cpu_past_stat_is_enable && !cpu_past_stat_is_read) {
		cpu_past_stat_load.is_update = true;
		getnstimeofday(&ts);
		if (cpu_past_stat_load.end == -1) {
			cpu_past_stat_load.end = 0;
			ts_st = ts;
		}
		cpu_past_stat_load.ts[cpu_past_stat_load.end] = ts;
		for (i = 0; i < CPUNUM; i++)
			cpu_past_stat_load.load[cpu_past_stat_load.end].buff[i] = -1;
	}
#endif /* CONFIG_CPU_PAST_STAT */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int)
			(cur_wall_time - j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int)
			(cur_idle_time - j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int)
			(cur_iowait_time - j_dbs_info->prev_cpu_iowait);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			u64 cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
					 j_dbs_info->prev_cpu_nice;
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		/*
		 * For the purpose of ondemand, waiting for disk IO is an
		 * indication that you're performance critical, and not that
		 * the system is actually idle. So subtract the iowait time
		 * from the cpu idle time.
		 */

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;
#ifdef CONFIG_CPU_PAST_STAT
		if (unlikely(!wall_time || wall_time < idle_time)) {
			if (cpu_past_stat_load.is_update)
				cpu_past_stat_load.load
					[cpu_past_stat_load.end].buff[j] = 0;
			continue;
		}
#else
		if (unlikely(!wall_time || wall_time < idle_time))
			continue;
#endif /* CONFIG_CPU_PAST_STAT */

		cur_load = 100 * (wall_time - idle_time) / wall_time;
#ifdef CONFIG_CPU_PAST_STAT
		if (cpu_past_stat_load.is_update)
			cpu_past_stat_load.load[cpu_past_stat_load.end].buff[j] = cur_load;
#endif /* CONFIG_CPU_PAST_STAT */

		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = cur_load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;
	}

#ifdef CONFIG_CPU_PAST_STAT
	if (cpu_past_stat_load.is_update) {
		cpu_past_stat_load.end++;
		if (cpu_past_stat_load.end >= LOADBUFFNUM) {
			cpu_past_stat_load.end = 0;
			cpu_past_stat_load.is_loop = true;
			printk("cpu_past_stat: load collect %ld -> %ld\n",
				ts_st.tv_sec, ts.tv_sec);
			ts_st = ts;
		}
		cpu_past_stat_load.is_update = false;
	}
#endif /* CONFIG_CPU_PAST_STAT */
	/* calculate the scaled load across CPU */
	load_at_max_freq = (cur_load * policy->cur)/policy->cpuinfo.max_freq;

	cpufreq_notify_utilization(policy, load_at_max_freq);

	/* Check for frequency increase */
	if (max_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
		/* If switching to max speed, apply sampling_down_factor */
		if (policy->cur < policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;
		dbs_freq_increase(policy, policy->max);
		return;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		return;

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		unsigned int freq_next;
		freq_next = max_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		/* No longer fully busy, reset rate_mult */
		this_dbs_info->rate_mult = 1;

		if (dbs_tuners_ins.boosted || dbs_tuners_ins.input_boosted ){
		unsigned int freq_boosted;
		freq_boosted =  max(dbs_tuners_ins.boosted==1?dbs_tuners_ins.boostfreq:0,
				dbs_tuners_ins.input_boosted==1?dbs_tuners_ins.input_boostfreq:0);
                freq_boosted = freq_boosted>policy->max?policy->max:freq_boosted;
			if (freq_next < freq_boosted){
				freq_next = freq_boosted;
      }
		}

		if (freq_next < policy->min)
			freq_next = policy->min;
		if (!dbs_tuners_ins.powersave_bias) {
			__cpufreq_driver_target(policy, freq_next,
					CPUFREQ_RELATION_L);
		} else {
			int freq = powersave_bias_target(policy, freq_next,
					CPUFREQ_RELATION_L);
			__cpufreq_driver_target(policy, freq,
				CPUFREQ_RELATION_L);
		}
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int sample_type = dbs_info->sample_type;

	int delay;

	mutex_lock(&dbs_info->timer_mutex);

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (!dbs_tuners_ins.powersave_bias ||
	    sample_type == DBS_NORMAL_SAMPLE) {
		dbs_check_cpu(dbs_info);
		if (dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			dbs_info->sample_type = DBS_SUB_SAMPLE;
			delay = dbs_info->freq_hi_jiffies;
		} else {
			/* We want all CPUs to do sampling nearly on
			 * same jiffy
			 */
			delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				* dbs_info->rate_mult);

			if (num_online_cpus() > 1)
				delay -= jiffies % delay;
		}
	} else {
		__cpufreq_driver_target(dbs_info->cur_policy,
			dbs_info->freq_lo, CPUFREQ_RELATION_H);
		delay = dbs_info->freq_lo_jiffies;
	}
	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

/*
 * Not all CPUs want IO time to be accounted as busy; this dependson how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (androidlcom) calis this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) andl later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
	    boot_cpu_data.x86 == 6 &&
	    boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}

static void dbs_refresh_callback(struct work_struct *unused)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int cpu = smp_processor_id();
        unsigned int next_freq;

	get_online_cpus();

	if (lock_policy_rwsem_write(cpu) < 0)
		goto bail_acq_sema_failed;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	policy = this_dbs_info->cur_policy;
	if (!policy) {
		/* CPU not using ondemand governor */
		goto bail_incorrect_governor;
	}
#if 0
	if (policy->cur < policy->max) {
		policy->cur = policy->max;

		__cpufreq_driver_target(policy, policy->max,
					CPUFREQ_RELATION_L);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&this_dbs_info->prev_cpu_wall);
	}
#else
	dbs_tuners_ins.input_boosted = 1;
	input_freq_boosted_time = ktime_to_us(ktime_get());
        next_freq = dbs_tuners_ins.boostfreq<policy->max?
                                 dbs_tuners_ins.boostfreq:policy->max;
	if (policy->cur < next_freq) {
		policy->cur = next_freq;

		__cpufreq_driver_target(policy, next_freq,
					CPUFREQ_RELATION_L);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&this_dbs_info->prev_cpu_wall);
	}
#endif

bail_incorrect_governor:
	unlock_policy_rwsem_write(cpu);

bail_acq_sema_failed:
	put_online_cpus();
	return;
}

static void dbs_boost_callback(struct work_struct *unused)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int cpu = smp_processor_id();
        unsigned int next_freq;

	get_online_cpus();

	if (lock_policy_rwsem_write(cpu) < 0)
		goto bail_acq_sema_failed;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	policy = this_dbs_info->cur_policy;
	if (!policy) {
		/* CPU not using ondemand governor */
		goto bail_incorrect_governor;
	}

	dbs_tuners_ins.boosted = 1;
	freq_boosted_time = ktime_to_us(ktime_get());
        next_freq = dbs_tuners_ins.boostfreq<policy->max?
                                 dbs_tuners_ins.boostfreq:policy->max;

	if (policy->cur <  next_freq ){
		policy->cur = next_freq; 

		__cpufreq_driver_target(policy, next_freq,
					CPUFREQ_RELATION_L);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&this_dbs_info->prev_cpu_wall);
	}

bail_incorrect_governor:
	unlock_policy_rwsem_write(cpu);

bail_acq_sema_failed:
	put_online_cpus();
	return;
}

static void dbs_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	int i;

	if ((dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MAXLEVEL) ||
		(dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MINLEVEL)) {
		/* nothing to do */
		return;
	}

	for_each_online_cpu(i) {
		queue_work_on(i, input_wq, &per_cpu(dbs_refresh_work, i));
	}
}

static int dbs_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler dbs_input_handler = {
	.event		= dbs_input_event,
	.connect	= dbs_input_connect,
	.disconnect	= dbs_input_disconnect,
	.name		= "cpufreq_ond",
	.id_table	= dbs_ids,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice)
				j_dbs_info->prev_cpu_nice =
						kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		}
		this_dbs_info->cpu = cpu;
		this_dbs_info->rate_mult = 1;
		ondemand_powersave_bias_init_cpu(cpu);
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			dbs_tuners_ins.io_is_busy = should_io_be_busy();
		}
		if (!cpu)
			rc = input_register_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);


		if (!ondemand_powersave_bias_setspeed(
					this_dbs_info->cur_policy,
					NULL,
					dbs_tuners_ins.powersave_bias))
			dbs_timer_init(this_dbs_info);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		/* If device is being removed, policy is no longer
		 * valid. */
		this_dbs_info->cur_policy = NULL;
		if (!cpu)
			input_unregister_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		else if (dbs_tuners_ins.powersave_bias != 0)
			ondemand_powersave_bias_setspeed(
				this_dbs_info->cur_policy,
				policy,
				dbs_tuners_ins.powersave_bias);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

#ifdef CONFIG_CPU_PAST_STAT
static int cpu_past_stat_read_proc(char *buff, char **start, off_t offset,
					int count, int *peof, void *dat)
{
	int ret, i, len = 0;
	struct rtc_time tm;
	static int load_st = 0, freq_st = 0;	/* read point of buffer, -1: no data */
	static int load_end, freq_end;	/* end of buffer */
	/* cpu status */
	struct timespec ts;
	static struct cpu_past_stat_load_buff_t load = {{0,0,0,0}};
	static unsigned int freq[CPUNUM] = {0,0,0,0};
	/* buffer size of one entry */
	const static int frame =
		sizeof("[ yyyy-mm-dd hh:mm:ss.uuuuuu ] lll lll lll lll "
			"fffffff fffffff fffffff fffffff\n");

	if (!cpu_past_stat_is_init
		|| ((cpu_past_stat_load.end == -1)
			&& (cpu_past_stat_freq.end == -1))) {
		CPUSTATDBG("%s: exit cpu_past_stat_load.end=%d, "
				"cpu_past_stat_freq.end=%d\n",
				__func__,
				cpu_past_stat_load.end,
				cpu_past_stat_freq.end);
		*peof = 1;
		return len;
	}
	if (!cpu_past_stat_is_read) {
		if ((load_end == cpu_past_stat_load.end)
			&& (freq_end == cpu_past_stat_freq.end)) {
			CPUSTATDBG("%s: exit load_end=%d, cpu_past_stat_load.end=%d, "
				"freq_end=%d, cpu_past_stat_freq.end=%d\n",
				__func__,
				load_end, cpu_past_stat_load.end,
				freq_end, cpu_past_stat_freq.end);
			*peof = 1;
			return len;
		}
		while (cpu_past_stat_load.is_update
			|| cpu_past_stat_freq.is_update)	//TODO use wait or lock
			msleep(1);
		cpu_past_stat_is_read = true;
		for (i = 0; i < CPUNUM; i++) {
			//load.buff[i] = 0;
			freq[i] = 0;
		}
		/* initialize start/end point */
		/* load */
		if (cpu_past_stat_load.is_loop) {
			load_st = cpu_past_stat_load.end + 1;
			if (load_st >= LOADBUFFNUM)
				load_st = 0;
		} else {
			if (cpu_past_stat_load.end == - 1)
				load_st = -1;
			else
				load_st = 0;
		}
		load_end = cpu_past_stat_load.end;
		/* freq */
		if (cpu_past_stat_freq.is_loop) {
			freq_st = cpu_past_stat_freq.end + 1;
			if (freq_st >= FREQBUFFNUM)
				freq_st = 0;
		} else {
			if (cpu_past_stat_freq.end == -1)
				freq_st = -1;
			else
				freq_st = 0;
		}
		freq_end = cpu_past_stat_freq.end;
		CPUSTATDBG("%s: load_st=%d, load_end=%d "
			"freq_st=%d, freq_end=%d\n",
			__func__, load_st, load_end, freq_st, freq_end);
	}
	while (!((load_st == -1) && (freq_st == -1))) {
		if ((len + frame) >= count)
			break;
		/* merge load freq */
		CPUSTATDBG("%s: load_st=%d, load_end=%d "
			"freq_st=%d, freq_end=%d ",
			__func__, load_st, load_end, freq_st, freq_end);
		//for (i = 0; i < CPUNUM; i++)
		//	load.buff[i] = 0;
		if ((load_st != -1) && (freq_st != -1)){
			/* load: data exists, freq: data exists */
			CPUSTATDBG("load.ts=%lu.%06lu, freq.ts=%lu.%06lu ",
				cpu_past_stat_load.ts[load_st].tv_sec,
				cpu_past_stat_load.ts[load_st].tv_nsec/1000,
				cpu_past_stat_freq.ts[freq_st].tv_sec,
				cpu_past_stat_freq.ts[freq_st].tv_nsec/1000);
			ret = timespec_compare(&cpu_past_stat_load.ts[load_st],
				&cpu_past_stat_freq.ts[freq_st]);
			if (ret < 0) {
				/* load time < freq time */
				CPUSTATDBG("load < freq\n");
				ts = cpu_past_stat_load.ts[load_st];
				load = cpu_past_stat_load.load[load_st];
				load_st++;
			} else if (ret > 0) {
				/* load time > freq time */
				CPUSTATDBG("load > freq\n");
				ts = cpu_past_stat_freq.ts[freq_st];
				freq[(int)cpu_past_stat_freq.cpu[freq_st]] =
					cpu_past_stat_freq.freq[freq_st];
				freq_st++;
			} else if (ret == 0) {
				/* load time == freq time */
				CPUSTATDBG("load == freq\n");
				ts = cpu_past_stat_load.ts[load_st];
				load = cpu_past_stat_load.load[load_st];
				freq[(int)cpu_past_stat_freq.cpu[freq_st]] =
					cpu_past_stat_freq.freq[freq_st];
				load_st++;
				freq_st++;
			}
		} else if ((load_st != -1) && (freq_st == -1)) {
			/* load: data exists, freq: no data */
			ts = cpu_past_stat_load.ts[load_st];
			CPUSTATDBG("load.ts=%lu.%06lu\n",
				ts.tv_sec, ts.tv_nsec/1000);
			load = cpu_past_stat_load.load[load_st];
			load_st++;
		} else if ((load_st == -1) && (freq_st != -1)) {
			/* load: no data, freq: data exists */
			ts = cpu_past_stat_freq.ts[freq_st];
			CPUSTATDBG("freq.ts=%lu.%06lu\n",
				ts.tv_sec, ts.tv_nsec/1000);
			freq[(int)cpu_past_stat_freq.cpu[freq_st]] =
				cpu_past_stat_freq.freq[freq_st];
			freq_st++;
		} else {
			/* not available path */
			CPUSTATDBG("error\n");
			load_st = -1;
			freq_st = -1;
			break;
		}
		/* loop check */
		if (load_st >= LOADBUFFNUM)
			load_st = 0;
		if (load_st == load_end)	//load: no data
			load_st = -1;
		if (freq_st >= FREQBUFFNUM)
			freq_st = 0;
		if (freq_st == freq_end)	//freq: no data
			freq_st = -1;

		rtc_time_to_tm(ts.tv_sec, &tm);
		len += snprintf(buff + len, count - len ,
			"[ %d-%02d-%02d %02d:%02d:%02d.%06u ] "
			"%3hhd %3hhd %3hhd %3hhd "
			"%7u %7u %7u %7u\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			(unsigned int)ts.tv_nsec/1000,
			load.buff[0], load.buff[1], load.buff[2], load.buff[3],
			freq[0], freq[1], freq[2], freq[3]);
	}
	if ((load_st == -1) && (freq_st == -1)) {
		CPUSTATDBG("%s: end\n", __func__);
		*peof = 1;
		cpu_past_stat_is_read = false;
	} else {
		CPUSTATDBG("%s: continue\n", __func__);
		*start = (char *)len;
	}
	return len;
}

static int cpu_past_stat_buff_init(void)
{
	if (cpu_past_stat_is_init)
		return 0;
	spin_lock_init(&cpu_past_stat_freq_lock);
	cpu_past_stat_load.ts = kmalloc(sizeof(struct timespec)
				* LOADBUFFNUM, GFP_KERNEL);
	if (cpu_past_stat_load.ts == NULL)
		return -1;
	cpu_past_stat_load.load = kmalloc(sizeof(struct cpu_past_stat_load_buff_t)
				* LOADBUFFNUM, GFP_KERNEL);
	if (cpu_past_stat_load.load == NULL)
		goto err1;
	cpu_past_stat_freq.ts = kmalloc(sizeof(struct timespec)
				* FREQBUFFNUM, GFP_KERNEL);
	if (cpu_past_stat_freq.ts == NULL)
		goto err2;
	cpu_past_stat_freq.cpu = kmalloc(sizeof(char)
				* FREQBUFFNUM, GFP_KERNEL);
	if (cpu_past_stat_freq.cpu == NULL)
		goto err3;
	cpu_past_stat_freq.freq = kmalloc(sizeof(signed char)
				* FREQBUFFNUM, GFP_KERNEL);
	if (cpu_past_stat_freq.freq == NULL)
		goto err4;
	cpu_past_stat_is_init = true;
	return 0;
err4:
	kfree(cpu_past_stat_freq.cpu);
err3:
	kfree(cpu_past_stat_freq.ts);
err2:
	kfree(cpu_past_stat_load.load);
err1:
	kfree(cpu_past_stat_load.ts);
	return -1;
}
static int cpu_past_stat_write_proc(struct file *filep, const char *buff,
				unsigned long count, void *dat)
{
	char command[64];
	static const char init_str[] = "init";
	static const char start_str[] = "start";
	static const char stop_str[] = "stop";

	if (sizeof(command) < count)
		count = sizeof(command);
	if(copy_from_user(command, buff, count)) {
		printk("cpu_past_stat: copy_from_user NG");
		return -EFAULT;
	}
	if (strncmp(command, init_str, sizeof(init_str) - 1) == 0) {
		if (cpu_past_stat_buff_init()) {
			printk("cpu_past_stat: init NG\n");
		} else {
			printk("cpu_past_stat: init OK\n");
		}
	} else if (strncmp(command, start_str, sizeof(start_str) - 1) == 0) {
		if(cpu_past_stat_buff_init()) {
			printk("cpu_past_stat: start NG\n");
		} else {
			cpu_past_stat_is_enable = true;
			printk("cpu_past_stat: start OK\n");
		}
	} else if (strncmp(command, stop_str, sizeof(stop_str) - 1) == 0) {
		while (cpu_past_stat_load.is_update
			|| cpu_past_stat_freq.is_update)	//TODO use wait or lock
			msleep(1);
		cpu_past_stat_is_enable = false;
		printk("cpu_past_stat: stop OK\n");
	}
	return count;
}
#endif /* CONFIG_CPU_PAST_STAT */

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	unsigned int i;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		dbs_tuners_ins.down_differential =
					MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate =
			MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

	input_wq = create_workqueue("iewq");
	if (!input_wq) {
		printk(KERN_ERR "Failed to create iewq workqueue\n");
		return -EFAULT;
	}
	boost_wq = create_workqueue("bowq");
	if (!boost_wq) {
		printk(KERN_ERR "Failed to create bowq workqueue\n");
		return -EFAULT;
	}
	for_each_possible_cpu(i) {
		struct cpu_dbs_info_s *this_dbs_info =
			&per_cpu(od_cpu_dbs_info, i);
		mutex_init(&this_dbs_info->timer_mutex);
		INIT_WORK(&per_cpu(dbs_refresh_work, i), dbs_refresh_callback);
		INIT_WORK(&per_cpu(dbs_boost_work, i), dbs_boost_callback);
	}
#ifdef  CONFIG_CPU_PAST_STAT
	cpu_past_stat_proc_ent =
		create_proc_entry("cpu_past_stat", S_IRUSR|S_IWUSR, NULL);
	if (cpu_past_stat_proc_ent == NULL) {
		printk("cpu_past_stat: create_proc_entry NG\n");
	} else {
		cpu_past_stat_proc_ent->read_proc = cpu_past_stat_read_proc;
		cpu_past_stat_proc_ent->write_proc = cpu_past_stat_write_proc;
	}
#endif /* CONFIG_CPU_PAST_STAT */

	return cpufreq_register_governor(&cpufreq_gov_ondemand);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_ondemand);
	destroy_workqueue(input_wq);
}


MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_DESCRIPTION("'cpufreq_ondemand' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
