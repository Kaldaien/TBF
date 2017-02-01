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
#include "log.h"
#include "command.h"
#include "sound.h"
#include "steam.h"

#include "framerate.h"
#include "render.h"

#include <d3d9.h>

#include "config.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <process.h>
#include <comdef.h>
#include "textures.h"

// Returns the original cursor position and stores the new one in pPoint
POINT
CalcCursorPos (LPPOINT pPoint)
{
  float xscale, yscale;
  float xoff,   yoff;

#if 0
  extern void TBF_ComputeAspectCoeffs ( float& xscale,
                                        float& yscale,
                                        float& xoff,
                                        float& yoff );

  TBF_ComputeAspectCoeffs (xscale, yscale, xoff, yoff);
#else
  xoff = 0.0f;
  yoff = 0.0f;
  xscale = 1.0f;
  yscale = 1.0f;
#endif

  *pPoint = {
    static_cast <LONG> ((static_cast <float> (pPoint->x) - xoff) * xscale),
    static_cast <LONG> ((static_cast <float> (pPoint->y) - yoff) * yscale)
  };

  return *pPoint;
}


typedef void (CALLBACK *SK_PluginKeyPress_pfn)( BOOL Control,
                        BOOL Shift,
                        BOOL Alt,
                        BYTE vkCode );
SK_PluginKeyPress_pfn SK_PluginKeyPress_Original = nullptr;

void
CALLBACK
SK_TBF_PluginKeyPress ( BOOL Control,
                        BOOL Shift,
                        BOOL Alt,
                        BYTE vkCode )
{
  SK_ICommandProcessor& command =
    *SK_GetCommandProcessor ();

  if (Control && Shift) {
    if (vkCode == 'U') {
      command.ProcessCommandLine ("Textures.Remap toggle");

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == 'Z') {
      command.ProcessCommandLine  ("Textures.Purge true");

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == 'X') {
      command.ProcessCommandLine  ("Textures.Trace true");

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == 'V') {
      command.ProcessCommandLine  ("Textures.ShowCache toggle");

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == VK_OEM_6) {
      extern std::vector <uint32_t> textures_used_last_dump;
      extern uint32_t               tex_dbg_idx;
      ++tex_dbg_idx;

      extern uint32_t debug_tex_id;

      if ((int32_t)tex_dbg_idx < 0 || (! textures_used_last_dump.size ())) {
        tex_dbg_idx  = -1;
        debug_tex_id =  0;
      } else {
        if (tex_dbg_idx >= textures_used_last_dump.size ())
          tex_dbg_idx = max (0, (uint32_t)textures_used_last_dump.size () - 1);

        debug_tex_id = textures_used_last_dump [tex_dbg_idx];
      }

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == VK_OEM_4) {
      extern std::vector <uint32_t> textures_used_last_dump;
      extern uint32_t               tex_dbg_idx;
      extern uint32_t               debug_tex_id;

      --tex_dbg_idx;

      if ((int32_t)tex_dbg_idx < 0 || (! textures_used_last_dump.size ())) {
        tex_dbg_idx  = -1;
        debug_tex_id =  0;
      } else {
        if (tex_dbg_idx >= textures_used_last_dump.size ())
          tex_dbg_idx = max (0, (uint32_t)textures_used_last_dump.size () - 1);

        debug_tex_id = textures_used_last_dump [tex_dbg_idx];
      }

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }
  }

  SK_PluginKeyPress_Original (Control, Shift, Alt, vkCode);
}

void
tbf::InputFix::Init (void)
{
  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                      "SK_PluginKeyPress",
                      SK_TBF_PluginKeyPress,
           (LPVOID *)&SK_PluginKeyPress_Original );


  TBF_ApplyQueuedHooks ();
}

void
tbf::InputFix::Shutdown (void)
{
}

