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

#include "iface-trampoline.h"

/* FIXME!!!! */
#define HAVE_AMD64 1
/* --------------- */

static int
iftr_get_cpuid (unsigned op, unsigned *a, unsigned *b, unsigned *c, unsigned *d)
{
#if defined(_MSC_VER)
  {
    int tmp[4];
    __cpuid (tmp, op);
    *a = tmp[0];
    *b = tmp[1];
    *c = tmp[2];
    *d = tmp[3];
    return 0;
  }
#elif defined(__GNUC__) || defined (__SUNPRO_C)
  {
    *a = op;
    *c = 0;
#if defined(HAVE_I386)
  __asm__ ("  pushl %%ebx\n" "  cpuid\n" "  mov %%ebx, %%esi\n" "  popl %%ebx\n":"+a" (*a), "=S" (*b), "+c" (*c),
        "=d"
        (*d));
#elif defined(HAVE_AMD64)
  __asm__ ("  cpuid\n":"+a" (*a), "=b" (*b), "+c" (*c), "=d" (*d));
#endif

    return 0;
  }
#else
  return -1;
#endif
}

static int
iftr_intel_has_sse (unsigned ecx, unsigned edx)
{
  /* SSE2 */
  if (edx & (1 << 26))
    return 1;

  /* SSE3 */
  if ((ecx & (1 << 0)) || (ecx & (1 << 9)))
    return 1;

  /* SSE4 */
  if ((ecx & (1 << 19)) || (ecx & (1 << 20)))
    return 1;

  /* No SSE supported? */
  return 0;
}

static int
iftr_intel_has_avx (void)
{
  /* Pending investigation */
  return 0;
}

static IFTR_backend
iftr_intel_select (unsigned level)
{
  unsigned eax, ebx, ecx, edx;

  if (level == 0)
    return DEFAULT;

  iftr_get_cpuid (1, &eax, &ebx, &ecx, &edx);

  if (iftr_intel_has_avx ()) {
    return AVX;
  }

  if (iftr_intel_has_sse (ecx, edx)) {
    return SSE;
  }

  return DEFAULT;
}


static IFTR_backend
iftr_amd_select (unsigned level)
{
  IFTR_backend std;
  unsigned ebx, ecx, edx;

  std = iftr_intel_select (level);
  if (DEFAULT != std)
    return std;

  iftr_get_cpuid (0x80000000, &level, &ebx, &ecx, &edx);

  if (level == 0)
    return DEFAULT;

  /* SSE4 */
  if (ecx & (1 << 6)) {
    return SSE;
  }
  /* SSE5 */
  if (ecx & (1 << 11)) {
    return SSE;
  }

  return DEFAULT;
}


IFTR_backend
iftr_select_backend (void)
{
  unsigned ebx, edx;
  unsigned level;
  unsigned vendor;
  int i;
  static IFTR_backend ret;

  if (ret)
    return ret;

  if (-1 == iftr_get_cpuid (0, &level, &ebx, &vendor, &edx) || level == 0) {
    return (ret = DEFAULT);
  }

  struct
  {
    union
    {
      char c[4];
      unsigned u;
    } u;

      IFTR_backend (*select) (unsigned);
  } known_vendors[] = {
    {{{'n', 't', 'e', 'l'}}, iftr_intel_select},
    {{{'c', 'A', 'M', 'D'}}, iftr_amd_select},
    {{{'u', 'i', 'n', 'e'}}, iftr_intel_select}
  };

  for (i = 0; i < (sizeof (known_vendors) / sizeof (int)); i++) {
    if (known_vendors[i].u.u == vendor) {
      return (ret = known_vendors[i].select (level));
    }
  }

  return (ret = DEFAULT);
}
