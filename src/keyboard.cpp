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

#include "keyboard.h"
#include <windows.h>

#include "hook.h"
#include "config.h"

extern "C" {
typedef int (__cdecl *SDL_GetKeyFromScancode_pfn)(int scancode);
SDL_GetKeyFromScancode_pfn SDL_GetKeyFromScancode_Original = nullptr;

typedef uint8_t* (__cdecl *SDL_GetKeyboardState_pfn)(int* numkeys);
SDL_GetKeyboardState_pfn SDL_GetKeyboardState_Original = nullptr;

const uint8_t*
__cdecl
SDL_GetKeyboardState_Detour (int *numkeys)
{
  int num_keys;

  uint8_t* out = SDL_GetKeyboardState_Original (&num_keys);

  if (numkeys != nullptr)
    *numkeys = num_keys;

  static uint8_t keys [512];
  memcpy (keys, out, sizeof (uint8_t) * num_keys);

  std::vector <std::pair <uint16_t, uint16_t>>::iterator it =
    tbf::KeyboardFix::swapped_keys.begin ();

  while (it != tbf::KeyboardFix::swapped_keys.end ()) {
    keys [it->first]  = out [it->second];
    keys [it->second] = out [it->first];
    ++it;
  }

  return keys;
}

int
__cdecl
SDL_GetKeyFromScancode_Detour (int scancode)
{
  if (tbf::KeyboardFix::remapped_scancodes.find (scancode) ==
      tbf::KeyboardFix::remapped_scancodes.end ()) {
    return SDL_GetKeyFromScancode_Original (scancode);
  } else {
    std::vector <std::pair <uint16_t, uint16_t>>::iterator it =
      tbf::KeyboardFix::swapped_keys.begin ();

    while (it != tbf::KeyboardFix::swapped_keys.end ()) {
      if (it->first == scancode)
        return SDL_GetKeyFromScancode_Original (it->second);
      else if (it->second == scancode)
        return SDL_GetKeyFromScancode_Original (it->first);
      ++it;
    }
  }

  // This should never happen, but ... just in case.
  return 0;
}
}


#include "log.h"

void
tbf::KeyboardFix::Init (void)
{
  // Don't even hook this stuff if no keyboard remapping is requested.
  if (config.keyboard.swap_keys.empty ())
    return;

  wchar_t* pairs = _wcsdup (config.keyboard.swap_keys.c_str ());
  wchar_t* orig  = pairs;

  size_t len       = wcslen (pairs);
  size_t remaining = len;

  TBF_CreateDLLHook2 ( L"SDL2.dll", "SDL_GetKeyFromScancode",
                       SDL_GetKeyFromScancode_Detour,
            (LPVOID *)&SDL_GetKeyFromScancode_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll", "SDL_GetKeyboardState",
                       SDL_GetKeyboardState_Detour,
            (LPVOID *)&SDL_GetKeyboardState_Original );

  TBF_ApplyQueuedHooks ();

  // Parse the swap pairs
  while (remaining > 0 && remaining <= len) {
    wchar_t* wszSwapPair = pairs;

    size_t pair_len = wcscspn (pairs, L",");

    remaining -= (pair_len + 1);
    pairs     += (pair_len);

    *(pairs++) = L'\0';

    size_t sep = wcscspn (wszSwapPair, L"-");

    *(wszSwapPair + sep) = L'\0';

    wchar_t* wszSwapFirst  = wszSwapPair;
    int16_t  first         = _wtoi  (wszSwapFirst);

    wchar_t* wszSwapSecond = (wszSwapPair + sep + 1);
    int16_t second         = _wtoi  (wszSwapSecond);

    if (remapped_scancodes.find (first)  == remapped_scancodes.end () &&
        remapped_scancodes.find (second) == remapped_scancodes.end ()) {
      remapped_scancodes.insert (first);
      remapped_scancodes.insert (second);

      swapped_keys.push_back (std::pair <uint16_t, uint16_t> (first, second));

      dll_log->Log (L"[ Keyboard ] # SDL Scancode Swap: (%i <-> %i)", first, second);
    } else {
      // Do not allow multiple remapping
      dll_log->Log ( L"[ Keyboard ] @ SDL Scancode Remapped Multiple Times! -- (%i <-> %i)",
                       first,
                         second );
    }
  }

  // Free the copied string
  free (orig);
}

void
tbf::KeyboardFix::Shutdown (void)
{
}

std::set    <uint16_t>                        tbf::KeyboardFix::remapped_scancodes;
std::vector <std::pair <uint16_t, uint16_t> > tbf::KeyboardFix::swapped_keys;