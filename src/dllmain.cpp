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

#include <Windows.h>

#include "config.h"
#include "log.h"

#include "sound.h"
#include "framerate.h"
#include "general_io.h"
#include "keyboard.h"
#include "steam.h"
#include "render.h"
#include "input.h"

#include "command.h"
#include "hook.h"

#include "scanner.h"

#include "tls.h"

#pragma comment (lib, "kernel32.lib")

typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)   (const wchar_t* wszProduct);
typedef bool    (__stdcall *SK_FetchVersionInfo_pfn) (const wchar_t* wszProduct);
typedef void    (__stdcall *SKX_SetPluginName_pfn)   (const wchar_t* wszName);

std::wstring injector_dll;

HMODULE hDLLMod      = { 0 }; // Handle to SELF
HMODULE hInjectorDLL = { 0 }; // Handle to Special K

volatile DWORD        __TBF_TLS_INDEX   = MAXDWORD;

SKX_SetPluginName_pfn SKX_SetPluginName = nullptr;


// Since changing aspect ratio involves a trip back to the
//   title screen, keep track of this to inform the user
//     when that is necessary.
float                 original_aspect   = 0.0f;



__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Shutdown (LPVOID* lpReserved)
{
  UNREFERENCED_PARAMETER (lpReserved);

  if (dll_log != nullptr)
  {
    // Do this so that fullscreen exclusive mode does not mess with a prompt that might come up when SGSSAA is setup
    ShowWindow (tbf::RenderFix::hWndDevice, SW_FORCEMINIMIZE | SW_HIDE);

    tbf::SoundFix::Shutdown      ();
    //tbf::FileIO::Shutdown      ();
    //tbf::SteamFix::Shutdown    ();
    tbf::FrameRateFix::Shutdown  ();
  
    TBF_SaveConfig     ();

    // Weird thing to do at shutdown, right? :P This avoids any error popups while the game is in fullscreen exclusive.
    //
    //   Only change the driver profile if settings change in-game or
    //     if a non-off mode is selected.
    if ( config.render.nv.sgssaa_mode != 0 ||
         TBF_NV_DriverProfileChanged () )
      tbf::RenderFix::InstallSGSSAA ();

    tbf::RenderFix::Shutdown      ();
    //tbf::KeyboardFix::Shutdown  ();

    TBF_UnInit_MinHook ();
  
  
    dll_log->LogEx ( false, L"=========== (Version: v %s) "
                            L"===========\n",
                              TBF_VER_STR.c_str () );
    dll_log->LogEx ( true,  L"End TBFix Plug-In\n" );
    dll_log->LogEx ( false, L"------- [Tales of Berseria  \"Fix\"] "
                            L"-------\n" );
  
    dll_log->close ();
  }

  return TRUE;
}

