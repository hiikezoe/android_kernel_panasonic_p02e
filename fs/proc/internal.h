/* internal.h: internal procfs definitions
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/proc_fs.h>
struct  ctl_table_header;

#if defined(CONFIG_CCSECURITY) && defined(CONFIG_CCSECURITY_PROCMEM_PROCMAPS)
#include <linux/sched.h>
#include <linux/ccsecurity.h>
#endif/* CONFIG_CCSECURITY && CONFIG_CCSECURITY_PROCMEM_PROCMAPS */

extern struct proc_dir_entry proc_root;
#ifdef CONFIG_PROC_SYSCTL
extern int proc_sys_init(void);
extern void sysctl_head_put(struct ctl_table_header *head);
#else
static inline void proc_sys_init(void) { }
static inline void sysctl_head_put(struct ctl_table_header *head) { }
#endif
#ifdef CONFIG_NET
extern int proc_net_init(void);
#else
static inline int proc_net_init(void) { return 0; }
#endif

struct vmalloc_info {
	unsigned long	used;
	unsigned long	largest_chunk;
};

extern struct mm_struct *mm_for_maps(struct task_struct *);

#ifdef CONFIG_MMU
#define VMALLOC_TOTAL (VMALLOC_END - VMALLOC_START)
extern void get_vmalloc_info(struct vmalloc_info *vmi);
#else

#define VMALLOC_TOTAL 0UL
#define get_vmalloc_info(vmi)			\
do {						\
	(vmi)->used = 0;			\
	(vmi)->largest_chunk = 0;		\
} while(0)
#endif

