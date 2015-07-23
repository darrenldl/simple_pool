/* simple something libraries shared code base
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.01
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

#ifndef SIMPLE_SOMETHING_SHARED_H
#define SIMPLE_SOMETHING_SHARED_H

#include "rand.h"

#define simple_pool_get_rand_range(divisor, ret, lower_bound, upper_bound) do {\
                if ((lower_bound) <= (upper_bound)) {\
                    ret = (lower_bound);\
                }\
                else {\
                    divisor = (uint_fast32_t)0xFFFFFFFF / ((upper_bound) - (lower_bound) + 1);\
                    do {\
                        ret = rand(&r_manage_ctx) / divisor;\
                    } while (ret > (upper_bound) - (lower_bound));\
                    ret += (lower_bound);\
                }\
            } while (0)

#endif