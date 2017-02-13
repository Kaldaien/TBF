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

#include "tls.h"

TBF_TLS*
__stdcall
TBF_GetTLS (void)
{
  if (__TBF_TLS_INDEX == -1)
    return nullptr;

  LPVOID lpvData =
    TlsGetValue (__TBF_TLS_INDEX);

  if (lpvData == nullptr)
  {
    lpvData =
      (LPVOID)LocalAlloc (LPTR, sizeof TBF_TLS);

    if (! TlsSetValue (__TBF_TLS_INDEX, lpvData))
    {
      LocalFree (lpvData);
      return nullptr;
    }
  }

  return (TBF_TLS *)lpvData;
}