extern int proc_tid_stat(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_tgid_stat(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_pid_status(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_pid_statm(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern loff_t mem_lseek(struct file *file, loff_t offset, int orig);

extern const struct file_operations proc_pid_maps_operations;
extern const struct file_operations proc_tid_maps_operations;
extern const struct file_operations proc_pid_numa_maps_operations;
extern const struct file_operations proc_tid_numa_maps_operations;
extern const struct file_operations proc_pid_smaps_operations;
extern const struct file_operations proc_tid_smaps_operations;
extern const struct file_operations proc_clear_refs_operations;
extern const struct file_operations proc_pagemap_operations;
extern const struct file_operations proc_net_operations;
extern const struct inode_operations proc_net_inode_operations;

struct proc_maps_private {
	struct pid *pid;
	struct task_struct *task;
#ifdef CONFIG_MMU
	struct vm_area_struct *tail_vma;
#endif
};

void proc_init_inodecache(void);

static inline struct pid *proc_pid(struct inode *inode)
{
	return PROC_I(inode)->pid;
}

static inline struct task_struct *get_proc_task(struct inode *inode)
{
	return get_pid_task(proc_pid(inode), PIDTYPE_PID);
}

static inline int proc_fd(struct inode *inode)
{
	return PROC_I(inode)->fd;
}

struct dentry *proc_lookup_de(struct proc_dir_entry *de, struct inode *ino,
		struct dentry *dentry);
int proc_readdir_de(struct proc_dir_entry *de, struct file *filp, void *dirent,
		filldir_t filldir);

struct pde_opener {
	struct inode *inode;
	struct file *file;
	int (*release)(struct inode *, struct file *);
	struct list_head lh;
};
void pde_users_dec(struct proc_dir_entry *pde);

extern spinlock_t proc_subdir_lock;

struct dentry *proc_pid_lookup(struct inode *dir, struct dentry * dentry, struct nameidata *);
int proc_pid_readdir(struct file * filp, void * dirent, filldir_t filldir);
unsigned long task_vsize(struct mm_struct *);
unsigned long task_statm(struct mm_struct *,
	unsigned long *, unsigned long *, unsigned long *, unsigned long *);
void task_mem(struct seq_file *, struct mm_struct *);

static inline struct proc_dir_entry *pde_get(struct proc_dir_entry *pde)
{
	atomic_inc(&pde->count);
	return pde;
}
void pde_put(struct proc_dir_entry *pde);

int proc_fill_super(struct super_block *);
struct inode *proc_get_inode(struct super_block *, struct proc_dir_entry *);
int proc_remount(struct super_block *sb, int *flags, char *data);

/*
 * These are generic /proc routines that use the internal
 * "struct proc_dir_entry" tree to traverse the filesystem.
 *
 * The /proc root directory has extended versions to take care
 * of the /proc/<pid> subdirectories.
 */
int proc_readdir(struct file *, void *, filldir_t);
struct dentry *proc_lookup(struct inode *, struct dentry *, struct nameidata *);



/* Lookups */
typedef struct dentry *instantiate_t(struct inode *, struct dentry *,
				struct task_struct *, const void *);
int proc_fill_cache(struct file *filp, void *dirent, filldir_t filldir,
	const char *name, int len,
	instantiate_t instantiate, struct task_struct *task, const void *ptr);
int pid_revalidate(struct dentry *dentry, struct nameidata *nd);
struct inode *proc_pid_make_inode(struct super_block * sb, struct task_struct *task);
extern const struct dentry_operations pid_dentry_operations;
int pid_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);
int proc_setattr(struct dentry *dentry, struct iattr *attr);

extern const struct inode_operations proc_ns_dir_inode_operations;
extern const struct file_operations proc_ns_dir_operations;

#if defined(CONFIG_CCSECURITY) && defined(CONFIG_CCSECURITY_PROCMEM_PROCMAPS)
static inline bool anti_proc_comm(const struct task_struct *task)
{
	unsigned char i = 0;

	if(anti_comm_nums == CCS_FILE_ERROR)
	{
		return false;
	}
	else
	{
		for (i = 0; i < anti_comm_nums; i++) {
			if(!strcmp(task->comm, anti_comm_name[i])) {
				return true;
			}
		}
	}
	return false;
}

static inline bool permitted_comm_check(void)
{
       unsigned char *permitted_comm_name[] = {
                                                       "system_server",
                                                       "vold",
                                                       "Signal Catcher",
                                                       "Binder_",
                                               };

       unsigned char permitted_comm_nums = 0;
       unsigned char i = 0;

       permitted_comm_nums = sizeof(permitted_comm_name) / sizeof(permitted_comm_name[0]);

       for (i = 0; i < permitted_comm_nums; i++) {
               if(!strcmp(current->comm, permitted_comm_name[i])
                       && (current_uid() < 10000) && (current_uid() != 2000)) {
                       return true;
               }else if(!strcmp(permitted_comm_name[i], "Binder_")){
                         if(!strncmp(current->comm, permitted_comm_name[i], 7)
                         && (current_uid() < 10000) && (current_uid() != 2000)) {
                         return true;
                         }
               }
       }
       return false;
}

static inline bool anti_procmem_uid(const struct task_struct *task,
                                   const char *funcname)
{
       if ((!ccsecurity_ops.disabled) && (task != current)) {
               const uid_t uid = task_uid(task);
               switch (uid) {
               case 1001:
               case 1013:
               case 1027:
               case 1200:
               case 4000:
               case 4001:
               case 4002:
               case 4200:
               case 4202:
               case 4212:
               case 4300:
               case 4500:
			return true;
		}
                if( anti_proc_comm(task) ) {
			return true;
                }
	}
	return false;
}
static inline bool anti_procmaps_uid(const struct task_struct *task,
                                    const char *funcname)
{
       if ((!ccsecurity_ops.disabled) && (task != current)) {
               const uid_t uid = task_uid(task);
               switch (uid) {
               case 1001:
               case 1013:
               case 1027:
               case 1200:
               case 4000:
               case 4001:
               case 4002:
               case 4200:
               case 4202:
               case 4212:
               case 4300:
               case 4500:
               if(permitted_comm_check()) {
                       return false;
               }
               else {
                       return true;
               }
               }
                if( anti_proc_comm(task) ) {
			if(permitted_comm_check()) {
				return false;
			}
			else {
				return true;
			}
                }
	}
	return false;
}
static inline bool anti_check_gid(const struct task_struct *task,
                                    const char *funcname)
{
	int i;
	gid_t gid;
	int groups;
	struct group_info *ginfo = task->cred->group_info;

	if (ccsecurity_ops.disabled || task == current || ginfo == NULL) {
		return false;
	}

	groups = ginfo->ngroups;
	for (i = 0; i < groups; i++) {
		gid = GROUP_AT(ginfo,i);
		switch (gid) {
			case 4250:
				return true;
				break;
			default:
				break;
		}
	}
	return false;
}
static inline bool anti_procmem_gid(const struct task_struct *task,
                                     const char *funcname)
{
	return anti_check_gid(task, funcname);
}
static inline bool anti_procmaps_gid(const struct task_struct *task,
                                     const char *funcname)
{
	return anti_check_gid(task, funcname);
}
#else /* defined(CONFIG_CCSECURITY) && defined(CONFIG_CCSECURITY_PROCMEM_PROCMAPS) */
static inline bool anti_procmem_uid(const struct task_struct *task,
                                   const char *funcname)
{
       return false;
}
static inline bool anti_procmaps_uid(const struct task_struct *task,
                                    const char *funcname)
{
       return false;
}
static inline bool anti_procmem_gid(const struct task_struct *task,
                                   const char *funcname)
{
       return false;
}
static inline bool anti_procmaps_gid(const struct task_struct *task,
                                    const char *funcname)
{
       return false;
}
#endif /* defined(CONFIG_CCSECURITY) && defined(CONFIG_CCSECURITY_PROCMEM_PROCMAPS) */
