#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <libos/mman.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <libos/backtrace.h>
#include <libos/barrier.h>
#include <libos/buf.h>
#include <libos/cpio.h>
#include <libos/cwd.h>
#include <libos/eraise.h>
#include <libos/errno.h>
#include <libos/exec.h>
#include <libos/fdtable.h>
#include <libos/file.h>
#include <libos/fsgs.h>
#include <libos/gcov.h>
#include <libos/id.h>
#include <libos/initfini.h>
#include <libos/kernel.h>
#include <libos/libc.h>
#include <libos/lsr.h>
#include <libos/mmanutils.h>
#include <libos/mount.h>
#include <libos/options.h>
#include <libos/panic.h>
#include <libos/paths.h>
#include <libos/pipedev.h>
#include <libos/printf.h>
#include <libos/process.h>
#include <libos/ramfs.h>
#include <libos/setjmp.h>
#include <libos/spinlock.h>
#include <libos/strings.h>
#include <libos/syscall.h>
#include <libos/tcall.h>
#include <libos/thread.h>
#include <libos/times.h>
#include <libos/trace.h>

#define DEV_URANDOM_FD (FD_OFFSET + FDTABLE_SIZE)

#define COLOR_RED "\e[31m"
#define COLOR_BLUE "\e[34m"
#define COLOR_GREEN "\e[32m"
#define COLOR_RESET "\e[0m"

long libos_syscall_isatty(int fd);

typedef struct _pair
{
    long num;
    const char* str;
} pair_t;

