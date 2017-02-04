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

#include <string>
#include <algorithm>

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

//#define LOG_CALL() dll_log->Log ("[SDLTrace] Function: %s, Line: %lu", __FUNCTION__, __LINE__);
#define LOG_CALL() ;


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
          tex_dbg_idx = std::max (0UL, (uint32_t)textures_used_last_dump.size () - 1UL);

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
          tex_dbg_idx = std::max (0UL, (uint32_t)textures_used_last_dump.size () - 1UL);

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


typedef enum
{
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;

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


typedef enum {
	SDL_FALSE = 0,
	SDL_TRUE  = 1
} SDL_bool;

struct SDL_GameController;

typedef                void     (__cdecl *SDL_JoystickUpdate_pfn)(void);
typedef                int      (__cdecl *SDL_NumJoysticks_pfn)(void);
typedef          const char*    (__cdecl *SDL_JoystickNameForIndex_pfn)(int device_index);
typedef            SDL_bool     (__cdecl *SDL_JoystickGetAttached_pfn)(SDL_Joystick* joystick);
typedef        SDL_Joystick*    (__cdecl *SDL_JoystickOpen_pfn)(int device_index);
typedef                void     (__cdecl *SDL_JoystickClose_pfn)(SDL_Joystick* joystick);
typedef          const char*    (__cdecl *SDL_JoystickName_pfn)(SDL_Joystick* joystick);
                                
typedef                void     (__cdecl *SDL_GameControllerUpdate_pfn)(void);
typedef SDL_GameController*     (__cdecl *SDL_GameControllerOpen_pfn)(int joystick_index);
typedef                void     (__cdecl *SDL_GameControllerClose_pfn)(SDL_GameController* controller);
typedef             int16_t     (__cdecl *SDL_GameControllerGetAxis_pfn)(SDL_GameController* controller, SDL_GameControllerAxis);
typedef             uint8_t     (__cdecl *SDL_GameControllerGetButton_pfn)(SDL_GameController* controller, SDL_GameControllerButton);
typedef       SDL_Joystick*     (__cdecl *SDL_GameControllerGetJoystick_pfn)(SDL_GameController* controller);
typedef               char*     (__cdecl *SDL_GameControllerMapping_pfn)(SDL_GameController* controller);

typedef        int16_t          (__cdecl *SDL_JoystickGetAxis_pfn)(SDL_Joystick* joystick, int axis);
typedef        uint8_t          (__cdecl *SDL_JoystickGetButton_pfn)(SDL_Joystick* joystick, int button);
typedef        SDL_JoystickGUID (__cdecl *SDL_JoystickGetGUID_pfn)(SDL_Joystick* joystick);
typedef        SDL_JoystickGUID (__cdecl *SDL_JoystickGetDeviceGUID_pfn)(int device_index);
typedef        void             (__cdecl *SDL_JoystickGetGUIDString_pfn)(SDL_JoystickGUID guid, char* pszGUID, int cbGUID);
typedef        uint8_t          (__cdecl *SDL_JoystickGetHat_pfn)(SDL_Joystick* joystick, int hat);
typedef        int              (__cdecl *SDL_JoystickNumAxes_pfn)(SDL_Joystick* joystick);
typedef        int              (__cdecl *SDL_JoystickNumButtons_pfn)(SDL_Joystick* joystick);
typedef        int              (__cdecl *SDL_JoystickNumHats_pfn)(SDL_Joystick* joystick);

SDL_GameControllerUpdate_pfn      SDL_GameControllerUpdate_Original      = nullptr;
SDL_GameControllerOpen_pfn        SDL_GameControllerOpen_Original        = nullptr;
SDL_GameControllerClose_pfn       SDL_GameControllerClose_Original       = nullptr;
SDL_GameControllerGetAxis_pfn     SDL_GameControllerGetAxis_Original     = nullptr;
SDL_GameControllerGetButton_pfn   SDL_GameControllerGetButton_Original   = nullptr;
SDL_GameControllerGetJoystick_pfn SDL_GameControllerGetJoystick_Original = nullptr;
SDL_GameControllerMapping_pfn     SDL_GameControllerMapping_Original     = nullptr;

SDL_JoystickUpdate_pfn            SDL_JoystickUpdate_Original            = nullptr;
SDL_NumJoysticks_pfn              SDL_NumJoysticks_Original              = nullptr;
SDL_JoystickName_pfn              SDL_JoystickName_Original              = nullptr;
SDL_JoystickNameForIndex_pfn      SDL_JoystickNameForIndex_Original      = nullptr;
SDL_JoystickGetAttached_pfn       SDL_JoystickGetAttached_Original       = nullptr;
SDL_JoystickOpen_pfn              SDL_JoystickOpen_Original              = nullptr;
SDL_JoystickClose_pfn             SDL_JoystickClose_Original             = nullptr;

SDL_JoystickGetAxis_pfn           SDL_JoystickGetAxis_Original           = nullptr;  
SDL_JoystickGetButton_pfn         SDL_JoystickGetButton_Original         = nullptr;
SDL_JoystickGetGUID_pfn           SDL_JoystickGetGUID_Original           = nullptr;
SDL_JoystickGetDeviceGUID_pfn     SDL_JoystickGetDeviceGUID_Original     = nullptr;
SDL_JoystickGetGUIDString_pfn     SDL_JoystickGetGUIDString_Original     = nullptr;
SDL_JoystickGetHat_pfn            SDL_JoystickGetHat_Original            = nullptr;
SDL_JoystickNumAxes_pfn           SDL_JoystickNumAxes_Original           = nullptr;
SDL_JoystickNumButtons_pfn        SDL_JoystickNumButtons_Original        = nullptr;
SDL_JoystickNumHats_pfn           SDL_JoystickNumHats_Original           = nullptr;


typedef SDL_bool (__cdecl *SDL_IsGameController_pfn)(int joystick_index);
SDL_IsGameController_pfn SDL_IsGameController_Original = nullptr;

#include <intrin.h>
#pragma intrinsic (_ReturnAddress)

bool TBF_IsAddressToB (LPCVOID pAddr)
{
  HMODULE hMod = nullptr;

  GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)pAddr, &hMod);

  if (hMod == GetModuleHandle (nullptr))
    return TRUE;

  return FALSE;
}

