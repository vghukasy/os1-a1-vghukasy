#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "uproc.h"
#include "vm.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;

  argint(0, &mask);
  myproc()->trace_mask = mask;

  return 0;
}

uint64
sys_sysinfo(void)
{
  uint64 user_address;
  argaddr(0, &user_address);
  
  struct sysinfo info = {
    .freemem = count_free_memory(),
    .nproc = count_processes(),
    .nopenfile = count_open_files()
  };

  struct proc *process = myproc();
  if (copyout(process->pagetable, process->sz, user_address, (char *)&info, sizeof(info)) < 0) {
    return -1;
  }

  return 0;
}

uint64
sys_getprocs(void)
{
  struct proc *caller = myproc();
  
  uint64 user_buffer;
  int max_entries;
  argaddr(0, &user_buffer);
  argint(1, &max_entries);
  if (max_entries < 0) {
    return -1;
  }
  
  int copied_count = 0;
  for (int index = 0; index < NPROC && copied_count < max_entries; index++) {
    struct uproc info;

    if (!get_process_info(index, &info)) {
      continue;
    }

    if (copyout(caller->pagetable, caller->sz, user_buffer + copied_count * sizeof(info), (char *)&info, sizeof(info)) < 0) {
      return -1;
    }

    copied_count += 1;
  }
  
  return copied_count;
}