static pair_t _pairs[] = {
    {SYS_read, "SYS_read"},
    {SYS_write, "SYS_write"},
    {SYS_open, "SYS_open"},
    {SYS_close, "SYS_close"},
    {SYS_stat, "SYS_stat"},
    {SYS_fstat, "SYS_fstat"},
    {SYS_lstat, "SYS_lstat"},
    {SYS_poll, "SYS_poll"},
    {SYS_lseek, "SYS_lseek"},
    {SYS_mmap, "SYS_mmap"},
    {SYS_mprotect, "SYS_mprotect"},
    {SYS_munmap, "SYS_munmap"},
    {SYS_brk, "SYS_brk"},
    {SYS_rt_sigaction, "SYS_rt_sigaction"},
    {SYS_rt_sigprocmask, "SYS_rt_sigprocmask"},
    {SYS_rt_sigreturn, "SYS_rt_sigreturn"},
    {SYS_ioctl, "SYS_ioctl"},
    {SYS_pread64, "SYS_pread64"},
    {SYS_pwrite64, "SYS_pwrite64"},
    {SYS_readv, "SYS_readv"},
    {SYS_writev, "SYS_writev"},
    {SYS_access, "SYS_access"},
    {SYS_pipe, "SYS_pipe"},
    {SYS_select, "SYS_select"},
    {SYS_sched_yield, "SYS_sched_yield"},
    {SYS_mremap, "SYS_mremap"},
    {SYS_msync, "SYS_msync"},
    {SYS_mincore, "SYS_mincore"},
    {SYS_madvise, "SYS_madvise"},
    {SYS_shmget, "SYS_shmget"},
    {SYS_shmat, "SYS_shmat"},
    {SYS_shmctl, "SYS_shmctl"},
    {SYS_dup, "SYS_dup"},
    {SYS_dup2, "SYS_dup2"},
    {SYS_pause, "SYS_pause"},
    {SYS_nanosleep, "SYS_nanosleep"},
    {SYS_getitimer, "SYS_getitimer"},
    {SYS_alarm, "SYS_alarm"},
    {SYS_setitimer, "SYS_setitimer"},
    {SYS_getpid, "SYS_getpid"},
    {SYS_sendfile, "SYS_sendfile"},
    {SYS_socket, "SYS_socket"},
    {SYS_connect, "SYS_connect"},
    {SYS_accept, "SYS_accept"},
    {SYS_sendto, "SYS_sendto"},
    {SYS_recvfrom, "SYS_recvfrom"},
    {SYS_sendmsg, "SYS_sendmsg"},
    {SYS_recvmsg, "SYS_recvmsg"},
    {SYS_shutdown, "SYS_shutdown"},
    {SYS_bind, "SYS_bind"},
    {SYS_listen, "SYS_listen"},
    {SYS_getsockname, "SYS_getsockname"},
    {SYS_getpeername, "SYS_getpeername"},
    {SYS_socketpair, "SYS_socketpair"},
    {SYS_setsockopt, "SYS_setsockopt"},
    {SYS_getsockopt, "SYS_getsockopt"},
    {SYS_clone, "SYS_clone"},
    {SYS_fork, "SYS_fork"},
    {SYS_vfork, "SYS_vfork"},
    {SYS_execve, "SYS_execve"},
    {SYS_exit, "SYS_exit"},
    {SYS_wait4, "SYS_wait4"},
    {SYS_kill, "SYS_kill"},
    {SYS_uname, "SYS_uname"},
    {SYS_semget, "SYS_semget"},
    {SYS_semop, "SYS_semop"},
    {SYS_semctl, "SYS_semctl"},
    {SYS_shmdt, "SYS_shmdt"},
    {SYS_msgget, "SYS_msgget"},
    {SYS_msgsnd, "SYS_msgsnd"},
    {SYS_msgrcv, "SYS_msgrcv"},
    {SYS_msgctl, "SYS_msgctl"},
    {SYS_fcntl, "SYS_fcntl"},
    {SYS_flock, "SYS_flock"},
    {SYS_fsync, "SYS_fsync"},
    {SYS_fdatasync, "SYS_fdatasync"},
    {SYS_truncate, "SYS_truncate"},
    {SYS_ftruncate, "SYS_ftruncate"},
    {SYS_getdents, "SYS_getdents"},
    {SYS_getcwd, "SYS_getcwd"},
    {SYS_chdir, "SYS_chdir"},
    {SYS_fchdir, "SYS_fchdir"},
    {SYS_rename, "SYS_rename"},
    {SYS_mkdir, "SYS_mkdir"},
    {SYS_rmdir, "SYS_rmdir"},
    {SYS_creat, "SYS_creat"},
    {SYS_link, "SYS_link"},
    {SYS_unlink, "SYS_unlink"},
    {SYS_symlink, "SYS_symlink"},
    {SYS_readlink, "SYS_readlink"},
    {SYS_chmod, "SYS_chmod"},
    {SYS_fchmod, "SYS_fchmod"},
    {SYS_chown, "SYS_chown"},
    {SYS_fchown, "SYS_fchown"},
    {SYS_lchown, "SYS_lchown"},
    {SYS_umask, "SYS_umask"},
    {SYS_gettimeofday, "SYS_gettimeofday"},
    {SYS_getrlimit, "SYS_getrlimit"},
    {SYS_getrusage, "SYS_getrusage"},
    {SYS_sysinfo, "SYS_sysinfo"},
    {SYS_times, "SYS_times"},
    {SYS_ptrace, "SYS_ptrace"},
    {SYS_getuid, "SYS_getuid"},
    {SYS_syslog, "SYS_syslog"},
    {SYS_getgid, "SYS_getgid"},
    {SYS_setuid, "SYS_setuid"},
    {SYS_setgid, "SYS_setgid"},
    {SYS_geteuid, "SYS_geteuid"},
    {SYS_getegid, "SYS_getegid"},
    {SYS_setpgid, "SYS_setpgid"},
    {SYS_getppid, "SYS_getppid"},
    {SYS_getpgrp, "SYS_getpgrp"},
    {SYS_setsid, "SYS_setsid"},
    {SYS_setreuid, "SYS_setreuid"},
    {SYS_setregid, "SYS_setregid"},
    {SYS_getgroups, "SYS_getgroups"},
    {SYS_setgroups, "SYS_setgroups"},
    {SYS_setresuid, "SYS_setresuid"},
    {SYS_getresuid, "SYS_getresuid"},
    {SYS_setresgid, "SYS_setresgid"},
    {SYS_getresgid, "SYS_getresgid"},
    {SYS_getpgid, "SYS_getpgid"},
    {SYS_setfsuid, "SYS_setfsuid"},
    {SYS_setfsgid, "SYS_setfsgid"},
    {SYS_getsid, "SYS_getsid"},
    {SYS_capget, "SYS_capget"},
    {SYS_capset, "SYS_capset"},
    {SYS_rt_sigpending, "SYS_rt_sigpending"},
    {SYS_rt_sigtimedwait, "SYS_rt_sigtimedwait"},
    {SYS_rt_sigqueueinfo, "SYS_rt_sigqueueinfo"},
    {SYS_rt_sigsuspend, "SYS_rt_sigsuspend"},
    {SYS_sigaltstack, "SYS_sigaltstack"},
    {SYS_utime, "SYS_utime"},
    {SYS_mknod, "SYS_mknod"},
    {SYS_uselib, "SYS_uselib"},
    {SYS_personality, "SYS_personality"},
    {SYS_ustat, "SYS_ustat"},
    {SYS_statfs, "SYS_statfs"},
    {SYS_fstatfs, "SYS_fstatfs"},
    {SYS_sysfs, "SYS_sysfs"},
    {SYS_getpriority, "SYS_getpriority"},
    {SYS_setpriority, "SYS_setpriority"},
    {SYS_sched_setparam, "SYS_sched_setparam"},
    {SYS_sched_getparam, "SYS_sched_getparam"},
    {SYS_sched_setscheduler, "SYS_sched_setscheduler"},
    {SYS_sched_getscheduler, "SYS_sched_getscheduler"},
    {SYS_sched_get_priority_max, "SYS_sched_get_priority_max"},
    {SYS_sched_get_priority_min, "SYS_sched_get_priority_min"},
    {SYS_sched_rr_get_interval, "SYS_sched_rr_get_interval"},
    {SYS_mlock, "SYS_mlock"},
    {SYS_munlock, "SYS_munlock"},
    {SYS_mlockall, "SYS_mlockall"},
    {SYS_munlockall, "SYS_munlockall"},
    {SYS_vhangup, "SYS_vhangup"},
    {SYS_modify_ldt, "SYS_modify_ldt"},
    {SYS_pivot_root, "SYS_pivot_root"},
    {SYS__sysctl, "SYS__sysctl"},
    {SYS_prctl, "SYS_prctl"},
    {SYS_arch_prctl, "SYS_arch_prctl"},
    {SYS_adjtimex, "SYS_adjtimex"},
    {SYS_setrlimit, "SYS_setrlimit"},
    {SYS_chroot, "SYS_chroot"},
    {SYS_sync, "SYS_sync"},
    {SYS_acct, "SYS_acct"},
    {SYS_settimeofday, "SYS_settimeofday"},
    {SYS_mount, "SYS_mount"},
    {SYS_umount2, "SYS_umount2"},
    {SYS_swapon, "SYS_swapon"},
    {SYS_swapoff, "SYS_swapoff"},
    {SYS_reboot, "SYS_reboot"},
    {SYS_sethostname, "SYS_sethostname"},
    {SYS_setdomainname, "SYS_setdomainname"},
    {SYS_iopl, "SYS_iopl"},
    {SYS_ioperm, "SYS_ioperm"},
    {SYS_create_module, "SYS_create_module"},
    {SYS_init_module, "SYS_init_module"},
    {SYS_delete_module, "SYS_delete_module"},
    {SYS_get_kernel_syms, "SYS_get_kernel_syms"},
    {SYS_query_module, "SYS_query_module"},
    {SYS_quotactl, "SYS_quotactl"},
    {SYS_nfsservctl, "SYS_nfsservctl"},
    {SYS_getpmsg, "SYS_getpmsg"},
    {SYS_putpmsg, "SYS_putpmsg"},
    {SYS_afs_syscall, "SYS_afs_syscall"},
    {SYS_tuxcall, "SYS_tuxcall"},
    {SYS_security, "SYS_security"},
    {SYS_gettid, "SYS_gettid"},
    {SYS_readahead, "SYS_readahead"},
    {SYS_setxattr, "SYS_setxattr"},
    {SYS_lsetxattr, "SYS_lsetxattr"},
    {SYS_fsetxattr, "SYS_fsetxattr"},
    {SYS_getxattr, "SYS_getxattr"},
    {SYS_lgetxattr, "SYS_lgetxattr"},
    {SYS_fgetxattr, "SYS_fgetxattr"},
    {SYS_listxattr, "SYS_listxattr"},
    {SYS_llistxattr, "SYS_llistxattr"},
    {SYS_flistxattr, "SYS_flistxattr"},
    {SYS_removexattr, "SYS_removexattr"},
    {SYS_lremovexattr, "SYS_lremovexattr"},
    {SYS_fremovexattr, "SYS_fremovexattr"},
    {SYS_tkill, "SYS_tkill"},
    {SYS_time, "SYS_time"},
    {SYS_futex, "SYS_futex"},
    {SYS_sched_setaffinity, "SYS_sched_setaffinity"},
    {SYS_sched_getaffinity, "SYS_sched_getaffinity"},
    {SYS_set_thread_area, "SYS_set_thread_area"},
    {SYS_io_setup, "SYS_io_setup"},
    {SYS_io_destroy, "SYS_io_destroy"},
    {SYS_io_getevents, "SYS_io_getevents"},
    {SYS_io_submit, "SYS_io_submit"},
    {SYS_io_cancel, "SYS_io_cancel"},
    {SYS_get_thread_area, "SYS_get_thread_area"},
    {SYS_lookup_dcookie, "SYS_lookup_dcookie"},
    {SYS_epoll_create, "SYS_epoll_create"},
    {SYS_epoll_ctl_old, "SYS_epoll_ctl_old"},
    {SYS_epoll_wait_old, "SYS_epoll_wait_old"},
    {SYS_remap_file_pages, "SYS_remap_file_pages"},
    {SYS_getdents64, "SYS_getdents64"},
    {SYS_set_tid_address, "SYS_set_tid_address"},
    {SYS_restart_syscall, "SYS_restart_syscall"},
    {SYS_semtimedop, "SYS_semtimedop"},
    {SYS_fadvise64, "SYS_fadvise64"},
    {SYS_timer_create, "SYS_timer_create"},
    {SYS_timer_settime, "SYS_timer_settime"},
    {SYS_timer_gettime, "SYS_timer_gettime"},
    {SYS_timer_getoverrun, "SYS_timer_getoverrun"},
    {SYS_timer_delete, "SYS_timer_delete"},
    {SYS_clock_settime, "SYS_clock_settime"},
    {SYS_clock_gettime, "SYS_clock_gettime"},
    {SYS_clock_getres, "SYS_clock_getres"},
    {SYS_clock_nanosleep, "SYS_clock_nanosleep"},
    {SYS_exit_group, "SYS_exit_group"},
    {SYS_epoll_wait, "SYS_epoll_wait"},
    {SYS_epoll_ctl, "SYS_epoll_ctl"},
    {SYS_tgkill, "SYS_tgkill"},
    {SYS_utimes, "SYS_utimes"},
    {SYS_vserver, "SYS_vserver"},
    {SYS_mbind, "SYS_mbind"},
    {SYS_set_mempolicy, "SYS_set_mempolicy"},
    {SYS_get_mempolicy, "SYS_get_mempolicy"},
    {SYS_mq_open, "SYS_mq_open"},
    {SYS_mq_unlink, "SYS_mq_unlink"},
    {SYS_mq_timedsend, "SYS_mq_timedsend"},
    {SYS_mq_timedreceive, "SYS_mq_timedreceive"},
    {SYS_mq_notify, "SYS_mq_notify"},
    {SYS_mq_getsetattr, "SYS_mq_getsetattr"},
    {SYS_kexec_load, "SYS_kexec_load"},
    {SYS_waitid, "SYS_waitid"},
    {SYS_add_key, "SYS_add_key"},
    {SYS_request_key, "SYS_request_key"},
    {SYS_keyctl, "SYS_keyctl"},
    {SYS_ioprio_set, "SYS_ioprio_set"},
    {SYS_ioprio_get, "SYS_ioprio_get"},
    {SYS_inotify_init, "SYS_inotify_init"},
    {SYS_inotify_add_watch, "SYS_inotify_add_watch"},
    {SYS_inotify_rm_watch, "SYS_inotify_rm_watch"},
    {SYS_migrate_pages, "SYS_migrate_pages"},
    {SYS_openat, "SYS_openat"},
    {SYS_mkdirat, "SYS_mkdirat"},
    {SYS_mknodat, "SYS_mknodat"},
    {SYS_fchownat, "SYS_fchownat"},
    {SYS_futimesat, "SYS_futimesat"},
    {SYS_newfstatat, "SYS_newfstatat"},
    {SYS_unlinkat, "SYS_unlinkat"},
    {SYS_renameat, "SYS_renameat"},
    {SYS_linkat, "SYS_linkat"},
    {SYS_symlinkat, "SYS_symlinkat"},
    {SYS_readlinkat, "SYS_readlinkat"},
    {SYS_fchmodat, "SYS_fchmodat"},
    {SYS_faccessat, "SYS_faccessat"},
    {SYS_pselect6, "SYS_pselect6"},
    {SYS_ppoll, "SYS_ppoll"},
    {SYS_unshare, "SYS_unshare"},
    {SYS_set_robust_list, "SYS_set_robust_list"},
    {SYS_get_robust_list, "SYS_get_robust_list"},
    {SYS_splice, "SYS_splice"},
    {SYS_tee, "SYS_tee"},
    {SYS_sync_file_range, "SYS_sync_file_range"},
    {SYS_vmsplice, "SYS_vmsplice"},
    {SYS_move_pages, "SYS_move_pages"},
    {SYS_utimensat, "SYS_utimensat"},
    {SYS_epoll_pwait, "SYS_epoll_pwait"},
    {SYS_signalfd, "SYS_signalfd"},
    {SYS_timerfd_create, "SYS_timerfd_create"},
    {SYS_eventfd, "SYS_eventfd"},
    {SYS_fallocate, "SYS_fallocate"},
    {SYS_timerfd_settime, "SYS_timerfd_settime"},
    {SYS_timerfd_gettime, "SYS_timerfd_gettime"},
    {SYS_accept4, "SYS_accept4"},
    {SYS_signalfd4, "SYS_signalfd4"},
    {SYS_eventfd2, "SYS_eventfd2"},
    {SYS_epoll_create1, "SYS_epoll_create1"},
    {SYS_dup3, "SYS_dup3"},
    {SYS_pipe2, "SYS_pipe2"},
    {SYS_inotify_init1, "SYS_inotify_init1"},
    {SYS_preadv, "SYS_preadv"},
    {SYS_pwritev, "SYS_pwritev"},
    {SYS_rt_tgsigqueueinfo, "SYS_rt_tgsigqueueinfo"},
    {SYS_perf_event_open, "SYS_perf_event_open"},
    {SYS_recvmmsg, "SYS_recvmmsg"},
    {SYS_fanotify_init, "SYS_fanotify_init"},
    {SYS_fanotify_mark, "SYS_fanotify_mark"},
    {SYS_prlimit64, "SYS_prlimit64"},
    {SYS_name_to_handle_at, "SYS_name_to_handle_at"},
    {SYS_open_by_handle_at, "SYS_open_by_handle_at"},
    {SYS_clock_adjtime, "SYS_clock_adjtime"},
    {SYS_syncfs, "SYS_syncfs"},
    {SYS_sendmmsg, "SYS_sendmmsg"},
    {SYS_setns, "SYS_setns"},
    {SYS_getcpu, "SYS_getcpu"},
    {SYS_process_vm_readv, "SYS_process_vm_readv"},
    {SYS_process_vm_writev, "SYS_process_vm_writev"},
    {SYS_kcmp, "SYS_kcmp"},
    {SYS_finit_module, "SYS_finit_module"},
    {SYS_sched_setattr, "SYS_sched_setattr"},
    {SYS_sched_getattr, "SYS_sched_getattr"},
    {SYS_renameat2, "SYS_renameat2"},
    {SYS_seccomp, "SYS_seccomp"},
    {SYS_getrandom, "SYS_getrandom"},
    {SYS_memfd_create, "SYS_memfd_create"},
    {SYS_kexec_file_load, "SYS_kexec_file_load"},
    {SYS_bpf, "SYS_bpf"},
    {SYS_execveat, "SYS_execveat"},
    {SYS_userfaultfd, "SYS_userfaultfd"},
    {SYS_membarrier, "SYS_membarrier"},
    {SYS_mlock2, "SYS_mlock2"},
    {SYS_copy_file_range, "SYS_copy_file_range"},
    {SYS_preadv2, "SYS_preadv2"},
    {SYS_pwritev2, "SYS_pwritev2"},
    {SYS_pkey_mprotect, "SYS_pkey_mprotect"},
    {SYS_pkey_alloc, "SYS_pkey_alloc"},
    {SYS_pkey_free, "SYS_pkey_free"},
    {SYS_statx, "SYS_statx"},
    {SYS_io_pgetevents, "SYS_io_pgetevents"},
    {SYS_rseq, "SYS_rseq"},
    {SYS_libos_trace, "SYS_libos_trace"},
    {SYS_libos_trace_ptr, "SYS_libos_trace_ptr"},
    {SYS_libos_dump_ehdr, "SYS_libos_dump_ehdr"},
    {SYS_libos_dump_argv, "SYS_libos_dump_argv"},
    {SYS_libos_dump_stack, "SYS_libos_dump_stack"},
    {SYS_libos_add_symbol_file, "SYS_libos_add_symbol_file"},
    {SYS_libos_load_symbols, "SYS_libos_load_symbols"},
    {SYS_libos_unload_symbols, "SYS_libos_unload_symbols"},
    {SYS_libos_gen_creds, "SYS_libos_gen_creds"},
    {SYS_libos_free_creds, "SYS_libos_free_creds"},
    {SYS_libos_clone, "SYS_libos_clone"},
    {SYS_libos_gcov_init, "SYS_libos_gcov_init"},
    {SYS_libos_max_threads, "SYS_libos_max_threads"},
    /* OE */
    {SYS_libos_oe_add_vectored_exception_handler,
     "SYS_libos_oe_add_vectored_exception_handler"},
    {SYS_libos_oe_remove_vectored_exception_handler,
     "SYS_libos_oe_remove_vectored_exception_handler"},
    {SYS_libos_oe_is_within_enclave, "SYS_libos_oe_is_within_enclave"},
    {SYS_libos_oe_is_outside_enclave, "SYS_libos_oe_is_outside_enclave"},
    {SYS_libos_oe_host_malloc, "SYS_libos_oe_host_malloc"},
    {SYS_libos_oe_host_realloc, "SYS_libos_oe_host_realloc"},
    {SYS_libos_oe_host_calloc, "SYS_libos_oe_host_calloc"},
    {SYS_libos_oe_host_free, "SYS_libos_oe_host_free"},
    {SYS_libos_oe_strndup, "SYS_libos_oe_strndup"},
    {SYS_libos_oe_abort, "SYS_libos_oe_abort"},
    {SYS_libos_oe_assert_fail, "SYS_libos_oe_assert_fail"},
    {SYS_libos_oe_get_report_v2, "SYS_libos_oe_get_report_v2"},
    {SYS_libos_oe_free_report, "SYS_libos_oe_free_report"},
    {SYS_libos_oe_get_target_info_v2, "SYS_libos_oe_get_target_info_v2"},
    {SYS_libos_oe_free_target_info, "SYS_libos_oe_free_target_info"},
    {SYS_libos_oe_parse_report, "SYS_libos_oe_parse_report"},
    {SYS_libos_oe_verify_report, "SYS_libos_oe_verify_report"},
    {SYS_libos_oe_get_seal_key_by_policy_v2,
     "SYS_libos_oe_get_seal_key_by_policy_v2"},
    {SYS_libos_oe_get_public_key_by_policy,
     "SYS_libos_oe_get_public_key_by_policy"},
    {SYS_libos_oe_get_public_key, "SYS_libos_oe_get_public_key"},
    {SYS_libos_oe_get_private_key_by_policy,
     "SYS_libos_oe_get_private_key_by_policy"},
    {SYS_libos_oe_get_private_key, "SYS_libos_oe_get_private_key"},
    {SYS_libos_oe_free_key, "SYS_libos_oe_free_key"},
    {SYS_libos_oe_get_seal_key_v2, "SYS_libos_oe_get_seal_key_v2"},
    {SYS_libos_oe_free_seal_key, "SYS_libos_oe_free_seal_key"},
    {SYS_libos_oe_get_enclave, "SYS_libos_oe_get_enclave"},
    {SYS_libos_oe_random, "SYS_libos_oe_random"},
    {SYS_libos_oe_generate_attestation_certificate,
     "SYS_libos_oe_generate_attestation_certificate"},
    {SYS_libos_oe_free_attestation_certificate,
     "SYS_libos_oe_free_attestation_certificate"},
    {SYS_libos_oe_verify_attestation_certificate,
     "SYS_libos_oe_verify_attestation_certificate"},
    {SYS_libos_oe_load_module_host_file_system,
     "SYS_libos_oe_load_module_host_file_system"},
    {SYS_libos_oe_load_module_host_socket_interface,
     "SYS_libos_oe_load_module_host_socket_interface"},
    {SYS_libos_oe_load_module_host_resolver,
     "SYS_libos_oe_load_module_host_resolver"},
    {SYS_libos_oe_load_module_host_epoll,
     "SYS_libos_oe_load_module_host_epoll"},
    {SYS_libos_oe_sgx_set_minimum_crl_tcb_issue_date,
     "SYS_libos_oe_sgx_set_minimum_crl_tcb_issue_date"},
    {SYS_libos_oe_result_str, "SYS_libos_oe_result_str"},
    {SYS_libos_oe_get_enclave_status, "SYS_libos_oe_get_enclave_status"},
    {SYS_libos_oe_allocate_ocall_buffer, "SYS_libos_oe_allocate_ocall_buffer"},
    {SYS_libos_oe_free_ocall_buffer, "SYS_libos_oe_free_ocall_buffer"},
    {
        SYS_libos_oe_call_host_function,
        "SYS_libos_oe_call_host_function",
    },
};