#define TBF_IsCallerToB() TBF_IsAddressToB (_ReturnAddress ())

SDL_bool
__cdecl
SDL_IsGameController_Detour (int joystick_index)
{
  LOG_CALL ();

  //dll_log->Log (L"device_index = %lu", joystick_index);

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  if (joystick_index >= SDL_NumJoysticks_Original ())
    return SDL_TRUE;

  return SDL_IsGameController_Original (joystick_index);
}

void
__cdecl
SDL_JoystickUpdate_Detour (void)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original ();
}

void
__cdecl
SDL_GameControllerUpdate_Detour (void)
{
  LOG_CALL();

  SDL_GameControllerUpdate_Original ();
}

int
__cdecl
SDL_NumJoysticks_Detour (void)
{
  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  return std::min (4, SDL_NumJoysticks_Original () +  config.input.gamepad.virtual_controllers);
}

const char*
__cdecl
SDL_JoystickName_Detour (struct SDL_Joystick* joystick)
{
  LOG_CALL ();

  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return "TBFix Dummy Controller";

  return SDL_JoystickName_Original (joystick);
}

const char*
__cdecl
SDL_JoystickNameForIndex_Detour (int device_index)
{
  LOG_CALL ();

  //dll_log->Log (L"device_index = %lu", device_index);

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  if (SDL_NumJoysticks_Original () <= device_index)
     return "TBFix Dummy Controller";

  return SDL_JoystickNameForIndex_Original (device_index);
}

SDL_bool
__cdecl
SDL_JoystickGetAttached_Detour (struct SDL_Joystick* joystick)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;


  if (idx >= 0 && idx < 4)
    return SDL_TRUE;

  return SDL_JoystickGetAttached_Original (joystick);
}

