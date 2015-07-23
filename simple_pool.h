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

#ifndef SIMPLE_POOL_H
#define SIMPLE_POOL_H

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "simple_bitmap.h"

#include "simple_something_error.h"
#include "simple_something_shared.h"

#define INIT_META_DATA_SLOTS     10000
#define GROW_META_DATA_SLOTS     1000

#define INIT_PUDDLE_NUMBER    500
#define GROW_PUDDLE_NUMBER    100

#define PUDDLE_SIZE     100

#define MIN_RANDOM_OPTIONS    5

#if INIT_META_DATA_SLOTS < INIT_PUDDLE_NUMBER
   #error "simple_pool.h : INIT_META_DATA_SLOTS is smaller than INIT_PUDDLE_NUMBER"
#endif

typedef struct simple_pool simple_pool;
typedef struct simple_puddle simple_puddle;

struct simple_pool {
   uint_fast16_t obj_size;
   uint_fast32_t meta_data_slots;
   
   map_block* raw_full_map;
   pthread_mutex_t full_map_mutex;
   simple_bitmap puddle_full_map;
   
   map_block* raw_lock_map;
   pthread_mutex_t lock_map_mutex;
   simple_bitmap puddle_lock_map;
   
   /* puddle array */
   simple_puddle** base;
   simple_puddle** cur;
   simple_puddle** end;
   
   randctx pool_random;
};

struct simple_puddle {
   map_block raw_map[get_bitmap_map_block_number(PUDDLE_SIZE)];
   
   simple_bitmap map;
   
   unsigned char* base;
   unsigned char* end;
};

int pool_init (simple_pool** p_pool, uint_fast32_t obj_size);
int puddle_init (simple_puddle** p_puddle, uint_fast32_t obj_size);

int pool_grow(simple_pool* pool);

#endif