static size_t _n_pairs = sizeof(_pairs) / sizeof(_pairs[0]);

const char* syscall_str(long n)
{
    for (size_t i = 0; i < _n_pairs; i++)
    {
        if (n == _pairs[i].num)
            return _pairs[i].str;
    }

    return "unknown";
}

__attribute__((format(printf, 2, 3))) static void _strace(
    long n,
    const char* fmt,
    ...)
{
    if (__options.trace_syscalls)
    {
        const bool isatty = libos_syscall_isatty(STDERR_FILENO) == 1;
        const char* blue = isatty ? COLOR_GREEN : "";
        const char* reset = isatty ? COLOR_RESET : "";
        char buf[1024];

        if (fmt)
        {
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(buf, sizeof(buf), fmt, ap);
            va_end(ap);
        }
        else
        {
            *buf = '\0';
        }

        libos_eprintf(
            "=== %s%s%s(%s): tid=%d\n",
            blue,
            syscall_str(n),
            reset,
            buf,
            libos_gettid());
    }
}

static long _forward_syscall(long n, long params[6])
{
    if (__options.trace_syscalls)
        libos_eprintf("    [forward syscall]\n");

    return libos_tcall(n, params);
}

typedef struct fd_entry
{
    int fd;
    char path[PATH_MAX];
} fd_entry_t;

static long _return(long n, long ret)
{
    if (__options.trace_syscalls)
    {
        const char* red = "";
        const char* reset = "";
        const char* error_name = NULL;

        if (ret < 0)
        {
            const bool isatty = libos_syscall_isatty(STDERR_FILENO) == 1;

            if (isatty)
            {
                red = COLOR_RED;
                reset = COLOR_RESET;
            }

            error_name = libos_error_name(-ret);
        }

        if (error_name)
        {
            libos_eprintf(
                "    %s%s(): return=-%s(%ld)%s: tid=%d\n",
                red,
                syscall_str(n),
                error_name,
                ret,
                reset,
                libos_gettid());
        }
        else
        {
            libos_eprintf(
                "    %s%s(): return=%ld(%lx)%s: tid=%d\n",
                red,
                syscall_str(n),
                ret,
                ret,
                reset,
                libos_gettid());
        }
    }

    return ret;
}

static int _add_fd_link(libos_fs_t* fs, libos_file_t* file, int fd)
{
    int ret = 0;
    char realpath[PATH_MAX];
    char linkpath[PATH_MAX];
    const size_t n = sizeof(linkpath);

    if (!fs || !file)
        ERAISE(-EINVAL);

    ECHECK((*fs->fs_realpath)(fs, file, realpath, sizeof(realpath)));

    if (snprintf(linkpath, n, "/proc/self/fd/%d", fd) >= (int)n)
        ERAISE(-ENAMETOOLONG);

    ECHECK(symlink(realpath, linkpath));

done:
    return ret;
}

static int _remove_fd_link(libos_fs_t* fs, libos_file_t* file, int fd)
{
    int ret = 0;
    char linkpath[PATH_MAX];
    const size_t n = sizeof(linkpath);
    char realpath[PATH_MAX];

    if (!fs || fd < 0)
        ERAISE(-EINVAL);

    ECHECK((*fs->fs_realpath)(fs, file, realpath, sizeof(realpath)));

    if (snprintf(linkpath, n, "/proc/self/fd/%d", fd) >= (int)n)
        ERAISE(-ENAMETOOLONG);

#if 0
    printf("REMOVE{%s=>%s}\n", realpath, linkpath);
#endif

    /* only the root file system can remove the link path */
    {
        char suffix[PATH_MAX];
        libos_fs_t* rootfs;

        ECHECK(libos_mount_resolve("/", suffix, &rootfs));

        ECHECK((*rootfs->fs_unlink)(rootfs, linkpath));
    }

done:
    return ret;
}

long libos_syscall_creat(const char* pathname, mode_t mode)
{
    long ret = 0;
    int fd;
    char suffix[PATH_MAX];
    libos_fs_t* fs;
    libos_file_t* file;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));

    ECHECK((*fs->fs_creat)(fs, suffix, mode, &file));

    if ((fd = libos_fdtable_assign(
             fdtable, LIBOS_FDTABLE_TYPE_FILE, fs, file)) < 0)
    {
        libos_eprintf("libos_fdtable_assign() failed: %d\n", fd);
    }

    ECHECK(_add_fd_link(fs, file, fd));

    ret = fd;

done:

    return ret;
}

long libos_syscall_open(const char* pathname, int flags, mode_t mode)
{
    long ret = 0;
    int fd;
    char suffix[PATH_MAX];
    libos_fs_t* fs;
    libos_file_t* file;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    /* Handle /dev/urandom as a special case */
    if (strcmp(pathname, "/dev/urandom") == 0)
    {
        /* ATTN: handle relative paths to /dev/urandom */
        return DEV_URANDOM_FD;
    }

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));

    ECHECK((*fs->fs_open)(fs, suffix, flags, mode, &file));

    if ((fd = libos_fdtable_assign(
             fdtable, LIBOS_FDTABLE_TYPE_FILE, fs, file)) < 0)
        libos_panic("libos_fdtable_assign() failed: %d", fd);

    ECHECK(_add_fd_link(fs, file, fd));

    ret = fd;

done:

    return ret;
}

long libos_syscall_lseek(int fd, off_t offset, int whence)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = ((*fs->fs_lseek)(fs, file, offset, whence));

done:
    return ret;
}

long libos_syscall_close(int fd)
{
    long ret = 0;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    libos_fdtable_type_t type;
    void* device = NULL;
    void* object = NULL;
    libos_fdops_t* fdops;

    ECHECK(libos_fdtable_get_any(fdtable, fd, &type, &device, &object));
    fdops = device;

    if (type == LIBOS_FDTABLE_TYPE_FILE)
        ECHECK(_remove_fd_link(device, object, fd));

    ECHECK((*fdops->fd_close)(device, object));
    ECHECK(libos_fdtable_remove(fdtable, fd));

done:
    return ret;
}

long libos_syscall_read(int fd, void* buf, size_t count)
{
    long ret = 0;
    void* device = NULL;
    void* object = NULL;
    libos_fdtable_type_t type;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    libos_fdops_t* fdops;

    ECHECK(libos_fdtable_get_any(fdtable, fd, &type, &device, &object));
    fdops = device;

    ret = (*fdops->fd_read)(device, object, buf, count);

done:
    return ret;
}

long libos_syscall_write(int fd, const void* buf, size_t count)
{
    long ret = 0;
    void* device = NULL;
    void* object = NULL;
    libos_fdtable_type_t type;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    libos_fdops_t* fdops;

    ECHECK(libos_fdtable_get_any(fdtable, fd, &type, &device, &object));
    fdops = device;

    ret = (*fdops->fd_write)(device, object, buf, count);

done:
    return ret;
}

long libos_syscall_pread(int fd, void* buf, size_t count, off_t offset)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = (*fs->fs_pread)(fs, file, buf, count, offset);

done:
    return ret;
}

long libos_syscall_pwrite(int fd, const void* buf, size_t count, off_t offset)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = (*fs->fs_pwrite)(fs, file, buf, count, offset);

done:
    return ret;
}

long libos_syscall_readv(int fd, const struct iovec* iov, int iovcnt)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = (*fs->fs_readv)(fs, file, iov, iovcnt);

done:
    return ret;
}

long libos_syscall_writev(int fd, const struct iovec* iov, int iovcnt)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = (*fs->fs_writev)(fs, file, iov, iovcnt);

done:
    return ret;
}

long libos_syscall_stat(const char* pathname, struct stat* statbuf)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_stat)(fs, suffix, statbuf));

done:
    return ret;
}

long libos_syscall_lstat(const char* pathname, struct stat* statbuf)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_lstat)(fs, suffix, statbuf));

done:
    return ret;
}

long libos_syscall_fstat(int fd, struct stat* statbuf)
{
    long ret = 0;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    libos_fdtable_type_t type;
    void* device;
    void* object;
    libos_fdops_t* fdops;

    ECHECK(libos_fdtable_get_any(fdtable, fd, &type, &device, &object));
    fdops = device;

    ret = (*fdops->fd_fstat)(device, object, statbuf);

done:
    return ret;
}

long libos_syscall_mkdir(const char* pathname, mode_t mode)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_mkdir)(fs, suffix, mode));

done:
    return ret;
}

long libos_syscall_rmdir(const char* pathname)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_rmdir)(fs, suffix));

done:
    return ret;
}

