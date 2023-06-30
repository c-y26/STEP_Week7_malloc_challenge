//
// >>>> malloc challenge! <<<<
//
// Your task is to improve utilization and speed of the following malloc
// implementation.
// Initial implementation is the same as the one implemented in simple_malloc.c.
// For the detailed explanation, please refer to simple_malloc.c.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// free list binにおけるbinの数
#define FREE_LIST_BIN 14

//
// Interfaces to get memory pages from OS
//

void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

//
// Struct definitions
//

typedef struct my_metadata_t {
  size_t size;
  struct my_metadata_t *next;
} my_metadata_t;

typedef struct my_heap_t {
  my_metadata_t *free_head;
  my_metadata_t dummy;
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
my_heap_t my_heap[FREE_LIST_BIN];

//
// Helper functions (feel free to add/remove/edit!)
//

/*
void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next);
  metadata->next = my_heap.free_head;
  my_heap.free_head = metadata;
}
*/

int calculate_index(size_t size) {
  int i;
  if (size < 1000) {
    i = size / 100;
  } else {
    i = (size - 1000) / 1000;
    i = i + 10;
  }
  return i;
  /*
  int i;
  i = size / 256;
  return i;
  */
}

void my_add_memory_to_free_list_bin(my_metadata_t *metadata, size_t size) {
  assert(!metadata->next);
  int index = calculate_index(size);
  metadata->next = my_heap[index].free_head;
  my_heap[index].free_head = metadata;
}

void my_add_to_free_list_bin(my_metadata_t *metadata) {
  assert(!metadata->next);
  int index = calculate_index(metadata->size);
  metadata->next = my_heap[index].free_head;
  my_heap[index].free_head = metadata;
}
/*
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  if (prev) {
    prev->next = metadata->next;
  } else {
    my_heap.free_head = metadata->next;
  }
  metadata->next = NULL;
}
*/

void my_remove_from_free_list_bin(my_metadata_t *metadata,
                                  my_metadata_t *prev) {
  if (prev) {
    prev->next = metadata->next;
  } else {
    int index = calculate_index(metadata->size);
    my_heap[index].free_head = metadata->next;
  }
  metadata->next = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

// This is called at the beginning of each challenge.
void my_initialize() {
  // oroginally
  // my_heap.free_head = &my_heap.dummy;
  // my_heap.dummy.size = 0;
  // my_heap.dummy.next = NULL;

  // free list bin
  for (int i = 0; i < FREE_LIST_BIN; i++) {
    my_heap[i].free_head = &my_heap[i].dummy;
    my_heap[i].dummy.size = 0;
    my_heap[i].dummy.next = NULL;
  }
}

// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().
void *my_malloc(size_t size) {
  // free list bin
  int index = calculate_index(size);
  my_metadata_t *metadata = my_heap[index].free_head;
  my_metadata_t *prev = NULL;
  // originally
  /*
  my_metadata_t *metadata = my_heap.free_head;
  my_metadata_t *prev = NULL;
  */
  // First-fit: Find the first free slot the object fits.
  // TODO: Update this logic to Best-fit!

  // 全てのbinで空き領域を探す
  /*
  while (1) {
    while (metadata && metadata->size < size) {
      prev = metadata;
      metadata = metadata->next;
    }
    if (!metadata && index + 1 < FREE_LIST_BIN) {
      index += 1;
      metadata = my_heap[index].free_head;
      prev = NULL;
    } else {
      break;
    }
  }
  */

  while (metadata && metadata->size < size) {
    prev = metadata;
    metadata = metadata->next;
  }

  //  now, metadata points to the first free slot
  //  and prev is the previous entry.

  // それ以降で Best-fit なスロットを探す
  my_metadata_t *min_size_slot = metadata;
  my_metadata_t *min_size_slot_prev = prev;

  while (metadata) {
    if ((min_size_slot->size > metadata->size) && (metadata->size >= size)) {
      min_size_slot_prev = prev;
      min_size_slot = metadata;
    }
    prev = metadata;
    metadata = metadata->next;
  }

  if (metadata) {
    prev = min_size_slot;
    metadata = min_size_slot->next;
  } else {
    prev = min_size_slot_prev;
    metadata = min_size_slot;
  }

  if (!metadata) {
    //  There was no free slot available. We need to request a new memory region
    //  from the system by calling mmap_from_system().
    //
    //      | metadata | free slot |
    //      ^
    //      metadata
    //      <---------------------->
    //             buffer_size
    size_t buffer_size = 4096;
    my_metadata_t *metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(my_metadata_t);
    metadata->next = NULL;
    // Add the memory region to the free list.
    // my_add_to_free_list(metadata);
    // my_add_to_free_list_bin(metadata);
    my_add_memory_to_free_list_bin(metadata, size);
    // Now, try my_malloc() again. This should succeed.
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  metadata->size = size;
  // Remove the free slot from the free list.
  // my_remove_from_free_list(metadata, prev);
  my_remove_from_free_list_bin(metadata, prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size;
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    // my_add_to_free_list(new_metadata);
    // my_add_to_free_list_bin(new_metadata);
    my_add_memory_to_free_list_bin(new_metadata, size);
  }
  return ptr;
}

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  // Add the free slot to the free list.
  // my_add_to_free_list(metadata);
  my_add_to_free_list_bin(metadata);
}

// This is called at the end of each challenge.
void my_finalize() {
  // Nothing is here for now.
  // feel free to add something if you want!
  // printf("%lu\n", sizeof(my_metadata_t));
}

void test() {
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}