SDL_Joystick*
__cdecl
SDL_JoystickOpen_Detour (int device_index)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  if (SDL_NumJoysticks_Original () <= device_index)
    return tbf::InputFix::ai_fix.joysticks [std::max (3, device_index - SDL_NumJoysticks_Original ())].handle;

  return SDL_JoystickOpen_Original (device_index);
}

void
__cdecl
SDL_JoystickClose_Detour (struct SDL_Joystick* joystick)
{
  LOG_CALL ();

  if ( (uintptr_t)joystick >= (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle &&
       (uintptr_t)joystick <= (uintptr_t)tbf::InputFix::ai_fix.joysticks [3].handle )
    return;

  SDL_JoystickClose_Original (joystick);
}

int16_t
__cdecl
SDL_JoystickGetAxis_Detour (struct SDL_Joystick* joystick, int axis)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 0;

  return SDL_JoystickGetAxis_Original (joystick, axis);
}

uint8_t
__cdecl
SDL_JoystickGetButton_Detour (struct SDL_Joystick* joystick, int button)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 0;

  return SDL_JoystickGetButton_Original (joystick, button);
}

SDL_JoystickGUID
__cdecl
SDL_JoystickGetDeviceGUID_Detour (int device_index)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  if (SDL_NumJoysticks_Original () <= device_index)
    return tbf::InputFix::ai_fix.joysticks [device_index - SDL_NumJoysticks_Original ()].guid;

  return SDL_JoystickGetDeviceGUID_Original (device_index);
}

SDL_JoystickGUID
__cdecl
SDL_JoystickGetGUID_Detour (struct SDL_Joystick* joystick)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return tbf::InputFix::ai_fix.joysticks [idx].guid;

  return SDL_JoystickGetGUID_Original (joystick);
}


void
__cdecl
SDL_JoystickGetGUIDString_Detour (SDL_JoystickGUID guid, char* pszGUID, int cbGUID)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  SDL_JoystickGetGUIDString_Original (guid, pszGUID, cbGUID);
}

uint8_t
__cdecl
SDL_JoystickGetHat_Detour (struct SDL_Joystick* joystick, int hat)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 0;

  return SDL_JoystickGetHat_Original (joystick, hat);
}

int
__cdecl
SDL_JoystickNumHats_Detour (struct SDL_Joystick* joystick)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 1; // 1 D-Pad

  return SDL_JoystickNumHats_Original (joystick);
}

int
__cdecl
SDL_JoystickNumButtons_Detour (struct SDL_Joystick* joystick)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 12;

  return SDL_JoystickNumButtons_Original (joystick);
}

int
__cdecl
SDL_JoystickNumAxes_Detour (struct SDL_Joystick* joystick)
{
  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 6; // 2 sticks + 1 trigger

  return SDL_JoystickNumAxes_Original (joystick);
}

void
__cdecl
SDL_GameControllerClose_Detour (SDL_GameController* gamecontroller)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  if ( (uintptr_t)gamecontroller >= (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle &&
       (uintptr_t)gamecontroller <= (uintptr_t)tbf::InputFix::ai_fix.joysticks [3].handle )
  {
    //dll_log->Log (L" %p <==", gamecontroller);
    return;
  }

  return SDL_GameControllerClose_Original (gamecontroller);
}

SDL_GameController*
__cdecl
SDL_GameControllerOpen_Detour (int device_index)
{
  //dll_log->Log (L"Open Controller: %lu", device_index);

  if (device_index >= SDL_NumJoysticks_Original ())
  {
    //dll_log->Log (L" == > %p", tbf::InputFix::ai_fix.joysticks [std::max (3, device_index - SDL_NumJoysticks_Original ())].handle);
    return (SDL_GameController *)tbf::InputFix::ai_fix.joysticks [std::max (3, device_index - SDL_NumJoysticks_Original ())].handle;
  }

  return SDL_GameControllerOpen_Original (device_index);
}

