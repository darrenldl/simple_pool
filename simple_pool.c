/* simple pool allocator
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.01
 * 
 * Note:
 *    simple pool is thread safe
 * 
 * License:
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 */

#include "simple_pool.h"

int pool_init (simple_pool** p_pool, uint_fast32_t obj_size) {
   uint_fast32_t alloc_size;
   
   uint_fast32_t alloc_size;
   
   simple_pool* pool;
   
   simple_puddle* pud_temp;
   simple_puddle* pud_cur;
   
   simple_puddle** p_pud_cur;
   
   unsigned char uchar_temp;
   unsigned char uchar_cur;
   
   int ret_val;
   
   // input check
   if (p_pool == NULL) {
      printf("pool_init : p_pool is NULL\n");
      return WRONG_INPUT;
   }
   if (obj_size == 0) {
      printf("pool_init : obj_size is 0\n");
      return WRONG_INPUT;
   }
   
   *p_pool = (simple_pool*) malloc(sizeof(simple_pool));
   if (*p_pool == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   pool = *p_pool;
   
   pool->obj_size = obj_size;
   
   pool->meta_data_slots = INIT_META_DATA_SLOTS;
   
   pool->raw_full_map = (map_block*) malloc(get_bitmap_map_block_number(INIT_META_DATA_SLOTS) * sizeof(map_block));
   if (pool->raw_full_map == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   
   // not using up all space as bitmap right now
   bitmap_init(&(pool->puddle_full_map), pool->raw_full_map, NULL, INIT_PUDDLE_NUMBER, 0);
   
   pool->full_map_mutex = PTHREAD_MUTEX_INITIALIZER;
   
   pool->raw_lock_map = (map_block*) malloc(get_bitmap_map_block_number(INIT_META_DATA_SLOTS) * sizeof(map_block));
   if (pool->raw_lock_map == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   
   // not using up all space as bitmap right now
   bitmap_init(&(pool->puddle_lock_map), pool->raw_lock_map_map, NULL, INIT_PUDDLE_NUMBER, 0);
   
   pool->lock_map_mutex = PTHREAD_MUTEX_INITIALIZER;
   
   pool->base = (simple_puddle**) malloc(INIT_META_DATA_SLOTS * sizeof(simple_puddle*));
   if (pool->base == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   
   pool->cur = pool->base;
   pool->end = pool->base + INIT_META_DATA_SLOTS - 1;
   
   pud_temp = (simple_puddle*) malloc(INIT_PUDDLE_NUMBER * sizeof(simple_puddle));
   if (pud_temp == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   
   uchar_temp = (unsigned char*) malloc(INIT_PUDDLE_NUMBER * PUDDLE_SIZE * obj_size);
   if (uchar_temp == NULL) {
      printf("pool_init : failed to allocate memory\n");
      return MEM_ALLOC_FAIL;
   }
   
   // setup pointer to puddle array and the puddles
   for (p_pud_cur = pool->base, pud_cur = pud_temp, uchar_cur = uchar_temp;
         pud_cur < pud_temp + INIT_PUDDLE_NUMBER;        // use pointer to array of simple puddles as counter
         p_pud_cur++, pud_cur++, uchar_cur += PUDDLE_SIZE * obj_size)
      {
      *p_pud_cur = pud_cur;
      puddle_init(pud_cur, obj_size, uchar_cur);
   }
   
   // setup random number
   randinit(&(pool->pool_random));
   
   return 0;
}

static
int puddle_init (simple_puddle* puddle, uint_fast32_t obj_size, unsigned char* chunk) {
   uint_fast32_t alloc_size;
   
   unsigned char* temp;
   
   // input check
   if (puddle == NULL) {
      printf("puddle_init : p_puddle is NULL\n");
      return WRONG_INPUT;
   }
   if (obj_size == 0) {
      printf("puddle_init : obj_size is 0\n");
      return WRONG_INPUT;
   }
   if (chunk == NULL) {
      printf("puddle_init : chunk is NULL\n");
      return WRONG_INPUT;
   }
   
   bitmap_init(&(puddle->map), puddle->raw_map, NULL, PUDDLE_SIZE, 0);
   
   puddle->base = chunk;
   puddle->end = chunk + PUDDLE_SIZE * obj_size - 1;
   
   return 0;
}

// pool_grow is not thread safe, thread safety is enforced by functions calling it
static
int pool_grow (simple_pool* pool) {
   unsigned char* old_ptr;
   unsigned char* new_ptr;
   
   uint_fast32_t alloc_size;
   
   uint_fast32_t puddles_to_add;
   uint_fast32_t puddles_free;
   uint_fast32_t puddles_new_total;
   
   uint_fast32_t meta_data_slots_to_add;
   uint_fast32_t meta_data_slots_free;
   uint_fast32_t meta_data_slots_new_total;
   
   uint_fast32_t old_num;
   uint_fast32_t new_num;
   
   simple_bitmap old_bitmap;
   
   simple_puddle** p_pud_temp;
   simple_puddle** p_pud_cur;
   
   simple_puddle** old_base;
   simple_puddle** old_cur;
   simple_puddle** old_end;
   
   simple_puddle* pud_temp;
   simple_puddle* pud_cur;
   
   map_block* map_temp;
   
   unsigned char* uchar_temp;
   unsigned char* uchar_cur;
   
   // input check
   if (pool == NULL) {
      printf("pool_grow : p_pool is NULL\n");
      return WRONG_INPUT;
   }
   
   // calculate required increment in puddles
   puddles_to_add = 0;
   puddles_free = pool->puddle_full_map.number_of_zeros;
   while (puddles_to_add + puddles_free < MIN_RANDOM_OPTIONS) {
      puddles_to_add += GROW_PUDDLE_NUMBER;
   }
   puddles_new_total = puddles_to_add + pool->puddle_full_map.length;
   
   // calculate required increment in meta data slots
   meta_data_slots_to_add = 0;
   meta_data_slots_free = pool->meta_data_slots - pool->puddle_full_map.length;
   while (meta_data_slots_to_add + meta_data_slots_free < puddles_to_add) {
      meta_data_slots_to_add += GROW_META_DATA_SLOTS;
   }
   meta_data_slots_new_total = meta_data_slots_to_add + pool->meta_data_slots;
   
   if (meta_data_slots_to_add) {
      // expand pointer array
      old_base = pool->base;
      old_cur = pool->cur;
      old_end = pool->end;
      alloc_size = meta_data_slots_new_total * sizeof(simple_puddle*);
      p_pud_temp = (simple_puddle**) realloc(pool->base, alloc_size);
      if (p_pud_temp == NULL) {
         printf("pool_grow : failed to expand pointer array\n");
         return MEM_ALLOC_FAIL;
      }
      pool->base = p_pud_temp;
      pool->cur = pool->base + (old_cur - old_base);
      pool->end = pool->base + meta_data_slots_new_total - 1;
      
      // expand full map
      bitmap_meta_copy(pool->puddle_full_map, &old_bitmap);
      alloc_size = meta_data_slots_new_total * sizeof(map_block);
      map_temp = (map_block*) realloc(pool->raw_full_map, alloc_size);
      if (map_temp == NULL) {
         printf("pool_grow : failed to expand full map\n");
         return MEM_ALLOC_FAIL;
      }
      pool->raw_full_map = map_temp;
      bitmap_init(&(pool->puddle_full_map), pool->raw_full_map, NULL, old_bitmap.length, 2);
      bitmap_grow(&(pool->puddle_full_map), NULL, meta_data_slots_new_total, 0);
      
      // expand lock map
      bitmap_meta_copy(pool->puddle_lock_map, &old_bitmap);
      //alloc_size = meta_data_slots_new_total * sizeof(map_block);
      map_temp = (map_block*) realloc(pool->raw_lock_map, alloc_size);
      if (map_temp == NULL) {
         printf("pool_grow : failed to expand lock map\n");
         return MEM_ALLOC_FAIL;
      }
      pool->raw_lock_map = map_temp;
      bitmap_init(&(pool->puddle_lock_map), pool->raw_lock_map, NULL, old_bitmap.length, 2);
      bitmap_grow(&(pool->puddle_lock_map), NULL, meta_data_slots_new_total, 0);
   }
   
   // get space for new puddles
   alloc_size = puddles_new_total * sizeof(simple_puddle);
   pud_temp = (simple_puddle*) malloc(alloc_size);
   if (pud_temp == NULL) {
      printf("pool_grow : failed to expand pointer to puddle array\n");
      return MEM_ALLOC_FAIL;
   }
   
   alloc_size = puddles_to_add * PUDDLE_SIZE * pool->obj_size;
   uchar_temp = (unsigned char*) malloc(alloc_size);
   if (uchar_temp == NULL) {
      printf("pool_grow : failed to allocate memory for new puddles\n");
      return MEM_ALLOC_FAIL;
   }
   
   // setup pointer to puddle array and the puddles
   for (p_pud_cur = pool->cur + 1, pud_cur = pud_temp, uchar_cur = uchar_temp;
         pud_cur < pud_temp + puddles_to_add;
         p_pud_cur++, pud_cur++, uchar_cur += PUDDLE_SIZE * obj_size)
      {
      *p_pud_cur = pud_cur;
      puddle_init(pud_cur, obj_size, uchar_cur);
   }
   
   pool->cur += puddles_to_add;
   
   return 0;
}

int pool_alloc (simple_pool* pool, void* ptr) {
   uint32_t rand_uint;
   
   unsigned char ;
   
   int ret_val;
   
   simple_puddle* pud;
   
   uint_fast32_t temp;
   
   map_block bit_buf;
   
   bit_index ret_index;
   
   // input check
   if (pool == NULL) {
      printf("pool_alloc : pool is NULL\n");
      return WRONG_INPUT;
   }
   
   // lock full map
   if ((ret_val = pthread_mutex_lock(&(pool->full_map_mutex)))) {
      printf("pool_alloc : failed to lock full map\n");
      return ret_val;
   }
   
   // check if need to grow
   if (pool->puddle_full_map.number_of_zeros < MIN_RANDOM_OPTIONS) {
      // lock lock map
      if ((ret_val = pthread_mutex_lock(&(pool->lock_map_mutex)))) {
         printf("pool_alloc : failed to lock lock map\n");
         return ret_val;
      }
      
      pool_grow(pool);
      
      // unlock lock map
      if ((ret_val = pthread_mutex_unlock(&(pool->lock_map_mutex)))) {
         printf("pool_alloc : failed to unlock lock map\n");
         return ret_val;
      }
   }
   
   // critical section starts
   
   // critical section ends
   
   return 0;
}

int pool_free () {
}