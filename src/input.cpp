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