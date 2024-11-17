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

#define NBUCKETS 13

struct {
  struct spinlock locks[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct buf buckets[NBUCKETS];  
} bcache;

void
binit(void)
{
  struct buf *b;
  
  for (int i = 0; i < NBUCKETS; i++) {
    initlock(&bcache.locks[i], "bcache");
    bcache.buckets[i].prev = &bcache.buckets[i];
    bcache.buckets[i].next = &bcache.buckets[i];
    for (b = bcache.buf + i * NBUF/NBUCKETS; b < bcache.buf + (i + 1) * NBUF/NBUCKETS; b++) {
      b->next = bcache.buckets[i].next;
      b->prev = &bcache.buckets[i];
      initsleeplock(&b->lock, "buffer");
      bcache.buckets[i].next->prev = b;
      bcache.buckets[i].next = b;
    }
  } 
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int hash = blockno % NBUCKETS;
  
  acquire(&bcache.locks[hash]);
  // Is the block already cached?
  for(b = bcache.buckets[hash].next; b != &bcache.buckets[hash]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.buckets[hash].prev; b != &bcache.buckets[hash]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.locks[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.locks[hash]);


  for (int i = 0; i < NBUCKETS; i++) {
    if (i == hash) continue;
    acquire(&bcache.locks[i]);
    for(b = bcache.buckets[i].prev; b != &bcache.buckets[i]; b = b->prev) {
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.locks[i]);
        acquire(&bcache.locks[hash]);
        b->prev = &bcache.buckets[hash];
        b->next = bcache.buckets[hash].next;
        bcache.buckets[hash].next->prev = b;
        bcache.buckets[hash].next = b;
        release(&bcache.locks[hash]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.locks[i]);
  }

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
  
  int hash = b->blockno % NBUCKETS;
  acquire(&bcache.locks[hash]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.buckets[hash].next;
    b->prev = &bcache.buckets[hash];
    bcache.buckets[hash].next->prev = b;
    bcache.buckets[hash].next = b;
  }
  release(&bcache.locks[hash]);
}

void
bpin(struct buf *b) {
  int hash = b->blockno % NBUCKETS;
  acquire(&bcache.locks[hash]);
  b->refcnt++;
  release(&bcache.locks[hash]);
}

void
bunpin(struct buf *b) {
  int hash = b->blockno % NBUCKETS;
  acquire(&bcache.locks[hash]);
  b->refcnt--;
  release(&bcache.locks[hash]);
}