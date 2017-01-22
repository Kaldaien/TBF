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

#define _CRT_SECURE_NO_WARNINGS

#include <string>

#include "hook.h"
#include "input.h"
#include "config.h"

MH_STATUS
WINAPI
TBF_CreateFuncHook ( LPCWSTR pwszFuncName,
                     LPVOID  pTarget,
                     LPVOID  pDetour,
                     LPVOID *ppOriginal )
{
  typedef MH_STATUS (WINAPI *SK_CreateFuncHook_pfn)
      ( LPCWSTR pwszFuncName, LPVOID  pTarget,
        LPVOID  pDetour,      LPVOID *ppOriginal );

  static SK_CreateFuncHook_pfn SK_CreateFuncHook =
    TBF_ImportFunctionFromSpecialK (
      "SK_CreateFuncHook",
        SK_CreateFuncHook
    );

  return
    SK_CreateFuncHook (pwszFuncName, pTarget, pDetour, ppOriginal);
}

MH_STATUS
WINAPI
TBF_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr )
{
  typedef MH_STATUS (WINAPI *SK_CreateDLLHook_pfn)(
        LPCWSTR pwszModule, LPCSTR  pszProcName,
        LPVOID  pDetour,    LPVOID *ppOriginal, 
        LPVOID *ppFuncAddr );

  static SK_CreateDLLHook_pfn SK_CreateDLLHook =
    TBF_ImportFunctionFromSpecialK (
      "SK_CreateDLLHook",
        SK_CreateDLLHook
    );

  return
    SK_CreateDLLHook (pwszModule,pszProcName,pDetour,ppOriginal,ppFuncAddr);
}

MH_STATUS
WINAPI
TBF_CreateDLLHook2 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                     LPVOID  pDetour,    LPVOID *ppOriginal,
                     LPVOID *ppFuncAddr )
{
  typedef MH_STATUS (WINAPI *SK_CreateDLLHook2_pfn)(
        LPCWSTR pwszModule, LPCSTR  pszProcName,
        LPVOID  pDetour,    LPVOID *ppOriginal, 
        LPVOID *ppFuncAddr );

  static SK_CreateDLLHook2_pfn SK_CreateDLLHook2 =
    TBF_ImportFunctionFromSpecialK (
      "SK_CreateDLLHook2",
        SK_CreateDLLHook2
    );

  return
    SK_CreateDLLHook2 (pwszModule,pszProcName,pDetour,ppOriginal,ppFuncAddr);
}

MH_STATUS
WINAPI
TBF_ApplyQueuedHooks (void)
{
  typedef MH_STATUS (WINAPI *SK_ApplyQueuedHooks_pfn)(void);

  static SK_ApplyQueuedHooks_pfn SK_ApplyQueuedHooks =
    TBF_ImportFunctionFromSpecialK (
      "SK_ApplyQueuedHooks",
        SK_ApplyQueuedHooks
    );

  return SK_ApplyQueuedHooks ();
}

MH_STATUS
WINAPI
TBF_EnableHook (LPVOID pTarget)
{
  typedef MH_STATUS (WINAPI *SK_EnableHook_pfn)(LPVOID pTarget);

  static SK_EnableHook_pfn SK_EnableHook =
    TBF_ImportFunctionFromSpecialK (
      "SK_EnableHook",
        SK_EnableHook
    );

  return SK_EnableHook (pTarget);
}

MH_STATUS
WINAPI
TBF_DisableHook (LPVOID pTarget)
{
  typedef MH_STATUS (WINAPI *SK_DisableHook_pfn)(LPVOID pTarget);

  static SK_DisableHook_pfn SK_DisableHook =
    TBF_ImportFunctionFromSpecialK (
      "SK_DisableHook",
        SK_DisableHook
    );

  return SK_DisableHook (pTarget);
}

MH_STATUS
WINAPI
TBF_RemoveHook (LPVOID pTarget)
{
  typedef MH_STATUS (WINAPI *SK_RemoveHook_pfn)(LPVOID pTarget);

  static SK_RemoveHook_pfn SK_RemoveHook =
    TBF_ImportFunctionFromSpecialK (
      "SK_RemoveHook",
        SK_RemoveHook
    );

  return SK_RemoveHook (pTarget);
}

MH_STATUS
WINAPI
TBF_Init_MinHook (void)
{
  MH_STATUS status = MH_OK;

  tbf::InputFix::Init ();

  return status;
}

MH_STATUS
WINAPI
TBF_UnInit_MinHook (void)
{
  MH_STATUS status = MH_OK;

  tbf::InputFix::Shutdown ();

  return status;
}