__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Init (HMODULE hModSpecialK)
{
  hInjectorDLL = hModSpecialK;

  wchar_t wszSKFileName [ MAX_PATH + 2] = { L'\0' };
          wszSKFileName [   MAX_PATH  ] =   L'\0';

  GetModuleFileName (hInjectorDLL, wszSKFileName, MAX_PATH - 1);

  injector_dll = wszSKFileName;

  std::wstring plugin_name = L"Tales of Berseria \"Fix\" v " + TBF_VER_STR;

  dll_log = TBF_CreateLog (L"logs/tbfix.log");

  dll_log->LogEx ( false, L"------- [Tales of Berseria  \"Fix\"] "
                          L"-------\n" );
  dll_log->Log   (        L"tbfix.dll Plug-In\n"
                          L"=========== (Version: v %s) "
                          L"===========",
                            TBF_VER_STR.c_str () );

  if (! TBF_LoadConfig ())
  {
    config.audio.channels            =  6;
    config.audio.sample_hz           = -1;
    config.audio.compatibility       = false;
    config.audio.enable_fix          = true;

    config.file_io.capture           = false;

    config.steam.allow_broadcasts    = false;

    config.framerate.replace_limiter = true;
    config.framerate.target          = 20;

    config.render.aspect_ratio       = 1.777778f;
    config.render.fovy               = 0.785398f;

    config.render.postproc_ratio     =  0.0f;
    config.render.shadow_rescale     = -2;
    config.render.env_shadow_rescale =  1;

    config.textures.remaster         = true;
    config.textures.cache            = true;
    config.textures.dump             = false;

    config.system.injector           = injector_dll;

    // Save a new config if none exists
    TBF_LoadConfig (L"tbfix_default");
    TBF_SaveConfig ();
  }

  original_aspect = config.render.aspect_ratio;

  config.system.injector             = injector_dll;

  SKX_SetPluginName = 
    TBF_ImportFunctionFromSpecialK (
      "SKX_SetPluginName",
        SKX_SetPluginName
    );

  //
  // If this is NULL, the injector system isn't working right!!!
  //
  if (SKX_SetPluginName != nullptr)
    SKX_SetPluginName (plugin_name.c_str ());

  static SK_UpdateSoftware_pfn SK_UpdateSoftware =
    TBF_ImportFunctionFromSpecialK ( "SK_UpdateSoftware",
                                       SK_UpdateSoftware );

  static SK_FetchVersionInfo_pfn SK_FetchVersionInfo =
    TBF_ImportFunctionFromSpecialK ( "SK_FetchVersionInfo",
                                       SK_FetchVersionInfo );


  if (! wcsstr (injector_dll.c_str (), L"SpecialK"))
  {
    if ( SK_FetchVersionInfo != nullptr &&
         SK_UpdateSoftware   != nullptr )
    {
      if (SK_FetchVersionInfo (L"TBF"))
      {
        SK_UpdateSoftware     (L"TBF");
      }
    }
  }

  if (TBF_Init_MinHook () == MH_OK) {
    extern void
    TBF_InitSDLOverride (void);

    TBF_InitSDLOverride ();

    extern void TBFix_ImGui_Init (void);
                TBFix_ImGui_Init ();

    bool success = 
      SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED));

    tbf::SoundFix::Init   ();
    //tbf::FileIO::Init   ();
    //tbf::SteamFix::Init ();
    tbf::RenderFix::Init  ();

    //
    // Hacky way to force off options in Special K that are known not to work well with this game
    //
    SK_GetCommandProcessor ()->ProcessCommandLine ("Window.Center     false");

    TBF_ApplyQueuedHooks ();


    CreateThread ( nullptr, 0,
      [](LPVOID user) ->
        DWORD {
          // Wait for Denuvo to finish its thing, once we have a render window we know we're in the clear.
          while (tbf::RenderFix::hWndDevice == nullptr)
          {
            Sleep (150UL);
          }

          tbf::FrameRateFix::Init ();

          if (tbf::RenderFix::fullscreen && tbf::RenderFix::hWndDevice)
          {
            //
            // Fix input problems in game (namely, ESC key doesn't register unti Alt+Tab).
            //
            SendMessage (tbf::RenderFix::hWndDevice, WM_ACTIVATE, MAKEWPARAM (WA_INACTIVE, 0), (LPARAM)(HWND (0)));
            SendMessage (tbf::RenderFix::hWndDevice, WM_ACTIVATE, MAKEWPARAM (WA_ACTIVE,   0), (LPARAM)(HWND (0)));
          }
    
          CloseHandle             (GetCurrentThread ());
    
          return 0;
        },
      nullptr, 0x00, nullptr );


    if (success)
      CoUninitialize ();
  }

  return TRUE;
}

BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD   ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      hDLLMod = hModule;

      __TBF_TLS_INDEX = TlsAlloc ();

      if (__TBF_TLS_INDEX == TLS_OUT_OF_INDEXES)
      {
        MessageBox ( NULL,
                       L"Out of TLS Indexes",
                         L"Cannot Init. TBFix",
                           MB_ICONERROR | MB_OK | MB_APPLMODAL );
        return FALSE;
      }

    } break;


    case DLL_THREAD_ATTACH:
    {
      LPVOID lpvData =
        (LPVOID)LocalAlloc (LPTR, sizeof TBF_TLS);

      if (lpvData != nullptr)
      {
        if (! TlsSetValue (__TBF_TLS_INDEX, lpvData))
          LocalFree (lpvData);
      }
    } break;

    case DLL_THREAD_DETACH:
    {
        LPVOID lpvData =
          (LPVOID)TlsGetValue (__TBF_TLS_INDEX);

        if (lpvData != nullptr)
        {
          LocalFree   (lpvData);
          TlsSetValue (__TBF_TLS_INDEX, nullptr);
        }
    } break;


    case DLL_PROCESS_DETACH:
    {
        TlsFree (__TBF_TLS_INDEX);
    } break;
  }

  return TRUE;
}