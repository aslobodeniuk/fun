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
#include <stdio.h>

IFTR_TRAMPOLINE_IFACE (MyInterface);

int
main ()
{
  float src[1024];
  float dst[1024];

  int i;
  MyInterface *iface;

  /* fill the data */
  for (i = 0; i < 1024; i++) {
    src[i] = (float)i / 256.0;
  }

  iface = IFTR_GET_IFACE (MyInterface);

  iface->my_memcpy (dst, src, 1024 * sizeof (float));

  for (i = 0; i < 1024; i++) {
    printf ("%f\n", dst[i]);
  }


  return 0;
}