typedef enum
{
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;


typedef uint8_t     (__cdecl *SDL_GameControllerGetButton_pfn) ( LPVOID                   controller,
                                                                 SDL_GameControllerButton button );
                                                                          SDL_GameControllerGetButton_pfn SDL_GameControllerGetButton_Original = nullptr;

typedef int         (__cdecl *SDL_ShowCursor_pfn)(int toggle);            SDL_ShowCursor_pfn SDL_ShowCursor_Original                       = nullptr;

typedef const char* (__cdecl *SDL_GetHint_pfn) ( const char* name );      SDL_GetHint_pfn SDL_GetHint_Original                             = nullptr;
typedef BOOL        (__cdecl *SDL_SetHint_pfn) ( const char* name,
                                                 const char* value );     SDL_SetHint_pfn SDL_SetHint_Original                             = nullptr;

typedef uint32_t    (__cdecl *SDL_GetMouseState_pfn) ( int* x,
                                                       int* y );          SDL_GetMouseState_pfn SDL_GetMouseState_Original                 = nullptr;

typedef uint32_t    (__cdecl *SDL_GetRelativeMouseState_pfn) ( int* x,
                                                               int* y );  SDL_GetRelativeMouseState_pfn SDL_GetRelativeMouseState_Original = nullptr;

typedef void        (__cdecl *SDL_WarpMouseInWindow_pfn) ( LPVOID window,
                                                           int    x,
                                                           int    y );    SDL_WarpMouseInWindow_pfn SDL_WarpMouseInWindow_Original         = nullptr;

typedef
    const uint8_t*  (__cdecl *SDL_GetKeyboardState_pfn) ( int* numkeys ); SDL_GetKeyboardState_pfn SDL_GetKeyboardState_Original           = nullptr;


bool cursor_visible = false;

int
__cdecl
SDL_ShowCursor_Detour (int toggle)
{
  if (toggle == 1)
    cursor_visible = true;

  if (toggle == 0)
    cursor_visible = false;

  return SDL_ShowCursor_Original (toggle);
}

uint8_t
__cdecl
SDL_GameControllerGetButton_Detour ( LPVOID                    controller,
                                     SDL_GameControllerButton  button )
{
  return SDL_GameControllerGetButton_Original (controller, button);
}

const char*
__cdecl
SDL_GetHint_Detour ( const char* name )
{
  return SDL_GetHint_Original (name);
}

BOOL
__cdecl
SDL_SetHint_Detour ( const char* name, const char* value )
{
  if (! stricmp (name, "SDL_HINT_MOUSE_RELATIVE_MODE_WARP")) {
    return TRUE;
  }

  return SDL_SetHint_Original (name, value);
}

uint32_t
__cdecl
SDL_GetMouseState_Detour ( int* x, int* y )
{
  uint32_t ret = SDL_GetMouseState_Original ( x, y );

#if 0
  if (! cursor_visible) {
    if (x != nullptr)
      *x = 0;

    if (y != nullptr)
      *y = 0;
  }
#endif

  return ret;
}

const
uint8_t*
__cdecl
SDL_GetKeyboardState_Detour ( int* numkeys )
{
  const uint8_t* ret = SDL_GetKeyboardState_Original (numkeys);

  //26	0x01A	SDL_SCANCODE_W
  //4	0x004	SDL_SCANCODE_A
  //22	0x016	SDL_SCANCODE_S
  //7	0x007	SDL_SCANCODE_D


  //79	0x04F	SDL_SCANCODE_RIGHT
  //80	0x050	SDL_SCANCODE_LEFT
  //81	0x051	SDL_SCANCODE_DOWN
  //82	0x052	SDL_SCANCODE_UP

  if (config.keyboard.swap_wasd) {
    int count = *numkeys;

    static uint8_t keys [1024];

    memcpy (keys, ret, *numkeys);

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

     return keys;
  }

  return ret;
}

typedef int (__cdecl *SDL_GetKeyFromScancode_pfn)(int scancode);
SDL_GetKeyFromScancode_pfn SDL_GetKeyFromScancode_Original = nullptr;

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

  return SDL_GetKeyFromScancode_Original (scancode);
}

typedef int16_t (__cdecl *SDL_GameControllerGetAxis_pfn) ( LPVOID joystick,
                                                           int     axis );
SDL_GameControllerGetAxis_pfn SDL_GameControllerGetAxis_Original = nullptr;

int16_t
__cdecl
SDL_GameControllerGetAxis_Detour ( LPVOID controller,
                                   int    axis )
{
  return SDL_GameControllerGetAxis_Original (controller, axis);
}


void
TBF_InitSDLOverride (void)
{
  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GetHint",
                       SDL_GetHint_Detour,
            (LPVOID *)&SDL_GetHint_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_SetHint",
                       SDL_SetHint_Detour,
            (LPVOID *)&SDL_SetHint_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GetMouseState",
                       SDL_GetMouseState_Detour,
            (LPVOID *)&SDL_GetMouseState_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GetKeyboardState",
                       SDL_GetKeyboardState_Detour,
            (LPVOID *)&SDL_GetKeyboardState_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GetKeyFromScancode",
                       SDL_GetKeyFromScancode_Detour,
            (LPVOID *)&SDL_GetKeyFromScancode_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_ShowCursor",
                       SDL_ShowCursor_Detour,
            (LPVOID *)&SDL_ShowCursor_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerGetButton",
                       SDL_GameControllerGetButton_Detour,
            (LPVOID *)&SDL_GameControllerGetButton_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerGetAxis",
                       SDL_GameControllerGetAxis_Detour,
            (LPVOID *)&SDL_GameControllerGetAxis_Original);

  TBF_ApplyQueuedHooks ();

  SDL_SetHint_Original ("SDL_HINT_MOUSE_RELATIVE_MODE_WARP", "0");
}