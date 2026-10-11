#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if (addr >= p->sz ||
      addr + sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if (copyin(p->pagetable, p->sz, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if (copyinstr(p->pagetable, p->sz, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (not including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_sync(void);
extern uint64 sys_trace(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
  // clang-format off
  [SYS_fork]    = sys_fork,
  [SYS_exit]    = sys_exit,
  [SYS_wait]    = sys_wait,
  [SYS_pipe]    = sys_pipe,
  [SYS_read]    = sys_read,
  [SYS_kill]    = sys_kill,
  [SYS_exec]    = sys_exec,
  [SYS_fstat]   = sys_fstat,
  [SYS_chdir]   = sys_chdir,
  [SYS_dup]     = sys_dup,
  [SYS_getpid]  = sys_getpid,
  [SYS_sbrk]    = sys_sbrk,
  [SYS_pause]   = sys_pause,
  [SYS_uptime]  = sys_uptime,
  [SYS_open]    = sys_open,
  [SYS_write]   = sys_write,
  [SYS_mknod]   = sys_mknod,
  [SYS_unlink]  = sys_unlink,
  [SYS_link]    = sys_link,
  [SYS_mkdir]   = sys_mkdir,
  [SYS_close]   = sys_close,
  [SYS_sync]    = sys_sync,
  [SYS_trace]   = sys_trace,
  // clang-format on
};

enum arg_kind { ARG_INTEGER, ARG_STRING, ARG_NONE };

static enum arg_kind
first_arg_kind(int num)
{
  switch (num) {
    case SYS_open:
    case SYS_exec:
    case SYS_chdir:
    case SYS_mkdir:
    case SYS_unlink:
    case SYS_link:
    case SYS_mknod:
      return ARG_STRING;
    case SYS_fork:
    case SYS_getpid:
    case SYS_uptime:
    case SYS_sync:
      return ARG_NONE;
    default:
      return ARG_INTEGER;
  }
}

static char *
get_syscall_name(int num)
{
  switch (num) {
    case SYS_fork: return "fork";
    case SYS_exit: return "exit";
    case SYS_wait: return "wait";
    case SYS_pipe: return "pipe";
    case SYS_read: return "read";
    case SYS_kill: return "kill";
    case SYS_exec: return "exec";
    case SYS_fstat: return "fstat";
    case SYS_chdir: return "chdir";
    case SYS_dup: return "dup";
    case SYS_getpid: return "getpid";
    case SYS_sbrk: return "sbrk";
    case SYS_pause: return "pause";
    case SYS_uptime: return "uptime";
    case SYS_open: return "open";
    case SYS_write: return "write";
    case SYS_mknod: return "mknod";
    case SYS_unlink: return "unlink";
    case SYS_link: return "link";
    case SYS_mkdir: return "mkdir";
    case SYS_close: return "close";
    case SYS_sync: return "sync";
    case SYS_trace: return "trace";
  }

  return "unknown";
}

static void
print_trace_line(int num, uint64 first_arg, char *string_arg)
{
  struct proc *p = myproc();
  char *name = get_syscall_name(num);
  int return_value = (int)p->trapframe->a0;

  switch(first_arg_kind(num)){
  case ARG_STRING:
    if(string_arg)
      printk("[%d] %s(\"%s\") -> %d\n", p->pid, name, string_arg, return_value);
    else
      printk("[%d] %s(?) -> %d\n", p->pid, name, return_value);
    break;
  case ARG_NONE:
    printk("[%d] %s() -> %d\n", p->pid, name, return_value);
    break;
  case ARG_INTEGER:
    printk("[%d] %s(%d) -> %d\n", p->pid, name, (int)first_arg, return_value);
    break;
  }
}

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    uint64 first_arg = p->trapframe->a0;
    char string_buffer[MAXPATH];
    char *string_arg = 0;

    if ((p->trace_mask & (1 << num)) && first_arg_kind(num) == ARG_STRING && fetchstr(first_arg, string_buffer, sizeof(string_buffer)) >= 0) {
      string_arg = string_buffer;
    }

    // Use num to lookup the system call function for num, call it,
    // and store its return value in p->trapframe->a0
    p->trapframe->a0 = syscalls[num]();

    if (p->trace_mask & (1 << num)) {
      print_trace_line(num, first_arg, string_arg);
    }
  } else {
    printk("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