long libos_syscall_getdents64(int fd, struct dirent* dirp, size_t count)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));

    ret = (*fs->fs_getdents64)(fs, file, dirp, count);

done:
    return ret;
}

long libos_syscall_link(const char* oldpath, const char* newpath)
{
    long ret = 0;
    char old_suffix[PATH_MAX];
    char new_suffix[PATH_MAX];
    libos_fs_t* old_fs;
    libos_fs_t* new_fs;

    ECHECK(libos_mount_resolve(oldpath, old_suffix, &old_fs));
    ECHECK(libos_mount_resolve(newpath, new_suffix, &new_fs));

    if (old_fs != new_fs)
    {
        /* oldpath and newpath are not on the same mounted file system */
        ERAISE(-EXDEV);
    }

    ECHECK((*old_fs->fs_link)(old_fs, old_suffix, new_suffix));

done:
    return ret;
}

long libos_syscall_unlink(const char* pathname)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_unlink)(fs, suffix));

done:
    return ret;
}

long libos_syscall_access(const char* pathname, int mode)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ECHECK((*fs->fs_access)(fs, suffix, mode));

done:
    return ret;
}

long libos_syscall_rename(const char* oldpath, const char* newpath)
{
    long ret = 0;
    char old_suffix[PATH_MAX];
    char new_suffix[PATH_MAX];
    libos_fs_t* old_fs;
    libos_fs_t* new_fs;

    ECHECK(libos_mount_resolve(oldpath, old_suffix, &old_fs));
    ECHECK(libos_mount_resolve(newpath, new_suffix, &new_fs));

    if (old_fs != new_fs)
    {
        /* oldpath and newpath are not on the same mounted file system */
        ERAISE(-EXDEV);
    }

    ECHECK((*old_fs->fs_rename)(old_fs, old_suffix, new_suffix));

done:
    return ret;
}

long libos_syscall_truncate(const char* path, off_t length)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(path, suffix, &fs));
    ERAISE((*fs->fs_truncate)(fs, suffix, length));

done:
    return ret;
}

long libos_syscall_ftruncate(int fd, off_t length)
{
    long ret = 0;
    libos_fs_t* fs;
    libos_file_t* file;
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_FILE;
    libos_fdtable_t* fdtable = libos_fdtable_current();

    ECHECK(libos_fdtable_get(fdtable, fd, type, (void**)&fs, (void**)&file));
    ERAISE((*fs->fs_ftruncate)(fs, file, length));

done:
    return ret;
}

long libos_syscall_readlink(const char* pathname, char* buf, size_t bufsiz)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(pathname, suffix, &fs));
    ERAISE((*fs->fs_readlink)(fs, pathname, buf, bufsiz));

done:
    return ret;
}

long libos_syscall_symlink(const char* target, const char* linkpath)
{
    long ret = 0;
    char suffix[PATH_MAX];
    libos_fs_t* fs;

    ECHECK(libos_mount_resolve(linkpath, suffix, &fs));
    ERAISE((*fs->fs_symlink)(fs, target, suffix));

done:
    return ret;
}

static char _cwd[PATH_MAX] = "/";
static libos_spinlock_t _cwd_lock = LIBOS_SPINLOCK_INITIALIZER;

long libos_syscall_chdir(const char* path)
{
    long ret = 0;
    char buf[PATH_MAX];
    char buf2[PATH_MAX];

    libos_spin_lock(&_cwd_lock);

    if (!path)
        ERAISE(-EINVAL);

    ECHECK(libos_path_absolute_cwd(_cwd, path, buf, sizeof(buf)));
    ECHECK(libos_normalize(buf, buf2, sizeof(buf2)));

    if (LIBOS_STRLCPY(_cwd, buf2) >= sizeof(_cwd))
        ERAISE(-ERANGE);

done:

    libos_spin_unlock(&_cwd_lock);

    return ret;
}

long libos_syscall_getcwd(char* buf, size_t size)
{
    long ret = 0;

    libos_spin_lock(&_cwd_lock);

    if (!buf)
        ERAISE(-EINVAL);

    if (libos_strlcpy(buf, _cwd, size) >= size)
        ERAISE(-ERANGE);

    ret = (long)buf;

done:

    libos_spin_unlock(&_cwd_lock);

    return ret;
}

long libos_syscall_getrandom(void* buf, size_t buflen, unsigned int flags)
{
    long ret = 0;

    (void)flags;

    if (!buf && buflen)
        ERAISE(-EINVAL);

    if (buf && buflen && libos_tcall_random(buf, buflen) != 0)
        ERAISE(-EINVAL);

    ret = (long)buflen;

done:
    return ret;
}

long libos_syscall_fcntl(int fd, int cmd, long arg)
{
    long ret = 0;
    void* device = NULL;
    void* object = NULL;
    libos_fdtable_type_t type;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    libos_fdops_t* fdops;

    ECHECK(libos_fdtable_get_any(fdtable, fd, &type, &device, &object));
    fdops = device;

    ret = (*fdops->fd_fcntl)(device, object, cmd, arg);

done:
    return ret;
}

long libos_syscall_chmod(const char* pathname, mode_t mode)
{
    printf("pathname{%s} mode{%o}\n", pathname, mode);
    (void)pathname;
    (void)mode;
    return 0;
}

long libos_syscall_mount(
    const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long mountflags,
    const void* data)
{
    long ret = 0;
    libos_fs_t* fs = NULL;

    if (!source || !target || !filesystemtype)
        ERAISE(-EINVAL);

    /* only "ramfs" is supported */
    if (strcmp(filesystemtype, "ramfs") != 0)
        ERAISE(-ENOTSUP);

    /* these arguments should be zero and null */
    if (mountflags || data)
        ERAISE(-EINVAL);

    /* create a new ramfs instance */
    ECHECK(libos_init_ramfs(&fs));

    /* perform the mount */
    ECHECK(libos_mount(fs, target));

    /* load the rootfs */
    ECHECK(libos_cpio_unpack(source, target));

done:
    return ret;
}

long libos_syscall_umount2(const char* target, int flags)
{
    long ret = 0;

    if (!target || flags != 0)
        ERAISE(-EINVAL);

    ECHECK(libos_umount(target));

done:
    return ret;
}

long libos_syscall_pipe2(int pipefd[2], int flags)
{
    int ret = 0;
    libos_pipe_t* pipe[2];
    int fd0;
    int fd1;
    libos_fdtable_t* fdtable = libos_fdtable_current();
    const libos_fdtable_type_t type = LIBOS_FDTABLE_TYPE_PIPE;
    libos_pipedev_t* pd = libos_pipedev_get();

    if (!pipefd)
        ERAISE(-EINVAL);

    ECHECK((*pd->pd_pipe2)(pd, pipe, flags));

    if ((fd0 = libos_fdtable_assign(fdtable, type, pd, pipe[0])) < 0)
    {
        libos_panic("libos_fdtable_assign() failed");
    }

    if ((fd1 = libos_fdtable_assign(fdtable, type, pd, pipe[1])) < 0)
    {
        libos_panic("libos_fdtable_assign() failed");
    }

    pipefd[0] = fd0;
    pipefd[1] = fd1;

done:
    return ret;
}

static size_t _count_args(char* const args[])
{
    size_t n = 0;

    if (args)
    {
        while (*args++)
            n++;
    }

    return n;
}

long libos_syscall_execve(
    const char* filename,
    char* const argv[],
    char* const envp[])
{
    /* ATTN: this will fail if filename is different than argv[0] */
    (void)filename;

    /* only returns on failure */
    if (libos_exec(
            libos_thread_self(),
            __libos_kernel_args.crt_data,
            __libos_kernel_args.crt_size,
            __libos_kernel_args.crt_reloc_data,
            __libos_kernel_args.crt_reloc_size,
            _count_args(argv),
            (const char**)argv,
            _count_args(envp),
            (const char**)envp) != 0)
    {
        return -ENOENT;
    }

    /* unreachable */
    return 0;
}

long libos_syscall_ret(long ret)
{
    if (ret < 0)
    {
        errno = (int)-ret;
        ret = -1;
    }

    return ret;
}

static const char* _fcntl_cmdstr(int cmd)
{
    switch (cmd)
    {
        case F_DUPFD:
            return "F_DUPFD";
        case F_SETFD:
            return "F_SETFD";
        case F_GETFD:
            return "F_GETFD";
        case F_SETFL:
            return "F_SETFL";
        case F_GETFL:
            return "F_GETFL";
        case F_SETOWN:
            return "F_SETOWN";
        case F_GETOWN:
            return "F_GETOWN";
        case F_SETSIG:
            return "F_SETSIG";
        case F_GETSIG:
            return "F_GETSIG";
        case F_SETLK:
            return "F_SETLK";
        case F_GETLK:
            return "F_GETLK";
        case F_SETLKW:
            return "F_SETLKW";
        case F_SETOWN_EX:
            return "F_SETOWN_EX";
        case F_GETOWN_EX:
            return "F_GETOWN_EX";
        case F_GETOWNER_UIDS:
            return "F_GETOWNER_UIDS";
        default:
            return "unknown";
    }
}

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_FD 2
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_LOCK_PI 6
#define FUTEX_UNLOCK_PI 7
#define FUTEX_TRYLOCK_PI 8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_PRIVATE 128
#define FUTEX_CLOCK_REALTIME 256

static const char* _futex_op_str(int op)
{
    switch (op & ~FUTEX_PRIVATE)
    {
        case FUTEX_WAIT:
            return "FUTEX_WAIT";
        case FUTEX_WAKE:
            return "FUTEX_WAKE";
        case FUTEX_FD:
            return "FUTEX_FD";
        case FUTEX_REQUEUE:
            return "FUTEX_REQUEUE";
        case FUTEX_CMP_REQUEUE:
            return "FUTEX_CMP_REQUEUE";
        case FUTEX_WAKE_OP:
            return "FUTEX_WAKE_OP";
        case FUTEX_LOCK_PI:
            return "FUTEX_LOCK_PI";
        case FUTEX_UNLOCK_PI:
            return "FUTEX_UNLOCK_PI";
        case FUTEX_TRYLOCK_PI:
            return "FUTEX_TRYLOCK_PI";
        case FUTEX_WAIT_BITSET:
            return "FUTEX_WAIT_BITSET";
        default:
            return "UNKNOWN";
    }
}

static ssize_t _dev_urandom_read(void* buf, size_t count)
{
    ssize_t ret = 0;

    if (!buf && count)
        ERAISE(-EFAULT);

    if (!buf && !count)
        return 0;

    if (libos_tcall_random(buf, count) != 0)
        ERAISE(-EIO);

    ret = (ssize_t)count;

done:
    return ret;
}

static ssize_t _dev_urandom_readv(const struct iovec* iov, int iovcnt)
{
    ssize_t ret = 0;
    size_t nread = 0;

    if (!iov && iovcnt)
        ERAISE(-EINVAL);

    for (int i = 0; i < iovcnt; i++)
    {
        if (iov[i].iov_base && iov[i].iov_len)
        {
            if (libos_tcall_random(iov[i].iov_base, iov[i].iov_len) != 0)
                ERAISE(-EINVAL);

            nread += iov[i].iov_len;
        }
    }

    ret = (ssize_t)nread;

done:
    return ret;
}

void libos_dump_ramfs(void)
{
    libos_strarr_t paths = LIBOS_STRARR_INITIALIZER;

    if (libos_lsr("/", &paths, true) != 0)
        libos_panic("unexpected");

    for (size_t i = 0; i < paths.size; i++)
    {
        printf("%s\n", paths.data[i]);
    }

    libos_strarr_release(&paths);
}

