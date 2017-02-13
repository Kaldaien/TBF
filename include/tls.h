/**
 * This file is part of Tales of Berseria "Fix".
 *
 * Tales of Berseria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Berseria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Berseria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __TBF__TLS_H__
#define __TBF__TLS_H__

#include <Windows.h>

struct TBF_TLS {
  struct {
    bool texinject_thread = false;
  } d3d9;
};

extern volatile DWORD __TBF_TLS_INDEX;

TBF_TLS* __stdcall TBF_GetTLS (void);

#endif /* __TBF__TLS_H__ */