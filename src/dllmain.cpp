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
#include "scanner.h"

#include "command.h"

#include "hook.h"

#include <process.h>

#pragma comment (lib, "kernel32.lib")

HMODULE hDLLMod      = { 0 }; // Handle to SELF
HMODULE hInjectorDLL = { 0 }; // Handle to Special K

typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)(const wchar_t* wszProduct);
typedef bool    (__stdcall *SK_FetchVersionInfo_pfn)(const wchar_t* wszProduct);

std::wstring injector_dll;

typedef void (__stdcall *SK_SetPluginName_pfn)(std::wstring name);
SK_SetPluginName_pfn SK_SetPluginName = nullptr;

unsigned int
WINAPI
DllThread (LPVOID user)
{
  std::wstring plugin_name = L"Tales of Berseria \"Fix\" v " + TBF_VER_STR;

  dll_log = TBF_CreateLog (L"logs/tbfix.log");

  dll_log->LogEx ( false, L"------- [Tales of Berseria  \"Fix\"] "
                          L"-------\n" );
  dll_log->Log   (        L"tbfix.dll Plug-In\n"
                          L"=========== (Version: v %s) "
                          L"===========",
                            TBF_VER_STR.c_str () );

  if (! TBF_LoadConfig ()) {
    config.audio.channels                 = 6;
    config.audio.sample_hz                = 48000;
    config.audio.compatibility            = false;
    config.audio.enable_fix               = true;

    config.file_io.capture                = false;

    config.steam.allow_broadcasts         = false;

    config.lua.fix_priest                 = true;

    config.render.aspect_ratio            = 1.777778f;
    config.render.fovy                    = 0.785398f;

    config.render.postproc_ratio          =  1.0f;
    config.render.shadow_rescale          = -2;
    config.render.env_shadow_rescale      =  0;

    config.textures.remaster              = true;
    config.textures.dump                  = false;
    config.textures.cache                 = true;

    config.system.injector = injector_dll;

    // Save a new config if none exists
    TBF_SaveConfig ();
  }

  config.system.injector = injector_dll;

  SK_SetPluginName = 
    (SK_SetPluginName_pfn)
      GetProcAddress (hInjectorDLL, "SK_SetPluginName");
  SK_GetCommandProcessor =
    (SK_GetCommandProcessor_pfn)
      GetProcAddress (hInjectorDLL, "SK_GetCommandProcessor");

  //
  // If this is NULL, the injector system isn't working right!!!
  //
  if (SK_SetPluginName != nullptr)
    SK_SetPluginName (plugin_name);

  if (TBF_Init_MinHook () == MH_OK) {
    CoInitializeEx (nullptr, COINIT_MULTITHREADED);

    tbf::SoundFix::Init     ();
    //tbf::FileIO::Init       ();
    //tbf::SteamFix::Init     ();
    tbf::RenderFix::Init    ();
    //tbf::FrameRateFix::Init ();
    //tbf::KeyboardFix::Init  ();

    // Uncomment this when spawning a thread
    //CoUninitialize ();
  }

  SK_UpdateSoftware_pfn SK_UpdateSoftware =
    (SK_UpdateSoftware_pfn)
      GetProcAddress ( hInjectorDLL,
                         "SK_UpdateSoftware" );

  SK_FetchVersionInfo_pfn SK_FetchVersionInfo =
    (SK_FetchVersionInfo_pfn)
      GetProcAddress ( hInjectorDLL,
                         "SK_FetchVersionInfo" );

  if (! wcsstr (injector_dll.c_str (), L"SpecialK")) {
    if ( SK_FetchVersionInfo != nullptr &&
         SK_UpdateSoftware   != nullptr ) {
      if (SK_FetchVersionInfo (L"TBF")) {
        SK_UpdateSoftware (L"TBF");
      }
    }
  }

  return 0;
}

__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Init (HMODULE hModSpecialK)
{
  wchar_t wszSKFileName [ MAX_PATH + 2] = { L'\0' };
          wszSKFileName [   MAX_PATH  ] =   L'\0';

  GetModuleFileName (hModSpecialK, wszSKFileName, MAX_PATH - 1);

  injector_dll = wszSKFileName;

  hInjectorDLL = hModSpecialK;

#if 1
  DllThread (nullptr);
#else
  _beginthreadex ( nullptr, 0, DllThread, nullptr, 0x00, nullptr );
#endif

  return TRUE;
}

BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD    ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      hDLLMod = hModule;
    } break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
    {
      if (dll_log != nullptr) {
        tbf::SoundFix::Shutdown     ();
        //tbf::FileIO::Shutdown       ();
        //tbf::SteamFix::Shutdown     ();
        tbf::RenderFix::Shutdown    ();
        //tbf::FrameRateFix::Shutdown ();
        //tbf::KeyboardFix::Shutdown  ();

        TBF_UnInit_MinHook ();
        TBF_SaveConfig     ();


        dll_log->LogEx ( false, L"=========== (Version: v %s) "
                                L"===========\n",
                                  TBF_VER_STR.c_str () );
        dll_log->LogEx ( true,  L"End TBFix Plug-In\n" );
        dll_log->LogEx ( false, L"------- [Tales of Berseria  \"Fix\"] "
                                L"-------\n" );

        dll_log->close ();
      }
    } break;
  }

  return TRUE;
}