int libos_export_ramfs(void)
{
    int ret = -1;
    libos_strarr_t paths = LIBOS_STRARR_INITIALIZER;
    void* data = NULL;
    size_t size = 0;

    if (libos_lsr("/", &paths, false) != 0)
        goto done;

    for (size_t i = 0; i < paths.size; i++)
    {
        const char* path = paths.data[i];

        /* Skip over entries in the /proc file system */
        if (strncmp(path, "/proc", 5) == 0)
            continue;

        if (libos_load_file(path, &data, &size) != 0)
        {
            libos_eprintf("Warning! failed to load %s from ramfs\n", path);
            continue;
        }

        if (libos_tcall_export_file(path, data, size) != 0)
        {
            libos_eprintf("Warning! failed to export %s from ramfs\n", path);
            continue;
        }

        free(data);
        data = NULL;
        size = 0;
    }

    ret = 0;

done:
    libos_strarr_release(&paths);

    if (data)
        free(data);

    return ret;
}

#define BREAK(RET)           \
    do                       \
    {                        \
        syscall_ret = (RET); \
        goto done;           \
    } while (0)

long libos_syscall(long n, long params[6])
{
    long syscall_ret = 0;
    long x1 = params[0];
    long x2 = params[1];
    long x3 = params[2];
    long x4 = params[3];
    long x5 = params[4];
    long x6 = params[5];
    static bool _set_thread_area_called;
    libos_td_t* target_td = NULL;
    libos_td_t* crt_td = NULL;
    libos_thread_t* thread = NULL;

    struct timespec tp_enter = libos_times_enter_kernel();

    /* resolve the target-thread-descriptor and the crt-thread-descriptor */
    if (_set_thread_area_called)
    {
        /* ---------- running C-runtime thread descriptor ---------- */

        /* get crt_td */
        crt_td = libos_get_fsbase();
        libos_assume(libos_valid_td(crt_td));

        /* get thread */
        libos_assume(libos_tcall_get_tsd((uint64_t*)&thread) == 0);
        libos_assume(libos_valid_thread(thread));

        /* get target_td */
        target_td = thread->target_td;
        libos_assume(libos_valid_td(target_td));

        /* the syscall on the target thread descriptor */
        libos_set_fsbase(target_td);
    }
    else
    {
        /* ---------- running target thread descriptor ---------- */

        /* get target_td */
        target_td = libos_get_fsbase();
        libos_assume(libos_valid_td(target_td));

        /* get thread */
        libos_assume(libos_tcall_get_tsd((uint64_t*)&thread) == 0);
        libos_assume(libos_valid_thread(thread));

        /* crt_td is null */
    }

    /* ---------- running target thread descriptor ---------- */

    libos_assume(target_td != NULL);
    libos_assume(thread != NULL);

    switch (n)
    {
#ifdef LIBOS_ENABLE_GCOV
        case SYS_libos_gcov_init:
        {
            libc_t* libc = (libc_t*)x1;
            FILE* stream = (FILE*)x2;

            _strace(n, "libc=%p stream=%p", libc, stream);

            if (gcov_init_libc(libc, stream) != 0)
                libos_panic("gcov_init_libc() failed");

            BREAK(_return(n, 0));
        }
#endif
        case SYS_libos_trace:
        {
            const char* msg = (const char*)x1;

            _strace(n, "msg=%s", msg);

            BREAK(_return(n, 0));
        }
        case SYS_libos_trace_ptr:
        {
            printf(
                "trace: %s: %lx %ld\n",
                (const char*)params[0],
                params[1],
                params[1]);
            BREAK(_return(n, 0));
        }
        case SYS_libos_dump_stack:
        {
            const void* stack = (void*)x1;

            _strace(n, NULL);

            libos_dump_stack((void*)stack);
            BREAK(_return(n, 0));
        }
        case SYS_libos_dump_ehdr:
        {
            libos_dump_ehdr((void*)params[0]);
            BREAK(_return(n, 0));
        }
        case SYS_libos_dump_argv:
        {
            int argc = (int)x1;
            const char** argv = (const char**)x2;

            printf("=== SYS_libos_dump_argv\n");

            printf("argc=%d\n", argc);
            printf("argv=%p\n", argv);

            for (int i = 0; i < argc; i++)
            {
                printf("argv[%d]=%s\n", i, argv[i]);
            }

            printf("argv[argc]=%p\n", argv[argc]);

            BREAK(_return(n, 0));
        }
        case SYS_libos_add_symbol_file:
        {
            const char* path = (const char*)x1;
            const void* text = (const void*)x2;
            size_t text_size = (size_t)x3;
            long ret;

            _strace(
                n,
                "path=\"%s\" text=%p text_size=%zu\n",
                path,
                text,
                text_size);

            ret = libos_syscall_add_symbol_file(path, text, text_size);

            BREAK(_return(n, ret));
        }
        case SYS_libos_load_symbols:
        {
            _strace(n, NULL);

            BREAK(_return(n, libos_syscall_load_symbols()));
        }
        case SYS_libos_unload_symbols:
        {
            _strace(n, NULL);

            BREAK(_return(n, libos_syscall_unload_symbols()));
        }
        case SYS_libos_gen_creds:
        {
            _strace(n, NULL);
            BREAK(_forward_syscall(LIBOS_TCALL_GEN_CREDS, params));
        }
        case SYS_libos_free_creds:
        {
            _strace(n, NULL);
            BREAK(_forward_syscall(LIBOS_TCALL_FREE_CREDS, params));
        }
        case SYS_libos_verify_cert:
        {
            _strace(n, NULL);
            BREAK(_forward_syscall(LIBOS_TCALL_VERIFY_CERT, params));
        }
        case SYS_libos_max_threads:
        {
            _strace(n, NULL);
            BREAK(_return(n, __libos_kernel_args.max_threads));
        }
        case SYS_read:
        {
            int fd = (int)x1;
            void* buf = (void*)x2;
            size_t count = (size_t)x3;

            _strace(n, "fd=%d buf=%p count=%zu", fd, buf, count);

            if (fd == DEV_URANDOM_FD)
                BREAK(_return(n, _dev_urandom_read(buf, count)));

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_read(fd, buf, count)));
        }
        case SYS_write:
        {
            int fd = (int)x1;
            const void* buf = (const void*)x2;
            size_t count = (size_t)x3;

            _strace(n, "fd=%d buf=%p count=%zu", fd, buf, count);

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_write(fd, buf, count)));
        }
        case SYS_pread64:
        {
            int fd = (int)x1;
            void* buf = (void*)x2;
            size_t count = (size_t)x3;
            off_t offset = (off_t)x4;

            _strace(
                n, "fd=%d buf=%p count=%zu offset=%ld", fd, buf, count, offset);

            if (fd == DEV_URANDOM_FD)
                BREAK(_return(n, _dev_urandom_read(buf, count)));

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_pread(fd, buf, count, offset)));
        }
        case SYS_pwrite64:
        {
            int fd = (int)x1;
            void* buf = (void*)x2;
            size_t count = (size_t)x3;
            off_t offset = (off_t)x4;

            _strace(
                n, "fd=%d buf=%p count=%zu offset=%ld", fd, buf, count, offset);

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_pwrite(fd, buf, count, offset)));
        }
        case SYS_open:
        {
            const char* path = (const char*)x1;
            int flags = (int)x2;
            mode_t mode = (mode_t)x3;
            long ret;

            _strace(n, "path=\"%s\" flags=0%o mode=0%o", path, flags, mode);

            ret = libos_syscall_open(path, flags, mode);

            BREAK(_return(n, ret));
        }
        case SYS_close:
        {
            int fd = (int)x1;

            _strace(n, "fd=%d", fd);

            if (fd == DEV_URANDOM_FD)
                BREAK(_return(n, 0));

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_close(fd)));
        }
        case SYS_stat:
        {
            const char* pathname = (const char*)x1;
            struct stat* statbuf = (struct stat*)x2;

            _strace(n, "pathname=\"%s\" statbuf=%p", pathname, statbuf);

            BREAK(_return(n, libos_syscall_stat(pathname, statbuf)));
        }
        case SYS_fstat:
        {
            int fd = (int)x1;
            void* statbuf = (void*)x2;

            _strace(n, "fd=%d statbuf=%p", fd, statbuf);

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_fstat(fd, statbuf)));
        }
        case SYS_lstat:
        {
            /* ATTN: remove this! */
            const char* pathname = (const char*)x1;
            struct stat* statbuf = (struct stat*)x2;

            _strace(n, "pathname=\"%s\" statbuf=%p", pathname, statbuf);

            BREAK(_return(n, libos_syscall_lstat(pathname, statbuf)));
        }
        case SYS_poll:
        {
            _strace(n, NULL);
            BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_lseek:
        {
            int fd = (int)x1;
            off_t offset = (off_t)x2;
            int whence = (int)x3;

            _strace(n, "fd=%d offset=%ld whence=%d", fd, offset, whence);

            if (fd == DEV_URANDOM_FD)
            {
                /* ATTN: ignored */
                BREAK(_return(n, 0));
            }

            BREAK(_return(n, libos_syscall_lseek(fd, offset, whence)));
        }
        case SYS_mmap:
        {
            void* addr = (void*)x1;
            size_t length = (size_t)x2;
            int prot = (int)x3;
            int flags = (int)x4;
            int fd = (int)x5;
            off_t offset = (off_t)x6;

            _strace(
                n,
                "addr=%lx length=%zu(%lx) prot=%d flags=%d fd=%d offset=%lu",
                (long)addr,
                length,
                length,
                prot,
                flags,
                fd,
                offset);

            BREAK(_return(
                n, (long)libos_mmap(addr, length, prot, flags, fd, offset)));
        }
        case SYS_mprotect:
        {
            const void* addr = (void*)x1;
            const size_t length = (size_t)x2;
            const int prot = (int)x3;

            _strace(
                n,
                "addr=%lx length=%zu(%lx) prot=%d",
                (long)addr,
                length,
                length,
                prot);

            BREAK(_return(n, 0));
        }
        case SYS_munmap:
        {
            void* addr = (void*)x1;
            size_t length = (size_t)x2;

            _strace(n, "addr=%lx length=%zu(%lx)", (long)addr, length, length);

            BREAK(_return(n, (long)libos_munmap(addr, length)));
        }
        case SYS_brk:
        {
            void* addr = (void*)x1;

            _strace(n, "addr=%lx", (long)addr);

            BREAK(_return(n, libos_syscall_brk(addr)));
        }
        case SYS_rt_sigaction:
        {
            int signum = (int)x1;
            const struct sigaction* act = (const struct sigaction*)x2;
            struct sigaction* oldact = (struct sigaction*)x3;

            /* ATTN: silently ignore since SYS_kill is not supported */
            _strace(n, "signum=%d act=%p oldact=%p", signum, act, oldact);

            BREAK(_return(n, 0));
        }
        case SYS_rt_sigprocmask:
        {
            _strace(n, NULL);
            BREAK(_return(n, 0));
        }
        case SYS_rt_sigreturn:
            break;
        case SYS_ioctl:
        {
            int fd = (int)x1;
            unsigned long request = (unsigned long)x2;
            void* arg = (void*)x3;
            int iarg = -1;

            if (request == FIONBIO && arg)
                iarg = *(int*)arg;

            _strace(
                n,
                "fd=%d request=0x%lx arg=%p iarg=%d",
                fd,
                request,
                arg,
                iarg);

            if (libos_is_libos_fd(fd))
            {
                if (request == TIOCGWINSZ)
                {
                    /* Fail because no libos fd can be a console device */
                    BREAK(_return(n, -EINVAL));
                }

                libos_panic("unhandled ioctl: 0x%lx()", request);
            }

            BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_readv:
        {
            int fd = (int)x1;
            const struct iovec* iov = (const struct iovec*)x2;
            int iovcnt = (int)x3;

            _strace(n, "fd=%d iov=%p iovcnt=%d", fd, iov, iovcnt);

            if (fd == DEV_URANDOM_FD)
                BREAK(_return(n, (long)_dev_urandom_readv(iov, iovcnt)));

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_readv(fd, iov, iovcnt)));
        }
        case SYS_writev:
        {
            int fd = (int)x1;
            const struct iovec* iov = (const struct iovec*)x2;
            int iovcnt = (int)x3;

            _strace(n, "fd=%d iov=%p iovcnt=%d", fd, iov, iovcnt);

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_writev(fd, iov, iovcnt)));
        }
        case SYS_access:
        {
            const char* pathname = (const char*)x1;
            int mode = (int)x2;

            _strace(n, "pathname=\"%s\" mode=%d", pathname, mode);

            BREAK(_return(n, libos_syscall_access(pathname, mode)));
        }
        case SYS_pipe:
        {
            int* pipefd = (int*)x1;

            _strace(n, "pipefd=%p flags=%0o", pipefd, 0);

            BREAK(_return(n, libos_syscall_pipe2(pipefd, 0)));
        }
        case SYS_select:
        {
            int nfds = (int)x1;
            fd_set* rfds = (fd_set*)x2;
            fd_set* wfds = (fd_set*)x3;
            fd_set* efds = (fd_set*)x4;
            struct timeval* timeout = (struct timeval*)x5;

            _strace(
                n,
                "nfds=%d rfds=%p wfds=%p xfds=%p timeout=%p",
                nfds,
                rfds,
                wfds,
                efds,
                timeout);

            BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_sched_yield:
            break;
        case SYS_mremap:
        {
            void* old_address = (void*)x1;
            size_t old_size = (size_t)x2;
            size_t new_size = (size_t)x3;
            int flags = (int)x4;
            void* new_address = (void*)x5;
            long ret;

            _strace(
                n,
                "old_address=%p "
                "old_size=%zu "
                "new_size=%zu "
                "flags=%d "
                "new_address=%p ",
                old_address,
                old_size,
                new_size,
                flags,
                new_address);

            ret = (long)libos_mremap(
                old_address, old_size, new_size, flags, new_address);

            BREAK(_return(n, ret));
        }
        case SYS_msync:
            /* ATTN: hook up implementation */
            break;
        case SYS_mincore:
            /* ATTN: hook up implementation */
            break;
        case SYS_madvise:
        {
            void* addr = (void*)x1;
            size_t length = (size_t)x2;
            int advice = (int)x3;

            _strace(n, "addr=%p length=%zu advice=%d", addr, length, advice);

            BREAK(_return(n, 0));
        }
        case SYS_shmget:
            break;
        case SYS_shmat:
            break;
        case SYS_shmctl:
            break;
        case SYS_dup:
            break;
        case SYS_dup2:
            break;
        case SYS_pause:
            break;
        case SYS_nanosleep:
        {
            const struct timespec* req = (const struct timespec*)x1;
            struct timespec* rem = (struct timespec*)x2;

            _strace(n, "req=%p rem=%p", req, rem);

            BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_getitimer:
            break;
        case SYS_alarm:
            break;
        case SYS_setitimer:
            break;
        case SYS_getpid:
        {
            _strace(n, NULL);
            BREAK(_return(n, libos_getpid()));
        }
        case SYS_clone:
        {
            /* unsupported: using SYS_libos_clone instead */
            break;
        }
        case SYS_libos_clone:
        {
            long* args = (long*)x1;
            int (*fn)(void*) = (void*)args[0];
            void* child_stack = (void*)args[1];
            int flags = (int)args[2];
            void* arg = (void*)args[3];
            pid_t* ptid = (pid_t*)args[4];
            void* newtls = (void*)args[5];
            pid_t* ctid = (void*)args[6];

            _strace(
                n,
                "fn=%p "
                "child_stack=%p "
                "flags=%x "
                "arg=%p "
                "ptid=%p "
                "newtls=%p "
                "ctid=%p",
                fn,
                child_stack,
                flags,
                arg,
                ptid,
                newtls,
                ctid);

            BREAK(_return(
                n,
                libos_syscall_clone(
                    fn, child_stack, flags, arg, ptid, newtls, ctid)));
        }
        case SYS_fork:
            break;
        case SYS_vfork:
            break;
        case SYS_execve:
        {
            const char* filename = (const char*)x1;
            char** argv = (char**)x2;
            char** envp = (char**)x3;

            _strace(n, "filename=%s argv=%p envp=%p", filename, argv, envp);

            long ret = libos_syscall_execve(filename, argv, envp);
            BREAK(_return(n, ret));
        }
        case SYS_exit:
        {
            const int status = (int)x1;
            libos_thread_t* thread = libos_thread_self();

            _strace(n, "status=%d", status);

            if (!thread || thread->magic != LIBOS_THREAD_MAGIC)
                libos_panic("unexpected");

            thread->exit_status = status;

            if (thread == __libos_main_thread)
            {
                // execute fini functions with the CRT fsbase since only
                // gcov uses them and gcov calls into CRT.
                libos_set_fsbase(crt_td);
                libos_call_fini_functions();
                libos_set_fsbase(target_td);

                if (__options.export_ramfs)
                    libos_export_ramfs();
            }

            libos_longjmp(&thread->jmpbuf, 1);
            /* unreachable */
            break;
        }
        case SYS_wait4:
        {
            pid_t pid = (pid_t)x1;
            int* wstatus = (int*)x2;
            int options = (int)x3;
            struct rusage* rusage = (struct rusage*)x4;
            long ret;

            ret = libos_syscall_wait4(pid, wstatus, options, rusage);
            BREAK(_return(n, ret));
        }
        case SYS_kill:
            break;
        case SYS_uname:
        {
            struct utsname* buf = (struct utsname*)x1;

            LIBOS_STRLCPY(buf->sysname, "OpenLibos");
            LIBOS_STRLCPY(buf->nodename, "libos");
            LIBOS_STRLCPY(buf->release, "1.0.0");
            LIBOS_STRLCPY(buf->version, "Libos 1.0.0");
            LIBOS_STRLCPY(buf->machine, "x86_64");

            BREAK(_return(n, 0));
        }
        case SYS_semget:
            break;
        case SYS_semop:
            break;
        case SYS_semctl:
            break;
        case SYS_shmdt:
            break;
        case SYS_msgget:
            break;
        case SYS_msgsnd:
            break;
        case SYS_msgrcv:
            break;
        case SYS_msgctl:
            break;
        case SYS_fcntl:
        {
            int fd = (int)x1;
            int cmd = (int)x2;
            long arg = (long)x3;

            const char* cmdstr = _fcntl_cmdstr(cmd);
            _strace(n, "fd=%d cmd=%d(%s) arg=0%lo", fd, cmd, cmdstr, arg);

            if (!libos_is_libos_fd(fd))
                BREAK(_return(n, _forward_syscall(n, params)));

            BREAK(_return(n, libos_syscall_fcntl(fd, cmd, arg)));
        }
        case SYS_flock:
            break;
        case SYS_fsync:
            break;
        case SYS_fdatasync:
            break;
        case SYS_truncate:
        {
            const char* path = (const char*)x1;
            off_t length = (off_t)x2;

            _strace(n, "path=\"%s\" length=%ld", path, length);

            BREAK(_return(n, libos_syscall_truncate(path, length)));
        }
        case SYS_ftruncate:
        {
            int fd = (int)x1;
            off_t length = (off_t)x2;

            _strace(n, "fd=%d length=%ld", fd, length);

            BREAK(_return(n, libos_syscall_ftruncate(fd, length)));
        }
        case SYS_getdents:
            break;
        case SYS_getcwd:
        {
            char* buf = (char*)x1;
            size_t size = (size_t)x2;

            _strace(n, "buf=%p size=%zu", buf, size);

            BREAK(_return(n, libos_syscall_getcwd(buf, size)));
        }
        case SYS_chdir:
        {
            const char* path = (const char*)x1;

            _strace(n, "path=\"%s\"", path);

            BREAK(_return(n, libos_syscall_chdir(path)));
        }
        case SYS_fchdir:
            break;
        case SYS_rename:
        {
            const char* oldpath = (const char*)x1;
            const char* newpath = (const char*)x2;

            _strace(n, "oldpath=\"%s\" newpath=\"%s\"", oldpath, newpath);

            BREAK(_return(n, libos_syscall_rename(oldpath, newpath)));
        }
        case SYS_mkdir:
        {
            const char* pathname = (const char*)x1;
            mode_t mode = (mode_t)x2;

            _strace(n, "pathname=\"%s\" mode=0%o", pathname, mode);

            BREAK(_return(n, libos_syscall_mkdir(pathname, mode)));
        }
        case SYS_rmdir:
        {
            const char* pathname = (const char*)x1;

            _strace(n, "pathname=\"%s\"", pathname);

            BREAK(_return(n, libos_syscall_rmdir(pathname)));
        }
        case SYS_creat:
        {
            const char* pathname = (const char*)x1;
            mode_t mode = (mode_t)x2;

            _strace(n, "pathname=\"%s\" mode=%x", pathname, mode);

            BREAK(_return(n, libos_syscall_creat(pathname, mode)));
        }
        case SYS_link:
        {
            const char* oldpath = (const char*)x1;
            const char* newpath = (const char*)x2;

            _strace(n, "oldpath=\"%s\" newpath=\"%s\"", oldpath, newpath);

            BREAK(_return(n, libos_syscall_link(oldpath, newpath)));
        }
        case SYS_unlink:
        {
            const char* pathname = (const char*)x1;

            _strace(n, "pathname=\"%s\"", pathname);

            BREAK(_return(n, libos_syscall_unlink(pathname)));
        }
        case SYS_symlink:
        {
            const char* target = (const char*)x1;
            const char* linkpath = (const char*)x2;

            _strace(n, "target=\"%s\" linkpath=\"%s\"", target, linkpath);

            BREAK(_return(n, libos_syscall_symlink(target, linkpath)));
        }
        case SYS_readlink:
        {
            const char* pathname = (const char*)x1;
            char* buf = (char*)x2;
            size_t bufsiz = (size_t)x3;

            _strace(
                n, "pathname=\"%s\" buf=%p bufsiz=%zu", pathname, buf, bufsiz);

            BREAK(_return(n, libos_syscall_readlink(pathname, buf, bufsiz)));
        }
        case SYS_chmod:
        {
            const char* pathname = (const char*)x1;
            mode_t mode = (mode_t)x2;

            _strace(n, "pathname=\"%s\" mode=%o", pathname, mode);

            BREAK(_return(n, libos_syscall_chmod(pathname, mode)));
        }
        case SYS_fchmod:
            break;
        case SYS_chown:
            break;
        case SYS_fchown:
            break;
        case SYS_lchown:
            break;
        case SYS_umask:
            break;
        case SYS_gettimeofday:
        {
            struct timeval* tv = (struct timeval*)x1;
            struct timezone* tz = (void*)x2;

            _strace(n, "tv=%p tz=%p", tv, tz);

            long ret = libos_syscall_gettimeofday(tv, tz);
            BREAK(_return(n, ret));
        }
        case SYS_getrlimit:
            break;
        case SYS_getrusage:
            break;
        case SYS_sysinfo:
            break;
        case SYS_times:
        {
            struct tms* tm = (struct tms*)x1;
            _strace(n, "tm=%p", tm);

            long stime = libos_times_system_time();
            long utime = libos_times_user_time();
            if (tm != NULL)
            {
                tm->tms_utime = utime;
                tm->tms_stime = stime;
                tm->tms_cutime = 0;
                tm->tms_cstime = 0;
            }

            BREAK(_return(n, stime + utime));
        }
        case SYS_ptrace:
            break;
        case SYS_getuid:
        {
            _strace(n, NULL);
            BREAK(_return(n, LIBOS_DEFAULT_UID));
        }
        case SYS_syslog:
        {
            /* Ignore syslog for now */
            BREAK(_return(n, 0));
        }
        case SYS_getgid:
        {
            _strace(n, NULL);
            BREAK(_return(n, LIBOS_DEFAULT_GID));
        }
        case SYS_setuid:
            break;
        case SYS_setgid:
            break;
        case SYS_geteuid:
        {
            _strace(n, NULL);
            BREAK(_return(n, LIBOS_DEFAULT_UID));
        }
        case SYS_getegid:
        {
            _strace(n, NULL);
            BREAK(_return(n, LIBOS_DEFAULT_GID));
        }
        case SYS_setpgid:
            break;
        case SYS_getppid:
        {
            _strace(n, NULL);
            BREAK(_return(n, libos_getppid()));
        }
        case SYS_getpgrp:
            break;
        case SYS_setsid:
            break;
        case SYS_setreuid:
            break;
        case SYS_setregid:
            break;
        case SYS_getgroups:
            break;
        case SYS_setgroups:
            break;
        case SYS_setresuid:
            break;
        case SYS_getresuid:
            break;
        case SYS_setresgid:
            break;
        case SYS_getresgid:
            break;
        case SYS_getpgid:
            break;
        case SYS_setfsuid:
            break;
        case SYS_setfsgid:
            break;
        case SYS_getsid:
        {
            _strace(n, NULL);
            BREAK(_return(n, libos_getsid()));
        }
        case SYS_capget:
            break;
        case SYS_capset:
            break;
        case SYS_rt_sigpending:
            break;
        case SYS_rt_sigtimedwait:
            break;
        case SYS_rt_sigqueueinfo:
            break;
        case SYS_rt_sigsuspend:
            break;
        case SYS_sigaltstack:
            break;
        case SYS_utime:
            break;
        case SYS_mknod:
            break;
        case SYS_uselib:
            break;
        case SYS_personality:
            break;
        case SYS_ustat:
            break;
        case SYS_statfs:
        {
            const char* path = (const char*)x1;
            struct statfs* buf = (struct statfs*)x2;

            _strace(n, "path=%s buf=%p", path, buf);

            if (buf)
                memset(buf, 0, sizeof(*buf));

            BREAK(_return(n, 0));
        }
        case SYS_fstatfs:
            break;
        case SYS_sysfs:
            break;
        case SYS_getpriority:
            break;
        case SYS_setpriority:
            break;
        case SYS_sched_setparam:
            break;
        case SYS_sched_getparam:
            break;
        case SYS_sched_setscheduler:
            break;
        case SYS_sched_getscheduler:
            break;
        case SYS_sched_get_priority_max:
            break;
        case SYS_sched_get_priority_min:
            break;
        case SYS_sched_rr_get_interval:
            break;
        case SYS_mlock:
            break;
        case SYS_munlock:
            break;
        case SYS_mlockall:
            break;
        case SYS_munlockall:
            break;
        case SYS_vhangup:
            break;
        case SYS_modify_ldt:
            break;
        case SYS_pivot_root:
            break;
        case SYS__sysctl:
            break;
        case SYS_prctl:
            break;
        case SYS_arch_prctl:
            break;
        case SYS_adjtimex:
            break;
        case SYS_setrlimit:
            break;
        case SYS_chroot:
            break;
        case SYS_sync:
            break;
        case SYS_acct:
            break;
        case SYS_settimeofday:
            break;
        case SYS_mount:
        {
            const char* source = (const char*)x1;
            const char* target = (const char*)x2;
            const char* filesystemtype = (const char*)x3;
            unsigned long mountflags = (unsigned long)x4;
            void* data = (void*)x5;
            long ret;

            _strace(
                n,
                "source=%s target=%s filesystemtype=%s mountflags=%lu data=%p",
                source,
                target,
                filesystemtype,
                mountflags,
                data);

            ret = libos_syscall_mount(
                source, target, filesystemtype, mountflags, data);

            BREAK(_return(n, ret));
        }
        case SYS_umount2:
        {
            const char* target = (const char*)x1;
            int flags = (int)x2;
            long ret;

            _strace(n, "target=%p flags=%d", target, flags);

            ret = libos_syscall_umount2(target, flags);

            BREAK(_return(n, ret));
        }
        case SYS_swapon:
            break;
        case SYS_swapoff:
            break;
        case SYS_reboot:
            break;
        case SYS_sethostname:
        {
            const char* name = (const char*)x1;
            size_t len = (size_t)x2;

            _strace(n, "name=\"%s\" len=%zu", name, len);

            BREAK(0);
            // BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_setdomainname:
            break;
        case SYS_iopl:
            break;
        case SYS_ioperm:
            break;
        case SYS_create_module:
            break;
        case SYS_init_module:
            break;
        case SYS_delete_module:
            break;
        case SYS_get_kernel_syms:
            break;
        case SYS_query_module:
            break;
        case SYS_quotactl:
            break;
        case SYS_nfsservctl:
            break;
        case SYS_getpmsg:
            break;
        case SYS_putpmsg:
            break;
        case SYS_afs_syscall:
            break;
        case SYS_tuxcall:
            break;
        case SYS_security:
            break;
        case SYS_gettid:
        {
            _strace(n, NULL);
            BREAK(_return(n, libos_gettid()));
        }
        case SYS_readahead:
            break;
        case SYS_setxattr:
            break;
        case SYS_lsetxattr:
            break;
        case SYS_fsetxattr:
            break;
        case SYS_getxattr:
            break;
        case SYS_lgetxattr:
            break;
        case SYS_fgetxattr:
            break;
        case SYS_listxattr:
            break;
        case SYS_llistxattr:
            break;
        case SYS_flistxattr:
            break;
        case SYS_removexattr:
            break;
        case SYS_lremovexattr:
            break;
        case SYS_fremovexattr:
            break;
        case SYS_tkill:
            break;
        case SYS_time:
        {
            time_t* tloc = (time_t*)x1;

            _strace(n, "tloc=%p", tloc);
            long ret = libos_syscall_time(tloc);
            BREAK(_return(n, ret));
        }
        case SYS_futex:
        {
            int* uaddr = (int*)x1;
            int futex_op = (int)x2;
            int val = (int)x3;
            long arg = (long)x4;
            int* uaddr2 = (int*)x5;
            int val3 = (int)val3;

            _strace(
                n,
                "uaddr=0x%lx(%d) futex_op=%u(%s) val=%d",
                (long)uaddr,
                (uaddr ? *uaddr : -1),
                futex_op,
                _futex_op_str(futex_op),
                val);

            BREAK(_return(
                n,
                libos_syscall_futex(uaddr, futex_op, val, arg, uaddr2, val3)));
        }
        case SYS_sched_setaffinity:
            break;
        case SYS_sched_getaffinity:
            break;
        case SYS_set_thread_area:
        {
            void* tp = (void*)params[0];

            _strace(n, "tp=%p", tp);

            /* ---------- running target thread descriptor ---------- */

#ifdef DISABLE_MULTIPLE_SET_THREAD_AREA_SYSCALLS
            if (_set_thread_area_called)
                libos_panic("SYS_set_thread_area called twice");
#endif

            /* get the C-runtime thread descriptor */
            crt_td = (libos_td_t*)tp;
            assert(libos_valid_td(crt_td));

            /* set the C-runtime thread descriptor for this thread */
            thread->crt_td = crt_td;

            /* propagate the canary from the old thread descriptor */
            crt_td->canary = target_td->canary;

            _set_thread_area_called = true;

            BREAK(_return(n, 0));
        }
        case SYS_io_setup:
            break;
        case SYS_io_destroy:
            break;
        case SYS_io_getevents:
            break;
        case SYS_io_submit:
            break;
        case SYS_io_cancel:
            break;
        case SYS_get_thread_area:
            break;
        case SYS_lookup_dcookie:
            break;
        case SYS_epoll_create:
            break;
        case SYS_epoll_ctl_old:
            break;
        case SYS_epoll_wait_old:
            break;
        case SYS_remap_file_pages:
            break;
        case SYS_getdents64:
        {
            unsigned int fd = (unsigned int)x1;
            struct dirent* dirp = (struct dirent*)x2;
            unsigned int count = (unsigned int)x3;

            _strace(n, "fd=%d dirp=%p count=%u", fd, dirp, count);

            BREAK(_return(n, libos_syscall_getdents64((int)fd, dirp, count)));
        }
        case SYS_set_tid_address:
        {
            int* tidptr = (int*)params[0];

            /* ATTN: unused */

            _strace(n, "tidptr=%p *tidptr=%d", tidptr, tidptr ? *tidptr : -1);

            BREAK(_return(n, libos_getpid()));
        }
        case SYS_restart_syscall:
            break;
        case SYS_semtimedop:
            break;
        case SYS_fadvise64:
            break;
        case SYS_timer_create:
            break;
        case SYS_timer_settime:
            break;
        case SYS_timer_gettime:
            break;
        case SYS_timer_getoverrun:
            break;
        case SYS_timer_delete:
            break;
        case SYS_clock_settime:
        {
            clockid_t clk_id = (clockid_t)x1;
            struct timespec* tp = (struct timespec*)x2;

            _strace(n, "clk_id=%u tp=%p", clk_id, tp);

            BREAK(_return(n, libos_syscall_clock_settime(clk_id, tp)));
        }
        case SYS_clock_gettime:
        {
            clockid_t clk_id = (clockid_t)x1;
            struct timespec* tp = (struct timespec*)x2;

            _strace(n, "clk_id=%u tp=%p", clk_id, tp);

            BREAK(_return(n, libos_syscall_clock_gettime(clk_id, tp)));
        }
        case SYS_clock_getres:
            break;
        case SYS_clock_nanosleep:
            break;
        case SYS_exit_group:
        {
            int status = (int)x1;

            _strace(n, "status=%d", status);

            BREAK(0);
        }
        case SYS_epoll_wait:
            break;
        case SYS_epoll_ctl:
            break;
        case SYS_tgkill:
            break;
        case SYS_utimes:
            break;
        case SYS_vserver:
            break;
        case SYS_mbind:
            break;
        case SYS_set_mempolicy:
            break;
        case SYS_get_mempolicy:
            break;
        case SYS_mq_open:
            break;
        case SYS_mq_unlink:
            break;
        case SYS_mq_timedsend:
            break;
        case SYS_mq_timedreceive:
            break;
        case SYS_mq_notify:
            break;
        case SYS_mq_getsetattr:
            break;
        case SYS_kexec_load:
            break;
        case SYS_waitid:
            break;
        case SYS_add_key:
            break;
        case SYS_request_key:
            break;
        case SYS_keyctl:
            break;
        case SYS_ioprio_set:
            break;
        case SYS_ioprio_get:
            break;
        case SYS_inotify_init:
            break;
        case SYS_inotify_add_watch:
            break;
        case SYS_inotify_rm_watch:
            break;
        case SYS_migrate_pages:
            break;
        case SYS_openat:
            break;
        case SYS_mkdirat:
            break;
        case SYS_mknodat:
            break;
        case SYS_fchownat:
            break;
        case SYS_futimesat:
            break;
        case SYS_newfstatat:
            break;
        case SYS_unlinkat:
            break;
        case SYS_renameat:
            break;
        case SYS_linkat:
            break;
        case SYS_symlinkat:
            break;
        case SYS_readlinkat:
            break;
        case SYS_fchmodat:
            break;
        case SYS_faccessat:
            break;
        case SYS_pselect6:
            break;
        case SYS_ppoll:
            break;
        case SYS_unshare:
            break;
        case SYS_set_robust_list:
            break;
        case SYS_get_robust_list:
            break;
        case SYS_splice:
            break;
        case SYS_tee:
            break;
        case SYS_sync_file_range:
            break;
        case SYS_vmsplice:
            break;
        case SYS_move_pages:
            break;
        case SYS_utimensat:
            break;
        case SYS_epoll_pwait:
            break;
        case SYS_signalfd:
            break;
        case SYS_timerfd_create:
            break;
        case SYS_eventfd:
            break;
        case SYS_fallocate:
            break;
        case SYS_timerfd_settime:
            break;
        case SYS_timerfd_gettime:
            break;
        case SYS_accept4:
            break;
        case SYS_signalfd4:
            break;
        case SYS_eventfd2:
            break;
        case SYS_epoll_create1:
            break;
        case SYS_dup3:
            break;
        case SYS_pipe2:
        {
            int* pipefd = (int*)x1;
            int flags = (int)x2;

            _strace(n, "pipefd=%p flags=%0o", pipefd, flags);

            BREAK(_return(n, libos_syscall_pipe2(pipefd, flags)));
        }
        case SYS_inotify_init1:
            break;
        case SYS_preadv:
            break;
        case SYS_pwritev:
            break;
        case SYS_rt_tgsigqueueinfo:
            break;
        case SYS_perf_event_open:
            break;
        case SYS_recvmmsg:
            break;
        case SYS_fanotify_init:
            break;
        case SYS_fanotify_mark:
            break;
        case SYS_prlimit64:
            break;
        case SYS_name_to_handle_at:
            break;
        case SYS_open_by_handle_at:
            break;
        case SYS_clock_adjtime:
            break;
        case SYS_syncfs:
            break;
        case SYS_sendmmsg:
            break;
        case SYS_setns:
            break;
        case SYS_getcpu:
            break;
        case SYS_process_vm_readv:
            break;
        case SYS_process_vm_writev:
            break;
        case SYS_kcmp:
            break;
        case SYS_finit_module:
            break;
        case SYS_sched_setattr:
            break;
        case SYS_sched_getattr:
            break;
        case SYS_renameat2:
            break;
        case SYS_seccomp:
            break;
        case SYS_getrandom:
        {
            void* buf = (void*)x1;
            size_t buflen = (size_t)x2;
            unsigned int flags = (unsigned int)x3;

            _strace(n, "buf=%p buflen=%zu flags=%d", buf, buflen, flags);

            BREAK(_return(n, libos_syscall_getrandom(buf, buflen, flags)));
        }
        case SYS_memfd_create:
            break;
        case SYS_kexec_file_load:
            break;
        case SYS_bpf:
            break;
        case SYS_execveat:
            break;
        case SYS_userfaultfd:
            break;
        case SYS_membarrier:
        {
            int cmd = (int)x1;
            int flags = (int)x2;

            _strace(n, "cmd=%d flags=%d", cmd, flags);

            libos_barrier();

            BREAK(_return(n, 0));
        }
        case SYS_mlock2:
            break;
        case SYS_copy_file_range:
            break;
        case SYS_preadv2:
            break;
        case SYS_pwritev2:
            break;
        case SYS_pkey_mprotect:
            break;
        case SYS_pkey_alloc:
            break;
        case SYS_pkey_free:
            break;
        case SYS_statx:
            break;
        case SYS_io_pgetevents:
            break;
        case SYS_rseq:
            break;
        case SYS_bind:
        case SYS_connect:
        {
            /* connect() and bind() have the same parameters */
            int sockfd = (int)x1;
            const struct sockaddr* addr = (const struct sockaddr*)x2;
            socklen_t addrlen = (socklen_t)x3;
            const uint8_t* p = (uint8_t*)addr->sa_data;
            uint16_t port = (uint16_t)((p[0] << 8) | p[1]);
            const uint8_t ip1 = p[2];
            const uint8_t ip2 = p[3];
            const uint8_t ip3 = p[4];
            const uint8_t ip4 = p[5];

            _strace(
                n,
                "sockfd=%d addrlen=%u family=%u ip=%u.%u.%u.%u port=%u",
                sockfd,
                addrlen,
                addr->sa_family,
                ip1,
                ip2,
                ip3,
                ip4,
                port);

            BREAK(_return(n, _forward_syscall(n, params)));
        }
        case SYS_recvfrom:
        {
            int sockfd = (int)x1;
            void* buf = (void*)x2;
            size_t len = (size_t)x3;
            int flags = (int)x4;
            struct sockaddr* src_addr = (struct sockaddr*)x5;
            socklen_t* addrlen = (socklen_t*)x6;
            long ret = 0;

            _strace(
                n,
                "sockfd=%d buf=%p len=%zu flags=%d src_addr=%p addrlen=%p",
                sockfd,
                buf,
                len,
                flags,
                src_addr,
                addrlen);

            for (size_t i = 0; i < 10; i++)
            {
                ret = _forward_syscall(n, params);

                if (ret != -EAGAIN)
                    break;

                {
                    struct timespec req;
                    req.tv_sec = 0;
                    req.tv_nsec = 1000000000 / 10;
                    long args[6];
                    args[0] = (long)&req;
                    args[1] = (long)NULL;
                    _forward_syscall(SYS_nanosleep, args);
                    continue;
                }
            }

            BREAK(_return(n, ret));
        }
        /* forward network syscdalls to OE */
        case SYS_sendfile:
        case SYS_socket:
        case SYS_accept:
        case SYS_sendto:
        case SYS_sendmsg:
        case SYS_recvmsg:
        case SYS_shutdown:
        case SYS_listen:
        case SYS_getsockname:
        case SYS_getpeername:
        case SYS_socketpair:
        case SYS_setsockopt:
        case SYS_getsockopt:
        case SYS_libos_oe_add_vectored_exception_handler:
        case SYS_libos_oe_remove_vectored_exception_handler:
        case SYS_libos_oe_is_within_enclave:
        case SYS_libos_oe_is_outside_enclave:
        case SYS_libos_oe_host_malloc:
        case SYS_libos_oe_host_realloc:
        case SYS_libos_oe_host_calloc:
        case SYS_libos_oe_host_free:
        case SYS_libos_oe_strndup:
        case SYS_libos_oe_abort:
        case SYS_libos_oe_assert_fail:
        case SYS_libos_oe_get_report_v2:
        case SYS_libos_oe_free_report:
        case SYS_libos_oe_get_target_info_v2:
        case SYS_libos_oe_free_target_info:
        case SYS_libos_oe_parse_report:
        case SYS_libos_oe_verify_report:
        case SYS_libos_oe_get_seal_key_by_policy_v2:
        case SYS_libos_oe_get_public_key_by_policy:
        case SYS_libos_oe_get_public_key:
        case SYS_libos_oe_get_private_key_by_policy:
        case SYS_libos_oe_get_private_key:
        case SYS_libos_oe_free_key:
        case SYS_libos_oe_get_seal_key_v2:
        case SYS_libos_oe_free_seal_key:
        case SYS_libos_oe_get_enclave:
        case SYS_libos_oe_random:
        case SYS_libos_oe_generate_attestation_certificate:
        case SYS_libos_oe_free_attestation_certificate:
        case SYS_libos_oe_verify_attestation_certificate:
        case SYS_libos_oe_load_module_host_file_system:
        case SYS_libos_oe_load_module_host_socket_interface:
        case SYS_libos_oe_load_module_host_resolver:
        case SYS_libos_oe_load_module_host_epoll:
        case SYS_libos_oe_sgx_set_minimum_crl_tcb_issue_date:
        case SYS_libos_oe_result_str:
        case SYS_libos_oe_get_enclave_status:
        case SYS_libos_oe_allocate_ocall_buffer:
        case SYS_libos_oe_free_ocall_buffer:
        case SYS_libos_oe_call_host_function:
        {
            _strace(n, "forwarded");
            BREAK(_return(n, _forward_syscall(n, params)));
        }
        default:
        {
            libos_panic("unknown syscall: %s(): %ld", syscall_str(n), n);
        }
    }

    libos_panic("unhandled syscall: %s()", syscall_str(n));

done:

    /* ---------- running target thread descriptor ---------- */

    /* the C-runtime must execute on its own thread descriptor */
    if (crt_td)
        libos_set_fsbase(crt_td);

    libos_times_leave_kernel(tp_enter);

    return syscall_ret;
}

/*
**==============================================================================
**
** syscalls
**
**==============================================================================
*/

static libos_spinlock_t _get_time_lock = LIBOS_SPINLOCK_INITIALIZER;
static libos_spinlock_t _set_time_lock = LIBOS_SPINLOCK_INITIALIZER;

long libos_syscall_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
    long params[6] = {(long)clk_id, (long)tp};
    libos_spin_lock(&_get_time_lock);
    long ret = libos_tcall(LIBOS_TCALL_CLOCK_GETTIME, params);
    libos_spin_unlock(&_get_time_lock);
    return ret;
}

