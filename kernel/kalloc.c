// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end,int i);
void kfree_init(void *pa,int i);
void *steal(int giver);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct spinlock steal_lock;

struct kmem kmems[NCPU];

void
kinit()
{
  uint64 range_num  = (PHYSTOP-(uint64)end)/PGSIZE;
  uint64 range_size = range_num/NCPU;
  char* kmem_name[] = {"kmem0","kmem1","kmem2","kmem3","kmem4","kmem5","kmem6","kmem7"};
  for(int i=0;i<NCPU;i++){
    initlock(&kmems[i].lock, kmem_name[i]);
    uint64 start_range = (uint64)end + i*range_size*PGSIZE;
    uint64 end_range = start_range + range_size*PGSIZE;
    freerange((void*)start_range,(void*)end_range,i);
  }
  // initlock(&kmem.lock, "kmem");
  // freerange(end, (void*)PHYSTOP);

}

// void
// freerange(void *pa_start, void *pa_end)
// {
//   char *p;
//   p = (char*)PGROUNDUP((uint64)pa_start);
//   for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
//     kfree(p);
// }

void
freerange(void *pa_start, void *pa_end, int i)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_init(p,i);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int cpu_id;

  push_off();
  cpu_id = cpuid();
  pop_off();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist;
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
}

void
kfree_init(void *pa,int i)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP){
    panic("kfree_init");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[i].lock);
  r->next = kmems[i].freelist;
  kmems[i].freelist = r;
  release(&kmems[i].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int cpu_id;

  push_off();
  cpu_id = cpuid();
  pop_off();

  acquire(&kmems[cpu_id].lock);
  r = kmems[cpu_id].freelist;
  if(r)
    kmems[cpu_id].freelist = r->next;
  release(&kmems[cpu_id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  else{
    //steal from others'
    for(int i=0;i<NCPU;i++){
      if(i == cpu_id)
        continue;
      acquire(&steal_lock);
      r = steal(i);
      release(&steal_lock);
      if(r)
        break;
    }
  
  }
  return (void*)r;
}

void *
steal(int giver){
  struct run *r_g;

  acquire(&kmems[giver].lock);
  r_g = kmems[giver].freelist;
  if(r_g)
    kmems[giver].freelist = r_g ->next;
  release(&kmems[giver].lock);
  if(r_g)
    memset((char*)r_g, 5, PGSIZE);
  return (void*) r_g;
}

// return the freemem for sysinfo
uint64
get_freemem(void)
{
  struct run *r;
  int free_mem = 0;
  int cpu_id;

  push_off();
  cpu_id = cpuid();
  pop_off();

  r = kmems[cpu_id].freelist;
  while(r){
    free_mem += PGSIZE;
    r = r->next;
  }
  return free_mem;
}
