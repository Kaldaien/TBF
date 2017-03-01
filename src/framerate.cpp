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

#define NOMINMAX

#include "framerate.h"
#include "config.h"
#include "log.h"
#include "hook.h"
#include "scanner.h"

#include "render.h"
#include "textures.h"
#include <d3d9.h>


#pragma pack(push,1)

typedef struct {
    uint8_t len;
    uint8_t p_rep;
    uint8_t p_lock;
    uint8_t p_seg;
    uint8_t p_66;
    uint8_t p_67;
    uint8_t rex;
    uint8_t rex_w;
    uint8_t rex_r;
    uint8_t rex_x;
    uint8_t rex_b;
    uint8_t opcode;
    uint8_t opcode2;
    uint8_t modrm;
    uint8_t modrm_mod;
    uint8_t modrm_reg;
    uint8_t modrm_rm;
    uint8_t sib;
    uint8_t sib_scale;
    uint8_t sib_index;
    uint8_t sib_base;
    union {
        uint8_t imm8;
        uint16_t imm16;
        uint32_t imm32;
        uint64_t imm64;
    } imm;
    union {
        uint8_t disp8;
        uint16_t disp16;
        uint32_t disp32;
    } disp;
    uint32_t flags;
} hde64s;

#pragma pack(pop)

extern "C" unsigned int hde64_disasm(const void *code, hde64s *hs);



volatile uintptr_t TICK_ADDR_BASE = 0x0UL;

int32_t            tbf::FrameRateFix::tick_scale               = INT32_MAX; // 0 FPS
bool               tbf::FrameRateFix::need_reset               = false;

CRITICAL_SECTION   tbf::FrameRateFix::alter_speed_cs           = { 0 };

bool               tbf::FrameRateFix::variable_speed_installed = false;
float              tbf::FrameRateFix::target_fps               = 0.0f;

HMODULE            tbf::FrameRateFix::bink_dll                 = 0;
HMODULE            tbf::FrameRateFix::kernel32_dll             = 0;

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
  // Fix compliance related issues   in both
  //   Tales of Symphonia and Zestiria
  if (Flags & 0x8) {
    Period = 0;
  }

  return CreateTimerQueueTimer_Original (phNewTimer, TimerQueue, Callback, Parameter, DueTime, Period, Flags);
}

void
NamcoLimiter_Detour (void)
{
  static uint64_t frame_num      = 0;
  static uint8_t  last_load_flag = *TBF_GetFlagFromIdx (24);

  ++frame_num;

  extern int needs_aspect;

  uint8_t test_state [32] = { 0 };
  test_state [8] = 1;

  if (! memcmp (TBF_GetFlagFromIdx (0), test_state, 26))
  {
    if (game_state.clear_enemies.want && *TBF_GetFlagFromIdx (8))
    {
      if (game_state.clear_enemies.frame == 0)
      {
        *TBF_GetFlagFromIdx (27)       = 1;
        game_state.clear_enemies.frame = frame_num;
      }
      
      else {
        *TBF_GetFlagFromIdx (27)       = 0;
        game_state.clear_enemies.want  = false;
        game_state.clear_enemies.frame = 0;
      }
    }

    if (game_state.respawn_enemies.want && *TBF_GetFlagFromIdx (8))
    {
      if (game_state.respawn_enemies.frame == 0 || game_state.respawn_enemies.frame > frame_num - 60) {
        *TBF_GetFlagFromIdx (27) = 1;

        if (game_state.respawn_enemies.frame == 0)
          game_state.respawn_enemies.frame = frame_num;
      }

      else
      {
        *TBF_GetFlagFromIdx (27)         = 0;
        game_state.respawn_enemies.frame = 0;
        game_state.respawn_enemies.want  = false;
      }
    }
  }

  // No Limit While Loading
  if (*TBF_GetFlagFromIdx (24))
  {
    if (config.framerate.replace_limiter && (! last_load_flag))
      tbf::FrameRateFix::DisengageLimiter ();
    else {
      last_load_flag = *TBF_GetFlagFromIdx (24);
      return;
    }
  }

  else if (last_load_flag)
  {
    if (config.framerate.replace_limiter)
      tbf::FrameRateFix::BlipFramerate ();
  }

  last_load_flag = *TBF_GetFlagFromIdx (24);


  if ((! config.framerate.replace_limiter) || tbf::FrameRateFix::need_reset)
    NamcoLimiter_Original ();

  if (tbf::FrameRateFix::need_reset)
    tbf::FrameRateFix::need_reset = false;
}

