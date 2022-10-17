// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#include "proc.h"

#define NBUCKETS 13

struct bcache{
  struct spinlock lock;
  struct buf buf[NBUF/NBUCKETS+1];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
};
struct bcache bcaches[NBUCKETS];
struct spinlock find_lock;

void
binit(void)
{
  struct buf *b;

  char * bcache_name[] = {"bcache0","bcache1","bcache2","bcache3","bcache4","bcache5","bcache6","bcache7","bcache8","bcache9","bcache10","bcache11","bchache12"}; 
  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcaches[i].lock, bcache_name[i]);
    // Create linked list of buffers
    bcaches[i].head.prev = &bcaches[i].head;
    bcaches[i].head.next = &bcaches[i].head;
    for(b = bcaches[i].buf; b < bcaches[i].buf+NBUF/NBUCKETS+1; b++){
      b->next = bcaches[i].head.next;
      b->prev = &bcaches[i].head;
      initsleeplock(&b->lock, bcache_name[i]);
      bcaches[i].head.next->prev = b;
      bcaches[i].head.next = b;
      b->last_use = 0;
      b->hash = i;
    }
  }



  initlock(&find_lock,"find");
  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  acquire(&bcaches[blockno % NBUCKETS].lock);

  // Is the block already cached?
  for(b = bcaches[blockno % NBUCKETS].head.next; b != &bcaches[blockno % NBUCKETS].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->last_use = ticks;
      release(&bcaches[blockno % NBUCKETS].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcaches[blockno % NBUCKETS].head.prev; b != &bcaches[blockno % NBUCKETS].head; b = b->prev){
    if(b->refcnt <= 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      b->last_use = ticks;
      b->hash = blockno % NBUCKETS;
      release(&bcaches[blockno % NBUCKETS].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  //No more cache, try to steal one from other hashlist
  release(&bcaches[blockno % NBUCKETS].lock);
  int find = 0;


  for(int i=NBUCKETS-1;i>=0;i--){
    if(i == blockno % NBUCKETS)
      continue;
    acquire(&bcaches[i].lock);
    for(b = bcaches[i].head.prev;b!=&bcaches[i].head;b = b->prev){
      if(b -> refcnt <= 0) {
        //take out
        b->prev->next = b ->next;
        b->next->prev = b ->prev;
        find = 1;
        break;
      }
    }
    release(&bcaches[i].lock);
    if(find==1)
      break;    
  }

  if(find==1){
    //add the find one to hashlist
    acquire(&bcaches[blockno % NBUCKETS].lock);
    b->next = bcaches[blockno % NBUCKETS].head.next;
    b->prev = &bcaches[blockno % NBUCKETS].head;
    bcaches[blockno % NBUCKETS].head.next = b;
    b->next->prev = b;
    b->hash = blockno % NBUCKETS;

    //init b
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->last_use = ticks;
    release(&bcaches[blockno % NBUCKETS].lock);
    acquiresleep(&b->lock);
    return b;
  }

  // //find the least recently used (LRU) used buffer
  // int hash_i,lru=b->last_use;
  // struct buf *lru_b;
  // for(int i=0;i<NBUCKETS;i++){
  //   acquire(&bcaches[i].lock);
  //   for(b = bcaches[i].head.prev;b!=&bcaches[i].head;b = b->prev){
  //     if(lru < b->last_use){
  //       lru_b = b;
  //       lru = b->last_use;
  //       hash_i = i;
  //     }
  //   }
  //   release(&bcaches[i].lock);
  // }

  // b = lru_b;
  // acquire(&bcaches[hash_i].lock);
  // //take out
  // b->prev->next = b ->next;
  // b->next->prev = b ->prev;
  // release(&bcaches[hash_i].lock);

  
  // //add the find one to hashlist
  // acquire(&bcaches[pid % NBUCKETS].lock);
  // b->next = bcaches[pid % NBUCKETS].head.next;
  // b->prev = &bcaches[pid % NBUCKETS].head;
  // bcaches[pid % NBUCKETS].head.next = b;
  // b->next->prev = b;
  // //init b
  // b->dev = dev;
  // b->blockno = blockno;
  // b->valid = 0;
  // b->refcnt = 1;
  // b->last_use = ticks;
  // release(&bcaches[pid % NBUCKETS].lock);
  // acquiresleep(&b->lock);
  // return b;

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcaches[b->hash].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcaches[b->hash].head.next;
    b->prev = &bcaches[b->hash].head;
    bcaches[b->hash].head.next->prev = b;
    bcaches[b->hash].head.next = b;
  }
  
  release(&bcaches[b->hash].lock);
}


void
bpin(struct buf *b) {
  acquire(&bcaches[b->hash].lock);
  b->refcnt++;
  release(&bcaches[b->hash].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcaches[b->hash].lock);
  b->refcnt--;
  release(&bcaches[b->hash].lock);
}


