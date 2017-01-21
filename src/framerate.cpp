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

#include "framerate.h"
#include "config.h"
#include "log.h"
#include "hook.h"
#include "scanner.h"

#include "render.h"
#include "textures.h"
#include <d3d9.h>

//
// @TODO: Develop a heuristic to scan for this memory address;
//          hardcoding it is going to break stuff :)
//
uintptr_t TICK_ADDR_BASE = 0xB1B074;

int32_t          tbf::FrameRateFix::tick_scale               = INT32_MAX; // 0 FPS

CRITICAL_SECTION tbf::FrameRateFix::alter_speed_cs           = { 0 };

bool             tbf::FrameRateFix::variable_speed_installed = false;
uint32_t         tbf::FrameRateFix::target_fps               = 0;

HMODULE          tbf::FrameRateFix::bink_dll                 = 0;
HMODULE          tbf::FrameRateFix::kernel32_dll             = 0;

typedef void (*NamcoLimiter_pfn)(void);
NamcoLimiter_pfn NamcoLimiter_Original = nullptr;

typedef void* (__stdcall *BinkOpen_pfn)(const char* filename, DWORD unknown0);
BinkOpen_pfn BinkOpen_Original = nullptr;

void*
__stdcall
BinkOpen_Detour ( const char* filename,
                  DWORD       unknown0 )
{
  // non-null on success
  void*       bink_ret = nullptr;
  static char szBypassName [MAX_PATH] = { '\0' };

  // Optionally play some other video (or no video)...
  if (! _stricmp (filename, "RAW\\MOVIE\\AM_TOZ_OP_001.BK2")) {
    dll_log->LogEx (true, L"[IntroVideo] >> Using %ws for Opening Movie ...",
                    config.system.intro_video.c_str ());

    sprintf (szBypassName, "%ws", config.system.intro_video.c_str ());

    bink_ret = BinkOpen_Original (szBypassName, unknown0);

    dll_log->LogEx ( false,
                       L" %s!\n",
                         bink_ret != nullptr ? L"Success" :
                                               L"Failed" );
  } else {
    bink_ret = BinkOpen_Original (filename, unknown0);
  }

  if (bink_ret != nullptr) {
    dll_log->Log (L"[FrameLimit] * Disabling TargetFPS -- Bink Video Opened");

    tbf::RenderFix::bink = true;
    //tbf::FrameRateFix::Begin30FPSEvent ();
  }

  return bink_ret;
}

typedef void (__stdcall *BinkClose_pfn)(DWORD unknown);
BinkClose_pfn BinkClose_Original = nullptr;

void
__stdcall
BinkClose_Detour (DWORD unknown)
{
  BinkClose_Original (unknown);

  dll_log->Log (L"[FrameLimit] * Restoring TargetFPS -- Bink Video Closed");

  tbf::RenderFix::bink = false;
  //tbf::FrameRateFix::End30FPSEvent ();
}



// Hook these to properly synch audio subtitles during FMVs
LPVOID pfnBinkOpen                 = nullptr;
LPVOID pfnBinkClose                = nullptr;

void
TBF_FlushInstructionCache ( LPCVOID base_addr,
                            size_t  code_size )
{
  FlushInstructionCache ( GetCurrentProcess (),
                            base_addr,
                              code_size );
}

void
TBF_InjectMachineCode ( LPVOID   base_addr,
                        uint8_t* new_code,
                        size_t   code_size,
                        DWORD    permissions,
                        uint8_t* old_code = nullptr )
{
  DWORD dwOld;

  VirtualProtect (base_addr, code_size, permissions, &dwOld);
  {
    if (old_code != nullptr)
      memcpy (old_code, base_addr, code_size);

    memcpy (base_addr, new_code, code_size);
  }
  VirtualProtect (base_addr, code_size, dwOld, &dwOld);

  TBF_FlushInstructionCache (base_addr, code_size);
}

typedef BOOL (WINAPI *CreateTimerQueueTimer_pfn)
(
  _Out_    PHANDLE             phNewTimer,
  _In_opt_ HANDLE              TimerQueue,
  _In_     WAITORTIMERCALLBACK Callback,
  _In_opt_ PVOID               Parameter,
  _In_     DWORD               DueTime,
  _In_     DWORD               Period,
  _In_     ULONG               Flags
);

CreateTimerQueueTimer_pfn CreateTimerQueueTimer_Original = nullptr;

BOOL
WINAPI
CreateTimerQueueTimer_Override (
  _Out_    PHANDLE             phNewTimer,
  _In_opt_ HANDLE              TimerQueue,
  _In_     WAITORTIMERCALLBACK Callback,
  _In_opt_ PVOID               Parameter,
  _In_     DWORD               DueTime,
  _In_     DWORD               Period,
  _In_     ULONG               Flags
)
{
  // Fix compliance related issues present in both
  //   Tales of Symphonia and Zestiria
  if (Flags & 0x8) {
    Period = 0;
  }

  return CreateTimerQueueTimer_Original (phNewTimer, TimerQueue, Callback, Parameter, DueTime, Period, Flags);
}