long libos_syscall_clock_settime(clockid_t clk_id, struct timespec* tp)
{
    long params[6] = {(long)clk_id, (long)tp};
    libos_spin_lock(&_set_time_lock);
    long ret = libos_tcall(LIBOS_TCALL_CLOCK_SETTIME, params);
    libos_spin_unlock(&_set_time_lock);
    return ret;
}

long libos_syscall_gettimeofday(struct timeval* tv, struct timezone* tz)
{
    (void)tz;
    struct timespec tp = {0};
    if (tv == NULL)
        return 0;

    long ret = libos_syscall_clock_gettime(CLOCK_REALTIME, &tp);
    if (ret == 0)
    {
        tv->tv_sec = tp.tv_sec;
        tv->tv_usec = tp.tv_nsec / 1000;
    }
    return ret;
}

long libos_syscall_time(time_t* tloc)
{
    struct timespec tp = {0};
    long ret = libos_syscall_clock_gettime(CLOCK_REALTIME, &tp);
    if (ret == 0)
    {
        if (tloc != NULL)
            *tloc = tp.tv_sec;
        ret = tp.tv_sec;
    }
    return ret;
}

long libos_syscall_isatty(int fd)
{
    long params[6] = {(long)fd};
    return libos_tcall(LIBOS_TCALL_ISATTY, params);
}

long libos_syscall_add_symbol_file(
    const char* path,
    const void* text,
    size_t text_size)
{
    long ret = 0;
    void* file_data = NULL;
    size_t file_size;
    long params[6] = {0};

    ECHECK(libos_load_file(path, &file_data, &file_size));

    params[0] = (long)file_data;
    params[1] = (long)file_size;
    params[2] = (long)text;
    params[3] = (long)text_size;

    ECHECK(libos_tcall(LIBOS_TCALL_ADD_SYMBOL_FILE, params));

done:

    if (file_data)
        free(file_data);

    return ret;
}

long libos_syscall_load_symbols(void)
{
    long params[6] = {0};
    return libos_tcall(LIBOS_TCALL_LOAD_SYMBOLS, params);
}

long libos_syscall_unload_symbols(void)
{
    long params[6] = {0};
    return libos_tcall(LIBOS_TCALL_UNLOAD_SYMBOLS, params);
}
