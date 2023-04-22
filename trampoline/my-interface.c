/* Funny code of using a trampoline
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "my-interface.h"
#include "iftr/iface-trampoline.h"

/* 
TODO

typedef char achr __attribute__ ((__aligned__(16)));
 */

static void
my_memcpy (void *restrict dst, const void *restrict src, int bytes)
{
  while (bytes--)
    *((char*)dst++) = *((char*)src++);
}

static void
my_float_multiply (float *restrict dst, const float *restrict src, int floats)
{
  while (floats--)
    *(dst++) *= *(src++);
}

IFTR_IFACE (MyInterface,
    IFTR_FUNCTION (my_memcpy),
    IFTR_FUNCTION (my_float_multiply)
);