int16_t
__cdecl
SDL_GameControllerGetAxis_Detour (SDL_GameController* controller, SDL_GameControllerAxis axis)
{
  ptrdiff_t idx = (uintptr_t)controller - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 0;

  return SDL_GameControllerGetAxis_Original (controller, axis);
}

uint8_t
__cdecl
SDL_GameControllerGetButton_Detour (SDL_GameController* controller, SDL_GameControllerButton button)
{
  ptrdiff_t idx = (uintptr_t)controller - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return 0;

  return SDL_GameControllerGetButton_Original (controller, button);
}

char*
__cdecl
SDL_GameControllerMapping_Detour (SDL_GameController* controller)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  static char last_mapping [1024];

  ptrdiff_t idx = (uintptr_t)controller - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return last_mapping;

  strncpy (last_mapping, SDL_GameControllerMapping_Original (controller), 1024);

  return SDL_GameControllerMapping_Original (controller);
}

SDL_Joystick*
__cdecl
SDL_GameControllerGetJoystick_Detour (SDL_GameController* controller)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  ptrdiff_t idx = (uintptr_t)controller - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return (SDL_Joystick *)controller;

  return SDL_GameControllerGetJoystick_Original (controller);
}


typedef struct SDL_ControllerDeviceEvent
{
    uint32_t type;        /**< ::SDL_CONTROLLERDEVICEADDED, ::SDL_CONTROLLERDEVICEREMOVED, or ::SDL_CONTROLLERDEVICEREMAPPED */
    uint32_t timestamp;
     int32_t which;       /**< The joystick device index for the ADDED event, instance id for the REMOVED or REMAPPED event */
} SDL_ControllerDeviceEvent;


struct SDL_Haptic;

typedef SDL_Haptic* (__cdecl *SDL_HapticOpenFromJoystick_pfn)(SDL_Joystick* joystick);

SDL_HapticOpenFromJoystick_pfn SDL_HapticOpenFromJoystick_Original = nullptr;

