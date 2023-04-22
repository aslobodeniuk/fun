/* Interface trampoline
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

typedef enum
{
  DEBUG,
  DEFAULT,
  SSE,
  AVX
} IFTR_backend;

typedef struct
{
  IFTR_backend enm;
  void *iface;
} ItrmpIfaceMap;

#define IFTR_IFACE_INSTANCE(type, backend) iftr_##type##_##backend
#define IFTR_IFACE_DEFINE(type, backend) type IFTR_IFACE_INSTANCE (type, backend)
#define IFTR_IFACE(type, ...) IFTR_IFACE_DEFINE (type, IFTR_BACKEND) = { __VA_ARGS__ }
#define IFTR_FUNCTION(func) .func = func
#define IFTR_IFACE_DECLARE(type, backend) extern IFTR_IFACE_DEFINE (type, backend)

#define IFTR_MAP_MEMBER(type, backend) {backend, & IFTR_IFACE_INSTANCE (type, backend) }

#define IFTR_IFACES_ARRAY(type) itrm_defined_ifaces_##type

#define IFTR_TRAMPOLINE_IFACE(type)                               \
  IFTR_IFACE_DECLARE (type, DEBUG);                               \
  IFTR_IFACE_DECLARE (type, DEFAULT);                             \
  IFTR_IFACE_DECLARE (type, SSE);                                 \
  IFTR_IFACE_DECLARE (type, AVX);                                 \
                                                                  \
  static ItrmpIfaceMap IFTR_IFACES_ARRAY (type)[] = {             \
    IFTR_MAP_MEMBER (type, DEBUG),                                \
    IFTR_MAP_MEMBER (type, DEFAULT),                              \
    IFTR_MAP_MEMBER (type, SSE),                                  \
    IFTR_MAP_MEMBER (type, AVX),                                  \
    {0, 0}                                                        \
  };                                                              \
                                                                  \
  static type *iftr_get_iface_##type (void)                       \
  {                                                               \
  static type *ret;                                               \
                                                                  \
  if (ret)                                                        \
    return ret;                                                   \
                                                                  \
  return (ret = iftr_get_iface (IFTR_IFACES_ARRAY (type)));       \
  }                                                               \


#define IFTR_GET_IFACE(type) iftr_get_iface_##type ()

void *iftr_get_iface (const ItrmpIfaceMap * ifaces);