void
tbf::FrameRateFix::Init (void)
{
  //CommandProcessor* comm_proc = CommandProcessor::getInstance ();

//  InitializeCriticalSectionAndSpinCount (&alter_speed_cs, 1000UL);

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

  //Tales of Berseria.Kaim::Thread::`default constructor closure'+251F60 - 48 83 EC 38           - sub rsp,38 { 56 }
  //Tales of Berseria.Kaim::Thread::`default constructor closure'+251F64 - 8B 0D 3A691FFF        - mov ecx,["Tales of Berseria.exe"+B15074] { [1104.03] }


  uint8_t sig  []    = { 0x48, 0x83, 0xec, 0x38, 0X8B, 0x0d, 0x3A, 0x69, 0x1F, 0xFF, 0x85, 0xc9 };
  uint8_t mask []    = { 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff };

  void* limiter_addr =
    TBF_Scan (sig, sizeof (sig), mask, 16);
  // x64 functions are 16-byte aligned in Microsoft's ABI, use this to speed the search up

  uintptr_t rip      = (uintptr_t)limiter_addr + 18;

  if (limiter_addr != nullptr)
  {
    dll_log->Log ( L"[FrameLimit] Scanned Namco's Framerate Limit Bug Address: %ph",
                     limiter_addr );
  }

  else {
    dll_log->Log (L"[FrameLimit]  >> ERROR: Unable to find Framerate Limiter code!");
  }

  if (limiter_addr != nullptr)
  {
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

      hde64s    dis = { };
      
      hde64_disasm ((void *)(rip), &dis);
      
      uintptr_t off =
        (uintptr_t)(dis.disp.disp32 + rip + 0x6) & ( 0x00FFFFFFUL );
      
      dll_log->Log ( L"[FrameLimit] Framerate Limiter Preference at Addr.: (Base+%ph)", (LPVOID *)off);
      
      config.framerate.limit_addr = off;
      TICK_ADDR_BASE              = off;
      
      
       DWORD*                   pTickAddr = 
         (DWORD *)(TBF_GetBaseAddr () + TICK_ADDR_BASE);

     __try
     {
       // Sit and spin -- Initialization happens from a separate thread, so this is
       //                   acceptable behavior.
       while (! ( (*pTickAddr == 1) ||
                  (*pTickAddr == 2) ) )
         Sleep (150UL);


       variable_speed_installed = true;
     }

     __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
     {
       dll_log->Log ( L"[Import Tbl] Access Violation While Attempting to "
                      L"Install Variable Rate Limiter." );
     }

     TBF_EnableHook    (limiter_addr);
   }
}

void
tbf::FrameRateFix::Shutdown (void)
{
  //DeleteCriticalSection (&alter_speed_cs);

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
  if (! variable_speed_installed)
    return 16.666666f;

  volatile int32_t* pTickScale =
    (volatile int32_t *)(TBF_GetBaseAddr () + TICK_ADDR_BASE);

  return ( (float)(*pTickScale) * 16.6666666f );
}