SDL_Haptic*
__cdecl
SDL_HapticOpenFromJoystick_Detour (SDL_Joystick* joystick)
{
  LOG_CALL ();

  SDL_JoystickUpdate_Original       ();
  SDL_GameControllerUpdate_Original ();

  ptrdiff_t idx = (uintptr_t)joystick - (uintptr_t)tbf::InputFix::ai_fix.joysticks [0].handle;

  if (idx >= 0 && idx < 4)
    return nullptr;

  return SDL_HapticOpenFromJoystick_Original (joystick);
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


  if (config.input.gamepad.virtual_controllers) {
  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_NumJoysticks",
                       SDL_NumJoysticks_Detour,
            (LPVOID *)&SDL_NumJoysticks_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickName",
                       SDL_JoystickName_Detour,
            (LPVOID *)&SDL_JoystickName_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickNameForIndex",
                       SDL_JoystickNameForIndex_Detour,
            (LPVOID *)&SDL_JoystickNameForIndex_Original);


  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickUpdate",
                       SDL_JoystickUpdate_Detour,
            (LPVOID *)&SDL_JoystickUpdate_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickOpen",
                       SDL_JoystickOpen_Detour,
            (LPVOID *)&SDL_JoystickOpen_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickClose",
                       SDL_JoystickClose_Detour,
            (LPVOID *)&SDL_JoystickClose_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetAttached",
                       SDL_JoystickGetAttached_Detour,
            (LPVOID *)&SDL_JoystickGetAttached_Original);


  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerUpdate",
                       SDL_GameControllerUpdate_Detour,
            (LPVOID *)&SDL_GameControllerUpdate_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerOpen",
                       SDL_GameControllerOpen_Detour,
            (LPVOID *)&SDL_GameControllerOpen_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_IsGameController",
                       SDL_IsGameController_Detour,
            (LPVOID *)&SDL_IsGameController_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerClose",
                       SDL_GameControllerClose_Detour,
            (LPVOID *)&SDL_GameControllerClose_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerMapping",
                       SDL_GameControllerMapping_Detour,
            (LPVOID *)&SDL_GameControllerMapping_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerGetAxis",
                       SDL_GameControllerGetAxis_Detour,
            (LPVOID *)&SDL_GameControllerGetAxis_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerGetButton",
                       SDL_GameControllerGetButton_Detour,
            (LPVOID *)&SDL_GameControllerGetButton_Original);

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_GameControllerGetJoystick",
                       SDL_GameControllerGetJoystick_Detour,
            (LPVOID *)&SDL_GameControllerGetJoystick_Original);


  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetAxis",
                       SDL_JoystickGetAxis_Detour,
            (LPVOID *)&SDL_JoystickGetAxis_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetButton",
                       SDL_JoystickGetButton_Detour,
            (LPVOID *)&SDL_JoystickGetButton_Original );
    
  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetDeviceGUID",
                       SDL_JoystickGetDeviceGUID_Detour,
            (LPVOID *)&SDL_JoystickGetDeviceGUID_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetGUID",
                       SDL_JoystickGetGUID_Detour,
            (LPVOID *)&SDL_JoystickGetGUID_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetGUIDString",
                       SDL_JoystickGetGUIDString_Detour,
            (LPVOID *)&SDL_JoystickGetGUIDString_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickGetHat",
                       SDL_JoystickGetHat_Detour,
            (LPVOID *)&SDL_JoystickGetHat_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickNumAxes",
                       SDL_JoystickNumAxes_Detour,
            (LPVOID *)&SDL_JoystickNumAxes_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickNumButtons",
                       SDL_JoystickNumButtons_Detour,
            (LPVOID *)&SDL_JoystickNumButtons_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_JoystickNumHats",
                       SDL_JoystickNumHats_Detour,
            (LPVOID *)&SDL_JoystickNumHats_Original );

  TBF_CreateDLLHook2 ( L"SDL2.dll",
                       "SDL_HapticOpenFromJoystick",
                       SDL_HapticOpenFromJoystick_Detour,
            (LPVOID *)&SDL_HapticOpenFromJoystick_Original );
  }

  TBF_ApplyQueuedHooks ();

  tbf::InputFix::ai_fix.joysticks [0].handle = (SDL_Joystick *)(LPVOID)0xDEADBEEFULL;
  tbf::InputFix::ai_fix.joysticks [0].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x31 };

  tbf::InputFix::ai_fix.joysticks [1].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF0ULL;
  tbf::InputFix::ai_fix.joysticks [1].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x32 };

  tbf::InputFix::ai_fix.joysticks [2].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF1ULL;
  tbf::InputFix::ai_fix.joysticks [2].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x33 };

  tbf::InputFix::ai_fix.joysticks [3].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF2ULL;
  tbf::InputFix::ai_fix.joysticks [3].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x34 };

  tbf::InputFix::ai_fix.joysticks [4].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF3ULL;
  tbf::InputFix::ai_fix.joysticks [4].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x35 };

  tbf::InputFix::ai_fix.joysticks [5].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF4ULL;
  tbf::InputFix::ai_fix.joysticks [5].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x36 };

  tbf::InputFix::ai_fix.joysticks [6].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF5ULL;
  tbf::InputFix::ai_fix.joysticks [6].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x37 };

  tbf::InputFix::ai_fix.joysticks [7].handle = (SDL_Joystick *)(LPVOID)0xDEADBEF6ULL;
  tbf::InputFix::ai_fix.joysticks [7].guid = { 0x62, 0x62, 0x7E, 0xBB,
                                               0x39, 0x6D,
                                               0x44, 0xDE,
                                               0x96, 0x44,
                                               0x1D, 0xD3,
                                               0x4D, 0xC8,
                                               0x51, 0x38 };
}

tbf::InputFix::ai_fix_s tbf::InputFix::ai_fix;