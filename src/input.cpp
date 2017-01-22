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

typedef BOOL (WINAPI *GetCursorInfo_pfn)
  (_Inout_ PCURSORINFO pci);

typedef BOOL (WINAPI *GetCursorPos_pfn)
  (_Out_ LPPOINT lpPoint);

typedef BOOL (WINAPI *SetCursorPos_pfn)
(
  _In_ int X,
  _In_ int Y
);

GetCursorInfo_pfn GetCursorInfo_Original = nullptr;
GetCursorPos_pfn  GetCursorPos_Original  = nullptr;
SetCursorPos_pfn  SetCursorPos_Original  = nullptr;

BOOL WINAPI GetCursorInfo_Detour (_Inout_ PCURSORINFO pci);
BOOL WINAPI GetCursorPos_Detour  (_Out_   LPPOINT     lpPoint);
BOOL WINAPI SetCursorPos_Detour  (_In_ int X, _In_ int Y);


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

  pPoint->x = (pPoint->x - xoff) * xscale;
  pPoint->y = (pPoint->y - yoff) * yscale;

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

  if (Control && Shift && vkCode == VK_BACK)  {
    extern void TBFix_ToggleConfigUI (void);
    TBFix_ToggleConfigUI             ();
  }


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
      extern int                    tex_dbg_idx;
      ++tex_dbg_idx;

      extern int debug_tex_id;

      if (tex_dbg_idx < 0 || (! textures_used_last_dump.size ())) {
        tex_dbg_idx  = -1;
        debug_tex_id =  0;
      } else {
        if (tex_dbg_idx >= textures_used_last_dump.size ())
          tex_dbg_idx = max (0, textures_used_last_dump.size () - 1);

        debug_tex_id = (int)textures_used_last_dump [tex_dbg_idx];
      }

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }

    else if (vkCode == VK_OEM_4) {
      extern std::vector <uint32_t> textures_used_last_dump;
      extern int                    tex_dbg_idx;
      extern int                    debug_tex_id;

      --tex_dbg_idx;

      if (tex_dbg_idx < 0 || (! textures_used_last_dump.size ())) {
        tex_dbg_idx  = -1;
        debug_tex_id =  0;
      } else {
        if (tex_dbg_idx >= textures_used_last_dump.size ())
          tex_dbg_idx = max (0, textures_used_last_dump.size () - 1);

        debug_tex_id = (int)textures_used_last_dump [tex_dbg_idx];
      }

      tbf::RenderFix::tex_mgr.updateOSD ();

      return;
    }
  }

  SK_PluginKeyPress_Original (Control, Shift, Alt, vkCode);
}

typedef LRESULT (CALLBACK *DetourWindowProc_pfn)(
                   _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam );

DetourWindowProc_pfn DetourWindowProc_Original = nullptr;

LRESULT
ImGui_ImplDX9_WndProcHandler ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  if (uMsg == WM_SYSCOMMAND)
  {
    //if ( (wParam & 0xfff0) == SC_KEYMENU ) // Disable ALT application menu
      //return 0;
  }

  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
    static POINT last_p = { LONG_MIN, LONG_MIN };

    POINT p;

    p.x = MAKEPOINTS (lParam).x;
    p.y = MAKEPOINTS (lParam).y;

    if (game_state.needsFixedMouseCoords () && config.render.aspect_correction) {
      // Only do this if cursor actually moved!
      //
      //   Otherwise, it tricks the game into thinking the input device changed
      //     from gamepad to mouse (and changes button icons).
      if (last_p.x != p.x || last_p.y != p.y) {
        CalcCursorPos (&p);

        last_p = p;
      }

      return DetourWindowProc_Original (hWnd, uMsg, wParam, MAKELPARAM (p.x, p.y));
    }

    last_p = p;
  }

  return DetourWindowProc_Original (hWnd, uMsg, wParam, lParam);
}

void
tbf::InputFix::Init (void)
{
  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                      "SK_PluginKeyPress",
                      SK_TBF_PluginKeyPress,
           (LPVOID *)&SK_PluginKeyPress_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                      "SK_DetourWindowProc",
                      DetourWindowProc,
           (LPVOID *)&DetourWindowProc_Original );

  TBF_ApplyQueuedHooks ();
}

void
tbf::InputFix::Shutdown (void)
{
}