//
// Called at the end of every frame to make sure the mod's replacement framerate limiter
//   is following user preferences.
//
void
tbf::FrameRateFix::RenderTick (void)
{
  volatile int32_t* pTickScale =
    (volatile int32_t *)(TBF_GetBaseAddr () + TICK_ADDR_BASE);

  // In case we fail to locate the actual variable in memory,
  //   allback to this...
  if (! variable_speed_installed)
    pTickScale = &tick_scale;

  if (*pTickScale != tick_scale)
{
    dll_log->Log ( L"[FrameLimit] [@] Framerate limit changed from "
                     L"%04.1f FPS to %04.1f FPS...",
                       60.0f / (float)tick_scale,
                         60.0f / (float)*pTickScale );

    tick_scale =             *pTickScale;
    target_fps = 60.0f / std::max (1.0f, (float)tick_scale);

    SK_GetCommandProcessor ()->ProcessCommandFormatted (
      "TargetFPS %f",
        60.0f / (float)tick_scale
    );

    SK_GetCommandProcessor()->ProcessCommandFormatted(
      "LimiterTolerance %f",
        config.framerate.tolerance
    );
  }
}





















#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xfd
#define DELTA_FPU_MODRM    0x104
#define DELTA_PREFIXES     0x13c
#define DELTA_OP_LOCK_OK   0x1ae
#define DELTA_OP2_LOCK_OK  0x1c6
#define DELTA_OP_ONLY_MEM  0x1d8
#define DELTA_OP2_ONLY_MEM 0x1e7

unsigned char hde64_table[] = {
  0xa5,0xaa,0xa5,0xb8,0xa5,0xaa,0xa5,0xaa,0xa5,0xb8,0xa5,0xb8,0xa5,0xb8,0xa5,
  0xb8,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xac,0xc0,0xcc,0xc0,0xa1,0xa1,
  0xa1,0xa1,0xb1,0xa5,0xa5,0xa6,0xc0,0xc0,0xd7,0xda,0xe0,0xc0,0xe4,0xc0,0xea,
  0xea,0xe0,0xe0,0x98,0xc8,0xee,0xf1,0xa5,0xd3,0xa5,0xa5,0xa1,0xea,0x9e,0xc0,
  0xc0,0xc2,0xc0,0xe6,0x03,0x7f,0x11,0x7f,0x01,0x7f,0x01,0x3f,0x01,0x01,0xab,
  0x8b,0x90,0x64,0x5b,0x5b,0x5b,0x5b,0x5b,0x92,0x5b,0x5b,0x76,0x90,0x92,0x92,
  0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x6a,0x73,0x90,
  0x5b,0x52,0x52,0x52,0x52,0x5b,0x5b,0x5b,0x5b,0x77,0x7c,0x77,0x85,0x5b,0x5b,
  0x70,0x5b,0x7a,0xaf,0x76,0x76,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,
  0x5b,0x5b,0x86,0x01,0x03,0x01,0x04,0x03,0xd5,0x03,0xd5,0x03,0xcc,0x01,0xbc,
  0x03,0xf0,0x03,0x03,0x04,0x00,0x50,0x50,0x50,0x50,0xff,0x20,0x20,0x20,0x20,
  0x01,0x01,0x01,0x01,0xc4,0x02,0x10,0xff,0xff,0xff,0x01,0x00,0x03,0x11,0xff,
  0x03,0xc4,0xc6,0xc8,0x02,0x10,0x00,0xff,0xcc,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x01,0x01,0x03,0x01,0xff,0xff,0xc0,0xc2,0x10,0x11,0x02,0x03,0x01,0x01,
  0x01,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0x10,
  0x10,0x10,0x10,0x02,0x10,0x00,0x00,0xc6,0xc8,0x02,0x02,0x02,0x02,0x06,0x00,
  0x04,0x00,0x02,0xff,0x00,0xc0,0xc2,0x01,0x01,0x03,0x03,0x03,0xca,0x40,0x00,
  0x0a,0x00,0x04,0x00,0x00,0x00,0x00,0x7f,0x00,0x33,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0xff,0xbf,0xff,0xff,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0xff,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
  0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,
  0xff,0x40,0x40,0x40,0x40,0x41,0x49,0x40,0x40,0x40,0x40,0x4c,0x42,0x40,0x40,
  0x40,0x40,0x40,0x40,0x40,0x40,0x4f,0x44,0x53,0x40,0x40,0x40,0x44,0x57,0x43,
  0x5c,0x40,0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
  0x40,0x40,0x64,0x66,0x6e,0x6b,0x40,0x40,0x6a,0x46,0x40,0x40,0x44,0x46,0x40,
  0x40,0x5b,0x44,0x40,0x40,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x01,0x06,
  0x06,0x02,0x06,0x06,0x00,0x06,0x00,0x0a,0x0a,0x00,0x00,0x00,0x02,0x07,0x07,
  0x06,0x02,0x0d,0x06,0x06,0x06,0x0e,0x05,0x05,0x02,0x02,0x00,0x00,0x04,0x04,
  0x04,0x04,0x05,0x06,0x06,0x06,0x00,0x00,0x00,0x0e,0x00,0x00,0x08,0x00,0x10,
  0x00,0x18,0x00,0x20,0x00,0x28,0x00,0x30,0x00,0x80,0x01,0x82,0x01,0x86,0x00,
  0xf6,0xcf,0xfe,0x3f,0xab,0x00,0xb0,0x00,0xb1,0x00,0xb3,0x00,0xba,0xf8,0xbb,
  0x00,0xc0,0x00,0xc1,0x00,0xc7,0xbf,0x62,0xff,0x00,0x8d,0xff,0x00,0xc4,0xff,
  0x00,0xc5,0xff,0x00,0xff,0xff,0xeb,0x01,0xff,0x0e,0x12,0x08,0x00,0x13,0x09,
  0x00,0x16,0x08,0x00,0x17,0x09,0x00,0x2b,0x09,0x00,0xae,0xff,0x07,0xb2,0xff,
  0x00,0xb4,0xff,0x00,0xb5,0xff,0x00,0xc3,0x01,0x00,0xc7,0xff,0xbf,0xe7,0x08,
  0x00,0xf0,0x02,0x00
};

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_IMM64         0x00000020
#define F_DISP8         0x00000040
#define F_DISP16        0x00000080
#define F_DISP32        0x00000100
#define F_RELATIVE      0x00000200
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_REX    0x40000000
#define F_PREFIX_ANY    0x7f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67


