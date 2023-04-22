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

static IFTR_backend
iftr_get_backend (void)
{
  /* TODO: set DEBUG on env var set */
  return iftr_select_backend ();
}

void *
iftr_get_iface (const ItrmpIfaceMap * ifaces)
{
  IFTR_backend backend;
  const char *backend_name;

  backend = iftr_get_backend ();

  while (ifaces->iface != 0) {

    if (ifaces->enm == backend)
      return ifaces->iface;

    ifaces++;
  }

  return 0;
}
