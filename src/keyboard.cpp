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

#define NOMINMAX
#include <algorithm>

#include "keyboard.h"
#include <windows.h>

#include "hook.h"
#include "config.h"

extern "C" {
typedef const char* (__cdecl *SDL_GetScancodeName_pfn)(SDL_Scancode);
SDL_GetScancodeName_pfn SDL_GetScancodeName = nullptr;

typedef void (__cdecl *SDL_PumpEvents_pfn)(void);
SDL_PumpEvents_pfn SDL_PumpEvents = nullptr;

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

  if (config.keyboard.swap_wasd)
  {
    uint8_t up    = keys [82];
    uint8_t down  = keys [81];
    uint8_t left  = keys [80];
    uint8_t right = keys [79];


    keys [82] = keys [26];
    keys [80] = keys [4];
    keys [81] = keys [22];
    keys [79] = keys [7];

    keys [26] = up;
    keys [22] = down;
    keys [7]  = right;
    keys [4]  = left;
  }

  return keys;
}

int
__cdecl
SDL_GetKeyFromScancode_Detour (int scancode)
{
 if (config.keyboard.swap_wasd)
  {
    if (scancode == 82)
      return SDL_GetKeyFromScancode_Original (26);
    else if (scancode == 81)
      return SDL_GetKeyFromScancode_Original (22);
    else if (scancode == 80)
      return SDL_GetKeyFromScancode_Original (4);
    else if (scancode == 79)
      return SDL_GetKeyFromScancode_Original (7);
    
    else if (scancode == 26)
      return SDL_GetKeyFromScancode_Original (82);
    else if (scancode == 22)
      return SDL_GetKeyFromScancode_Original (81);
    else if (scancode == 4)
      return SDL_GetKeyFromScancode_Original (80);
    else if (scancode == 7)
      return SDL_GetKeyFromScancode_Original (79);
  }


  if (! tbf::KeyboardFix::remapped_scancodes.count (scancode)) {
    return SDL_GetKeyFromScancode_Original (scancode);
  }

  else
  {
    std::vector <std::pair <uint16_t, uint16_t>>::iterator it =
      tbf::KeyboardFix::swapped_keys.begin ();

    while (it != tbf::KeyboardFix::swapped_keys.end ())
    {
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
  wchar_t* pairs = _wcsdup (config.keyboard.swap_keys.c_str ());
  wchar_t* orig  = pairs;

  size_t len       = wcslen (pairs);
  size_t remaining = len;

  SDL_GetScancodeName =
    (SDL_GetScancodeName_pfn)
      GetProcAddress ( GetModuleHandle (L"SDL2.dll"), "SDL_GetScancodeName" );

  SDL_PumpEvents =
    (SDL_PumpEvents_pfn)
      GetProcAddress ( GetModuleHandle (L"SDL2.dll"), "SDL_PumpEvents" );

  TBF_CreateDLLHook2 ( L"SDL2.dll", "SDL_GetKeyFromScancode",
                       SDL_GetKeyFromScancode_Detour,
            (LPVOID *)&SDL_GetKeyFromScancode_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll", "SDL_GetKeyboardState",
                       SDL_GetKeyboardState_Detour,
            (LPVOID *)&SDL_GetKeyboardState_Original );

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

    if ( remapped_scancodes.find (first)  == remapped_scancodes.end () &&
         remapped_scancodes.find (second) == remapped_scancodes.end () )
    {
      remapped_scancodes.insert (first);
      remapped_scancodes.insert (second);

      swapped_keys.push_back (std::pair <uint16_t, uint16_t> (first, second));

      dll_log->Log (L"[ Keyboard ] # SDL Scancode Swap: (%i <-> %i)", first, second);
    } 

    else
    {
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
tbf::KeyboardFix::UpdateConfig (void)
{
  config.keyboard.swap_keys = L"";

  for (int i = 0; i < swapped_keys.size (); i++)
  {
    wchar_t wszPair [64] = { L'\0' };

    swprintf ( wszPair, 63,
                 L"%lu-%lu",
                   swapped_keys [i].first,
                   swapped_keys [i].second );
 
    config.keyboard.swap_keys += wszPair;

    if (i < (swapped_keys.size () - 1))
      config.keyboard.swap_keys += L",";
  }
}

void
tbf::KeyboardFix::Shutdown (void)
{
  UpdateConfig ();
}


#include <imgui/imgui.h>

static bool list_dirty = true;

void
TBF_DrawRemapList (void)
{
  const   float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  struct enumerated_remap_s
  {
    struct {
      std::string  name;
      SDL_Scancode scancode;
    } in, out;
  };

  static std::vector <enumerated_remap_s> remaps;
  static int                              sel        = 0;

  auto EnumerateRemap =
    [](int idx) ->
      enumerated_remap_s
      {
        SDL_Scancode scan0 = tbf::KeyboardFix::swapped_keys [idx].first,
                     scan1 = tbf::KeyboardFix::swapped_keys [idx].second;

        enumerated_remap_s remap;
        remap.in.scancode = scan0;
        remap.in.name     = SDL_GetScancodeName (scan0);

        remap.out.scancode = scan1;
        remap.out.name     = SDL_GetScancodeName (scan1);

        return remap;
      };

  if (list_dirty)
  {
    remaps.clear ();

    for ( int i  = 0; i < tbf::KeyboardFix::swapped_keys.size (); i++ ) {
      remaps.push_back (EnumerateRemap (i));
    }

    list_dirty = false;
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border,                   ImVec4 (0.4f, 0.6f, 0.9f, 1.0f));

#define REMAP_LIST_WIDTH  font_size * 30.0f
#define REMAP_LIST_HEIGHT font_size * 5.0f

  ImGui::BeginChild ( "Remap List",
                        ImVec2 ( REMAP_LIST_WIDTH, std::max (44.0f, font_size * ((float)remaps.size () + 3.0f)) ),
                          true,
                            ImGuiWindowFlags_AlwaysAutoResize );

  if (remaps.size ())
  {
    static      int last_sel = 0;
    static bool sel_changed  = false;
  
    if (sel != last_sel)
      sel_changed = true;
  
    last_sel = sel;
  
    for ( int line = 0; line < remaps.size (); line++)
    {
      char szDescription [512] = { '\0' };

      snprintf ( szDescription, 511, "%s  <==>  %s", remaps [line].in.name.c_str  (),
                                                     remaps [line].out.name.c_str () );
      if (line == sel)
      {
        bool selected = true;
        ImGui::Selectable (szDescription, &selected);
   
        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
          sel_changed = false;
        }
      }
   
      else
      {
        bool selected = false;

        if (ImGui::Selectable (szDescription, &selected))
        {
          sel_changed = true;
          sel         = line;
        }
      }

      //if (ImGui::IsItemHovered ())
        //DataSourceTooltip (line);
    }
  }

  ImGui::EndChild      ();

  ImVec2 list_size = ImGui::GetItemRectSize ();

  ImGui::PopStyleColor ();
  ImGui::PopStyleVar   ();

  ImGui::SameLine   ();

  ImGui::BeginGroup ();

  if (ImGui::Button ("Add Remapping"))
  {
    ImGui::OpenPopup ("Keyboard Remap");
  }

  if (remaps.size () && sel >= 0 && sel < remaps.size ())
  {
    if (ImGui::Button ("Remove Remapping"))
    {
      enumerated_remap_s remap = remaps [sel];

      tbf::KeyboardFix::remapped_scancodes.erase (remap.in.scancode);
      tbf::KeyboardFix::remapped_scancodes.erase (remap.out.scancode);

      std::vector <std::pair <uint16_t, uint16_t>>::iterator it =
        tbf::KeyboardFix::swapped_keys.begin ();

      while (it != tbf::KeyboardFix::swapped_keys.end ())
      {
        if (it->first == remap.in.scancode && it->second == remap.out.scancode) {
          tbf::KeyboardFix::swapped_keys.erase (it);
          break;
        }

        ++it;
      }

      list_dirty = true;

      tbf::KeyboardFix::UpdateConfig ();
    }
  }

  ImGui::EndGroup   ();
}

uint8_t remap_keys [1024];

int            last_key      = 0;
std::string    remap_in      = "";
std::string    remap_out     = "";
SDL_Scancode   remap_in_sdl  = 0;
SDL_Scancode   remap_out_sdl = 0;

void
tbf::KeyboardFix::DrawControlPanel (void)
{
  ImGui::Checkbox  ("Swap WASD and Arrow Keys", &config.keyboard.swap_wasd);

  TBF_DrawRemapList ();

  RemapDialog ();
}

bool
tbf::KeyboardFix::RemapDialog (void)
{
  const  float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;
  static bool  was_open  = false;

  if (ImGui::BeginPopupModal ("Keyboard Remap", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    int keys = 1024;

    if (! was_open) {
      memcpy (remap_keys, SDL_GetKeyboardState_Original (&keys), keys);

      remap_in  = "";
      remap_out = "";
      last_key  =  0;

      was_open = true;
    }

    uint8_t* active_keys = SDL_GetKeyboardState_Original (&keys);

    if (memcmp (active_keys, remap_keys, keys))
    {
      int i = 0;

      for (i = 0; i < keys; i++) {
        if ( active_keys [i] != remap_keys [i] && last_key != i )
          break;
      }

      if (i != keys && (! tbf::KeyboardFix::remapped_scancodes.count (i)))
      {
        last_key = i;

        if (remap_in != "") {
          remap_out = SDL_GetScancodeName (i);
          ImGui::CloseCurrentPopup ();
          was_open = false;

          remap_out_sdl = i;

          tbf::KeyboardFix::remapped_scancodes.insert (remap_in_sdl);
          tbf::KeyboardFix::remapped_scancodes.insert (remap_out_sdl);

          tbf::KeyboardFix::swapped_keys.push_back (std::make_pair (remap_in_sdl, remap_out_sdl));

          list_dirty = true;

          UpdateConfig ();
        }
        
        else {
          remap_in_sdl = i;
          remap_in     = SDL_GetScancodeName (i);
        }
      }

      memcpy (remap_keys, active_keys, keys);
    }

    ImGui::Text ("Input:  %s", remap_in.c_str ());

    if (remap_in != "") {
      ImGui::SameLine ();
      ImGui::Text ("  ==>  Output: %s", remap_out.c_str ());
    }

    ImGui::EndPopup();
  }

  return true;
}

std::set    <uint16_t>                        tbf::KeyboardFix::remapped_scancodes;
std::vector <std::pair <uint16_t, uint16_t> > tbf::KeyboardFix::swapped_keys;