void
NamcoLimiter_Detour(void)
{
  if (! config.framerate.replace_limiter)
    NamcoLimiter_Original ();
}

void
tbf::FrameRateFix::Init (void)
{
  //CommandProcessor* comm_proc = CommandProcessor::getInstance ();

  InitializeCriticalSectionAndSpinCount (&alter_speed_cs, 1000UL);

  target_fps = 0;
  bink_dll   = LoadLibrary (L"bink2w64.dll");

#if 0
  TBF_CreateDLLHook2 ( L"bink2w64.dll", "_BinkOpen@8",
                       BinkOpen_Detour, 
            (LPVOID *)&BinkOpen_Original,
                      &pfnBinkOpen );

  TBF_CreateDLLHook2 ( L"bink2w64.dll", "_BinkClose@4",
                       BinkClose_Detour, 
            (LPVOID *)&BinkClose_Original,
                      &pfnBinkClose );
#endif

  TBF_CreateDLLHook2 ( L"kernel32.dll", "CreateTimerQueueTimer",
                       CreateTimerQueueTimer_Override,
            (LPVOID *)&CreateTimerQueueTimer_Original );

  TBF_ApplyQueuedHooks ();

  //if (config.framerate.replace_limiter) {
    /*
      6D3610 0x48 0x83 0xec 0x38	; Namco Limiter
      6D3610 0xc3 0x90 0x90 0x90	; No Limiter

      Tales of Berseria.exe+6D3610 - 48 83 EC 38           - sub rsp,38 { 56 }

      0x48 0x83 0xec 0x38 0x8b 0x0d -------- 0x85 0xc9
    */

    uint8_t sig  [] = { 0x48, 0x83, 0xec, 0x38,
                        0x8b, 0x0d,             0xff, 0xff, 0xff, 0xff,
                        0x85, 0xc9 };
    uint8_t mask [] = { 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff,             0x00, 0x00, 0x00, 0x00,
                        0xff, 0xff };

    void* limiter_addr =
      TBF_Scan (sig, sizeof (sig), mask);

    if (limiter_addr != nullptr) {
      dll_log->Log ( L"[FrameLimit] Scanned Namco's Framerate Limit Bug Address: %ph",
                       limiter_addr );
    } else {
      dll_log->Log (L"[FrameLimit]  >> ERROR: Unable to find Framerate Limiter code!");
    }

    if (limiter_addr != nullptr) {
      //dll_log->LogEx (true, L"[FrameLimit]  * Installing variable rate simulation... ");

#if 0
      uint8_t disable_inst [] = { 0xc3, 0x90, 0x90, 0x90 };

      TBF_InjectMachineCode ( limiter_addr,
                                disable_inst,
                                  sizeof (disable_inst),
                                    PAGE_EXECUTE_READWRITE );

#endif

      TBF_CreateFuncHook ( L"NamcoLimiter",
                             limiter_addr,
                               NamcoLimiter_Detour,
                    (LPVOID *)&NamcoLimiter_Original );
      TBF_EnableHook    (limiter_addr);


      variable_speed_installed = true;
    }
  //}
}

void
tbf::FrameRateFix::Shutdown (void)
{
  DeleteCriticalSection (&alter_speed_cs);

  FreeLibrary (kernel32_dll);
  FreeLibrary (bink_dll);
}

// Force RevMatching when re-engaging the limiter
void
tbf::FrameRateFix::BlipFramerate (void)
{
  tick_scale = 0;

  // A value of 0 will cause the game's preference be re-engaged.
}

void
tbf::FrameRateFix::DisengageLimiter (void)
{
  SK_GetCommandProcessor ()->ProcessCommandLine ("TargetFPS 0.0");
}


float
tbf::FrameRateFix::GetTargetFrametime (void)
{
  uint32_t* pTickScale =
    (uint32_t *)(TBF_GetBaseAddr () + TICK_ADDR_BASE);

  return ( (float)(*pTickScale) * 16.6666666f );
}

//
// Called at the end of every frame to make sure the mod's replacement framerate limiter
//   is following user preferences.
//
void
tbf::FrameRateFix::RenderTick (void)
{
  uint32_t* pTickScale =
    (uint32_t *)(TBF_GetBaseAddr () + TICK_ADDR_BASE);

  if (*pTickScale != tick_scale) {
    dll_log->Log ( L"[FrameLimit] [@] Framerate limit changed from "
                     L"%04.1f FPS to %04.1f FPS...",
                       60.0f / (float)tick_scale,
                         60.0f / (float)*pTickScale );

    tick_scale =             *pTickScale;
    target_fps = 60 / max (0, tick_scale);

    SK_GetCommandProcessor ()->ProcessCommandFormatted (
      "TargetFPS %f",
        60.0f / (float)tick_scale
    );
  }
}