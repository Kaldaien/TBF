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

#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"

#include "DLL_VERSION.H"

extern HMODULE hInjectorDLL;

std::wstring TBF_VER_STR = TBF_VERSION_STR_W;
std::wstring DEFAULT_BK2 = L"RAW\\MOVIE\\AM_TOZ_OP_001.BK2";

static
  iSK_INI*   dll_ini       = nullptr;
tbf_config_t config;

tbf::ParameterFactory g_ParameterFactory;

struct {
  tbf::ParameterInt*     sample_rate;
  tbf::ParameterInt*     channels;
  tbf::ParameterBool*    compatibility;
  tbf::ParameterBool*    enable_fix;
} audio;

struct {
  tbf::ParameterBool*    allow_fake_sleep;
  tbf::ParameterBool*    yield_processor;
  tbf::ParameterBool*    minimize_latency;
  tbf::ParameterInt*     speedresetcode_addr;
  tbf::ParameterInt*     speedresetcode2_addr;
  tbf::ParameterInt*     speedresetcode3_addr;
  tbf::ParameterInt*     limiter_branch_addr;
  tbf::ParameterBool*    disable_limiter;
  tbf::ParameterBool*    auto_adjust;
  tbf::ParameterInt*     target;
  tbf::ParameterInt*     battle_target;
  tbf::ParameterBool*    battle_adaptive;
  tbf::ParameterInt*     cutscene_target;
} framerate;

struct {
  tbf::ParameterInt*     fovy_addr;
  tbf::ParameterInt*     aspect_addr;
  tbf::ParameterFloat*   fovy;
  tbf::ParameterFloat*   aspect_ratio;
  tbf::ParameterBool*    aspect_correct_vids;
  tbf::ParameterBool*    aspect_correction;
  tbf::ParameterInt*     rescale_shadows;
  tbf::ParameterInt*     rescale_env_shadows;
  tbf::ParameterFloat*   postproc_ratio;
  tbf::ParameterBool*    clear_blackbars;
} render;

struct {
  tbf::ParameterBool*    remaster;
  tbf::ParameterBool*    cache;
  tbf::ParameterBool*    dump;
  tbf::ParameterInt*     cache_size;
  tbf::ParameterInt*     worker_threads;
} textures;


struct {
  tbf::ParameterBool*    allow_broadcasts;
} steam;


struct {
  tbf::ParameterStringW* swap_keys;
} keyboard;


struct {
  tbf::ParameterBool*    fix_priest;
} lua;

struct {
  tbf::ParameterStringW* intro_video;
  tbf::ParameterStringW* version;
  tbf::ParameterStringW* injector;
} sys;

typedef const wchar_t* (__stdcall *SK_GetConfigPath_pfn)(void);
static SK_GetConfigPath_pfn SK_GetConfigPath = nullptr;

bool
TBF_LoadConfig (std::wstring name)
{
  SK_GetConfigPath =
    (SK_GetConfigPath_pfn)
      GetProcAddress (
        hInjectorDLL,
          "SK_GetConfigPath"
      );

  // Load INI File
  wchar_t wszFullName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");
  dll_ini = TBF_CreateINI (wszFullName);


  bool empty = dll_ini->get_sections ().empty ();

  //
  // Create Parameters
  //

  audio.channels =
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Audio Channels")
      );
  audio.channels->register_to_ini (
    dll_ini,
      L"TBFIX.Audio",
        L"Channels" );

  audio.sample_rate =
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Sample Rate")
      );
  audio.sample_rate->register_to_ini (
    dll_ini,
      L"TBFIX.Audio",
        L"SampleRate" );

  audio.compatibility =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Compatibility Mode")
      );
  audio.compatibility->register_to_ini (
    dll_ini,
      L"TBFIX.Audio",
        L"CompatibilityMode" );

  audio.enable_fix =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable Fix")
      );
  audio.enable_fix->register_to_ini (
    dll_ini,
      L"TBFIX.Audio",
        L"EnableFix" );


  sys.version =
    static_cast <tbf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Software Version")
      );
  sys.version->register_to_ini (
    dll_ini,
      L"TBFIX.System",
        L"Version" );

  sys.intro_video =
    static_cast <tbf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Intro Video")
      );
  sys.intro_video->register_to_ini (
    dll_ini,
      L"TBFIX.System",
        L"IntroVideo" );

  sys.injector =
    static_cast <tbf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"DLL That Injected Us")
      );
  sys.injector->register_to_ini (
    dll_ini,
      L"TBFIX.System",
        L"Injector" );

  //
  // Load Parameters
  //
  audio.channels->load      ( (int &)config.audio.channels      );
  audio.sample_rate->load   ( (int &)config.audio.sample_hz     );
  audio.compatibility->load (        config.audio.compatibility );
  audio.enable_fix->load    (        config.audio.enable_fix    );

  sys.version->load     (config.system.version);
  sys.intro_video->load (config.system.intro_video);
  sys.injector->load    (config.system.injector);

  if (empty)
    return false;

  return true;
}

void
TBF_SaveConfig (std::wstring name, bool close_config)
{
  audio.channels->store      (config.audio.channels);
  audio.sample_rate->store   (config.audio.sample_hz);
  audio.compatibility->store (config.audio.compatibility);
  audio.enable_fix->store    (config.audio.enable_fix);

  sys.version->store             (TBF_VER_STR);
  sys.intro_video->store         (config.system.intro_video);
  sys.injector->store            (config.system.injector);

  wchar_t wszFullName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW ( wszFullName,
               SK_GetConfigPath () );
  lstrcatW ( wszFullName,
               name.c_str () );
  lstrcatW ( wszFullName,
               L".ini" );

  dll_ini->write (wszFullName);

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }
  }
}