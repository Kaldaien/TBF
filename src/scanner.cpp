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

#include "scanner.h"
#include <log.h>

#include <Windows.h>

static LPVOID __SK_base_img_addr = nullptr;
static LPVOID __SK_end_img_addr  = nullptr;

uintptr_t
TBF_GetBaseAddr (void)
{
  return (uintptr_t)__SK_base_img_addr;
}

void*
TBF_Scan (uint8_t* pattern, size_t len, uint8_t* mask, int align)
{
  uint8_t* base_addr = (uint8_t *)GetModuleHandle (nullptr);

  MEMORY_BASIC_INFORMATION mem_info;
  VirtualQuery (base_addr, &mem_info, sizeof mem_info);

  //
  // VMProtect kills this, so let's do something else...
  //
#ifdef VMPROTECT_IS_NOT_A_FACTOR
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)mem_info.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((uintptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = (uint8_t *)mem_info.BaseAddress;//AllocationBase;
  uint8_t* end_addr  = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

  if (base_addr != (uint8_t *)0x400000) {
    static bool warned = false;
    if (! warned) {
      dll_log->Log ( L"[ Sig Scan ] Expected module base addr. 40000h, but got: %ph",
                      base_addr );
      warned = true;
    }
  }

  size_t pages = 0;

// Scan up to 256 MiB worth of data
#ifdef __WIN32
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 26));
#else
  // Dark Souls 3 needs this, its address space is completely random to the point
  //   where it may be occupying a range well in excess of 36 bits. Probably a stupid
  //     anti-cheat attempt.
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 36));
#endif

  //
  // For practical purposes, let's just assume that all valid games have less than 256 MiB of
  //   committed executable image data.
  //
  while (VirtualQuery (end_addr, &mem_info, sizeof mem_info) && end_addr < PAGE_WALK_LIMIT) {
    //if (mem_info.Protect & PAGE_NOACCESS || (! (mem_info.Type & MEM_IMAGE)))
      //break;

    pages += VirtualQuery (end_addr, &mem_info, sizeof mem_info);

    end_addr = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;
  } 

  if (end_addr > PAGE_WALK_LIMIT) {
    static bool warned = false;

    if (! warned) {
      dll_log->Log ( L"[ Sig Scan ] Module page walk resulted in end addr. out-of-range: %ph",
                      end_addr );
      dll_log->Log ( L"[ Sig Scan ]  >> Restricting to %ph",
                      PAGE_WALK_LIMIT );
      warned = true;
    }

    end_addr = (uint8_t *)PAGE_WALK_LIMIT;
  }

#if 0
  dll_log->Log ( L"[ Sig Scan ] Module image consists of %zu pages, from %ph to %ph",
                  pages,
                    base_addr,
                      end_addr );
#endif
#endif

  __SK_base_img_addr = base_addr;
  __SK_end_img_addr  = end_addr;

  uint8_t*  begin = (uint8_t *)base_addr;
  uint8_t*  it    = begin;
  int       idx   = 0;

  while (it < end_addr)
  {
    VirtualQuery (it, &mem_info, sizeof mem_info);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (mem_info.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

    if ( (! (mem_info.Type    & MEM_IMAGE))  ||
         (! (mem_info.State   & MEM_COMMIT)) ||
             mem_info.Protect & PAGE_NOACCESS ) {
      it    = next_rgn;
      idx   = 0;
      begin = it;
      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it < next_rgn) {
      uint8_t* scan_addr = it;

      bool match = (*scan_addr == pattern [idx]);

      // For portions we do not care about... treat them
      //   as matching.
      if (mask != nullptr && (! mask [idx]))
        match = true;

      if (match)
      {
        if (++idx == len)
        {
          if (((uintptr_t)begin % align) == 0)
            return (void *)begin;
          else
          {
            begin += (idx + 1);
            begin += align - ((uintptr_t)begin % align);

            it     = begin;
            idx    = 0;
          }
        }

        else
          ++it;
      }

      else {
        // No match?!
        if (it > end_addr - len)
          break;

        begin += (idx + 1);
        begin += align - ((uintptr_t)begin % align);

        it  = begin;
        idx = 0;
      }
    }
  }

  return nullptr;
}

BOOL
SK_InjectMemory ( LPVOID   base_addr,
                  uint8_t* new_data,
                  size_t   data_size,
                  DWORD    permissions,
                  uint8_t* old_data = nullptr
                )
{
  DWORD dwOld;

  if (VirtualProtect (base_addr, data_size, permissions, &dwOld))
  {
    if (old_data != nullptr)
      memcpy (old_data, base_addr, data_size);

    memcpy (base_addr, new_data, data_size);

    VirtualProtect (base_addr, data_size, dwOld, &dwOld);

    return TRUE;
  }

  return FALSE;
}

void*
TBF_ScanEx (uint8_t* pattern, size_t len, uint8_t* mask, void* after, int align)
{
  uint8_t* base_addr = (uint8_t *)GetModuleHandle (nullptr);

  MEMORY_BASIC_INFORMATION mem_info;
  VirtualQuery (base_addr, &mem_info, sizeof mem_info);

  //
  // VMProtect kills this, so let's do something else...
  //
#ifdef VMPROTECT_IS_NOT_A_FACTOR
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)mem_info.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((uintptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = (uint8_t *)mem_info.BaseAddress;//AllocationBase;
  uint8_t* end_addr  = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

  size_t pages = 0;

// Scan up to 256 MiB worth of data
#ifdef __WIN32
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 26));
#else
  // Dark Souls 3 needs this, its address space is completely random to the point
  //   where it may be occupying a range well in excess of 36 bits. Probably a stupid
  //     anti-cheat attempt.
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 36));
#endif

  //
  // For practical purposes, let's just assume that all valid games have less than 256 MiB of
  //   committed executable image data.
  //
  while (VirtualQuery (end_addr, &mem_info, sizeof mem_info) && end_addr < PAGE_WALK_LIMIT) {
    //if (mem_info.Protect & PAGE_NOACCESS || (! (mem_info.Type & MEM_IMAGE)))
      //break;

    pages += VirtualQuery (end_addr, &mem_info, sizeof mem_info);

    end_addr = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;
  } 

  if (end_addr > PAGE_WALK_LIMIT) {
    end_addr = (uint8_t *)PAGE_WALK_LIMIT;
  }
#endif

  uint8_t*  begin = (uint8_t *)after + align;
  uint8_t*  it    = begin;
  int       idx   = 0;

  while (it < end_addr)
  {
    VirtualQuery (it, &mem_info, sizeof mem_info);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (mem_info.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

    if ( (! (mem_info.Type    & MEM_IMAGE))  ||
         (! (mem_info.State   & MEM_COMMIT)) ||
             mem_info.Protect & PAGE_NOACCESS ) {
      it    = next_rgn;
      idx   = 0;
      begin = it;
      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it < next_rgn)
    {
      uint8_t* scan_addr = it;

      bool match = (*scan_addr == pattern [idx]);

      // For portions we do not care about... treat them
      //   as matching.
      if (mask != nullptr && (! mask [idx]))
        match = true;

      if (match)
      {
        if (++idx == len)
        {
          if (((uintptr_t)begin % align) == 0)
            return (void *)begin;
          else
          {
            begin += (idx + 1);
            begin += align - ((uintptr_t)begin % align);

            it     = begin;
            idx    = 0;
          }
        }

        else
          ++it;
      }

      else
      {
        // No match?!
        if (it > end_addr - len)
          break;

        begin += (idx+1);
        begin += align - ((uintptr_t)begin % align);

        it  = begin;
        idx = 0;
      }
    }
  }

  return nullptr;
}