extern "C"
unsigned int
hde64_disasm (const void *code, hde64s *hs)
{
    uint8_t x, c, *p = (uint8_t *)code, cflags, opcode, pref = 0;
    uint8_t *ht = hde64_table, m_mod, m_reg, m_rm, disp_size = 0;
    uint8_t op64 = 0;

    // Avoid using memset to reduce the footprint.
#ifndef _MSC_VER
    memset((LPBYTE)hs, 0, sizeof(hde64s));
#else
    __stosb((LPBYTE)hs, 0, sizeof(hde64s));
#endif

    for (x = 16; x; x--)
        switch (c = *p++) {
            case 0xf3:
                hs->p_rep = c;
                pref |= PRE_F3;
                break;
            case 0xf2:
                hs->p_rep = c;
                pref |= PRE_F2;
                break;
            case 0xf0:
                hs->p_lock = c;
                pref |= PRE_LOCK;
                break;
            case 0x26: case 0x2e: case 0x36:
            case 0x3e: case 0x64: case 0x65:
                hs->p_seg = c;
                pref |= PRE_SEG;
                break;
            case 0x66:
                hs->p_66 = c;
                pref |= PRE_66;
                break;
            case 0x67:
                hs->p_67 = c;
                pref |= PRE_67;
                break;
            default:
                goto pref_done;
        }
  pref_done:

    hs->flags = (uint32_t)pref << 23;

    if (!pref)
        pref |= PRE_NONE;

    if ((c & 0xf0) == 0x40) {
        hs->flags |= F_PREFIX_REX;
        if ((hs->rex_w = (c & 0xf) >> 3) && (*p & 0xf8) == 0xb8)
            op64++;
        hs->rex_r = (c & 7) >> 2;
        hs->rex_x = (c & 3) >> 1;
        hs->rex_b = c & 1;
        if (((c = *p++) & 0xf0) == 0x40) {
            opcode = c;
            goto error_opcode;
        }
    }

    if ((hs->opcode = c) == 0x0f) {
        hs->opcode2 = c = *p++;
        ht += DELTA_OPCODES;
    } else if (c >= 0xa0 && c <= 0xa3) {
        op64++;
        if (pref & PRE_67)
            pref |= PRE_66;
        else
            pref &= ~PRE_66;
    }

    opcode = c;
    cflags = ht[ht[opcode / 4] + (opcode % 4)];

    if (cflags == C_ERROR) {
      error_opcode:
        hs->flags |= F_ERROR | F_ERROR_OPCODE;
        cflags = 0;
        if ((opcode & -3) == 0x24)
            cflags++;
    }

    x = 0;
    if (cflags & C_GROUP) {
        uint16_t t;
        t = *(uint16_t *)(ht + (cflags & 0x7f));
        cflags = (uint8_t)t;
        x = (uint8_t)(t >> 8);
    }

    if (hs->opcode2) {
        ht = hde64_table + DELTA_PREFIXES;
        if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
            hs->flags |= F_ERROR | F_ERROR_OPCODE;
    }

    if (cflags & C_MODRM) {
        hs->flags |= F_MODRM;
        hs->modrm = c = *p++;
        hs->modrm_mod = m_mod = c >> 6;
        hs->modrm_rm = m_rm = c & 7;
        hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

        if (x && ((x << m_reg) & 0x80))
            hs->flags |= F_ERROR | F_ERROR_OPCODE;

        if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
            uint8_t t = opcode - 0xd9;
            if (m_mod == 3) {
                ht = hde64_table + DELTA_FPU_MODRM + t*8;
                t = ht[m_reg] << m_rm;
            } else {
                ht = hde64_table + DELTA_FPU_REG;
                t = ht[t] << m_reg;
            }
            if (t & 0x80)
                hs->flags |= F_ERROR | F_ERROR_OPCODE;
        }

        if (pref & PRE_LOCK) {
            if (m_mod == 3) {
                hs->flags |= F_ERROR | F_ERROR_LOCK;
            } else {
                uint8_t *table_end, op = opcode;
                if (hs->opcode2) {
                    ht = hde64_table + DELTA_OP2_LOCK_OK;
                    table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
                } else {
                    ht = hde64_table + DELTA_OP_LOCK_OK;
                    table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
                    op &= -2;
                }
                for (; ht != table_end; ht++)
                    if (*ht++ == op) {
                        if (!((*ht << m_reg) & 0x80))
                            goto no_lock_error;
                        else
                            break;
                    }
                hs->flags |= F_ERROR | F_ERROR_LOCK;
              no_lock_error:
                ;
            }
        }

        if (hs->opcode2) {
            switch (opcode) {
                case 0x20: case 0x22:
                    m_mod = 3;
                    if (m_reg > 4 || m_reg == 1)
                        goto error_operand;
                    else
                        goto no_error_operand;
                case 0x21: case 0x23:
                    m_mod = 3;
                    if (m_reg == 4 || m_reg == 5)
                        goto error_operand;
                    else
                        goto no_error_operand;
            }
        } else {
            switch (opcode) {
                case 0x8c:
                    if (m_reg > 5)
                        goto error_operand;
                    else
                        goto no_error_operand;
                case 0x8e:
                    if (m_reg == 1 || m_reg > 5)
                        goto error_operand;
                    else
                        goto no_error_operand;
            }
        }

        if (m_mod == 3) {
            uint8_t *table_end;
            if (hs->opcode2) {
                ht = hde64_table + DELTA_OP2_ONLY_MEM;
                table_end = ht + sizeof(hde64_table) - DELTA_OP2_ONLY_MEM;
            } else {
                ht = hde64_table + DELTA_OP_ONLY_MEM;
                table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
            }
            for (; ht != table_end; ht += 2)
                if (*ht++ == opcode) {
                    if (*ht++ & pref && !((*ht << m_reg) & 0x80))
                        goto error_operand;
                    else
                        break;
                }
            goto no_error_operand;
        } else if (hs->opcode2) {
            switch (opcode) {
                case 0x50: case 0xd7: case 0xf7:
                    if (pref & (PRE_NONE | PRE_66))
                        goto error_operand;
                    break;
                case 0xd6:
                    if (pref & (PRE_F2 | PRE_F3))
                        goto error_operand;
                    break;
                case 0xc5:
                    goto error_operand;
            }
            goto no_error_operand;
        } else
            goto no_error_operand;

      error_operand:
        hs->flags |= F_ERROR | F_ERROR_OPERAND;
      no_error_operand:

        c = *p++;
        if (m_reg <= 1) {
            if (opcode == 0xf6)
                cflags |= C_IMM8;
            else if (opcode == 0xf7)
                cflags |= C_IMM_P66;
        }

        switch (m_mod) {
            case 0:
                if (pref & PRE_67) {
                    if (m_rm == 6)
                        disp_size = 2;
                } else
                    if (m_rm == 5)
                        disp_size = 4;
                break;
            case 1:
                disp_size = 1;
                break;
            case 2:
                disp_size = 2;
                if (!(pref & PRE_67))
                    disp_size <<= 1;
        }

        if (m_mod != 3 && m_rm == 4) {
            hs->flags |= F_SIB;
            p++;
            hs->sib = c;
            hs->sib_scale = c >> 6;
            hs->sib_index = (c & 0x3f) >> 3;
            if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
                disp_size = 4;
        }

        p--;
        switch (disp_size) {
            case 1:
                hs->flags |= F_DISP8;
                hs->disp.disp8 = *p;
                break;
            case 2:
                hs->flags |= F_DISP16;
                hs->disp.disp16 = *(uint16_t *)p;
                break;
            case 4:
                hs->flags |= F_DISP32;
                hs->disp.disp32 = *(uint32_t *)p;
        }
        p += disp_size;
    } else if (pref & PRE_LOCK)
        hs->flags |= F_ERROR | F_ERROR_LOCK;

    if (cflags & C_IMM_P66) {
        if (cflags & C_REL32) {
            if (pref & PRE_66) {
                hs->flags |= F_IMM16 | F_RELATIVE;
                hs->imm.imm16 = *(uint16_t *)p;
                p += 2;
                goto disasm_done;
            }
            goto rel32_ok;
        }
        if (op64) {
            hs->flags |= F_IMM64;
            hs->imm.imm64 = *(uint64_t *)p;
            p += 8;
        } else if (!(pref & PRE_66)) {
            hs->flags |= F_IMM32;
            hs->imm.imm32 = *(uint32_t *)p;
            p += 4;
        } else
            goto imm16_ok;
    }


    if (cflags & C_IMM16) {
      imm16_ok:
        hs->flags |= F_IMM16;
        hs->imm.imm16 = *(uint16_t *)p;
        p += 2;
    }
    if (cflags & C_IMM8) {
        hs->flags |= F_IMM8;
        hs->imm.imm8 = *p++;
    }

    if (cflags & C_REL32) {
      rel32_ok:
        hs->flags |= F_IMM32 | F_RELATIVE;
        hs->imm.imm32 = *(uint32_t *)p;
        p += 4;
    } else if (cflags & C_REL8) {
        hs->flags |= F_IMM8 | F_RELATIVE;
        hs->imm.imm8 = *p++;
    }

  disasm_done:

    if ((hs->len = (uint8_t)(p-(uint8_t *)code)) > 15) {
        hs->flags |= F_ERROR | F_ERROR_LENGTH;
        hs->len = 15;
    }

    return (unsigned int)hs->len;
}