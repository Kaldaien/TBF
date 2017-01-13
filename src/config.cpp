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
  iSK_INI*   dll_ini     = nullptr;
static
  iSK_INI*   gamepad_ini = nullptr;
static
  iSK_INI*   render_ini  = nullptr;

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
  struct {
    tbf::ParameterStringW* texture_set;
  } gamepad;
} input;

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
  wchar_t wszFullName   [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszPadName    [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszRenderName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");
  dll_ini = TBF_CreateINI (wszFullName);

  lstrcatW (wszPadName,  SK_GetConfigPath ());
  lstrcatW (wszPadName, L"TBFix_Gamepad.ini");
  gamepad_ini = TBF_CreateINI (wszPadName);

  lstrcatW (wszRenderName,  SK_GetConfigPath ());
  lstrcatW (wszRenderName, L"TBFix_Render.ini");
  render_ini = TBF_CreateINI (wszRenderName);

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


  textures.cache =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Cache Textures to Speed Up Menus (and Remaster)")
      );
  textures.cache->register_to_ini (
    dll_ini,
      L"TBFIX.Textures",
        L"Cache" );

  textures.dump =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Dump Textures as they are loaded")
      );
  textures.dump->register_to_ini (
    dll_ini,
      L"TBFIX.Textures",
        L"Dump" );

  textures.remaster =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Various Fixes to Eliminate Texture Aliasing")
      );
  textures.remaster->register_to_ini (
    dll_ini,
      L"TBFIX.Textures",
        L"Remaster" );

  textures.cache_size = 
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Size of Texture Cache")
      );
  textures.cache_size->register_to_ini (
    dll_ini,
      L"TBFIX.Textures",
        L"MaxCacheInMiB" );

  textures.worker_threads = 
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Number of Worker Threads")
      );
  textures.worker_threads->register_to_ini (
    dll_ini,
      L"TBFIX.Textures",
        L"WorkerThreads" );


  render.rescale_shadows =
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Shadow Rescale Factor")
      );
  render.rescale_shadows->register_to_ini (
    render_ini,
      L"Shadow.Quality",
        L"RescaleShadows" );

  render.rescale_env_shadows =
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Environmental Shadow Rescale Factor")
      );
  render.rescale_env_shadows->register_to_ini (
    render_ini,
      L"Shadow.Quality",
        L"RescaleEnvShadows" );


  input.gamepad.texture_set =
    static_cast <tbf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Gamepad Type")
      );
  input.gamepad.texture_set->register_to_ini (
    gamepad_ini,
      L"Gamepad.Type",
        L"TextureSet" );


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

  textures.remaster->load       (config.textures.remaster);
  textures.cache->load          (config.textures.cache);
  textures.dump->load           (config.textures.dump);
  textures.cache_size->load     (config.textures.max_cache_in_mib);
  textures.worker_threads->load (config.textures.worker_threads);

  sys.version->load     (config.system.version);
  //sys.intro_video->load (config.system.intro_video);
  sys.injector->load    (config.system.injector);

  if (gamepad_ini->get_sections ().empty () || render_ini->get_sections ().empty ()) {
    TBF_SaveConfig (name, false);

    gamepad_ini->parse ();
    render_ini->parse  ();
  }

  input.gamepad.texture_set->load (config.input.gamepad.texture_set);

  render.rescale_shadows->load     (config.render.shadow_rescale);
  render.rescale_env_shadows->load (config.render.env_shadow_rescale);

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

  render.rescale_shadows->store     (config.render.shadow_rescale);
  render.rescale_env_shadows->store (config.render.env_shadow_rescale);

  textures.remaster->store       (config.textures.remaster);
  textures.cache->store          (config.textures.cache);
  textures.dump->store           (config.textures.dump);
  textures.cache_size->store     (config.textures.max_cache_in_mib);
  textures.worker_threads->store (config.textures.worker_threads);

  input.gamepad.texture_set->store (config.input.gamepad.texture_set);

  sys.version->store             (TBF_VER_STR);
  //sys.intro_video->store         (config.system.intro_video);
  sys.injector->store            (config.system.injector);

  wchar_t wszFullName   [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszPadName    [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszRenderName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW ( wszFullName,
               SK_GetConfigPath () );
  lstrcatW ( wszFullName,
               name.c_str () );
  lstrcatW ( wszFullName,
               L".ini" );

  dll_ini->write (wszFullName);

  lstrcatW ( wszPadName,
               SK_GetConfigPath () );
  lstrcatW ( wszPadName,
               L"TBFix_Gamepad.ini" );

  gamepad_ini->write (wszPadName);

  lstrcatW ( wszRenderName,
               SK_GetConfigPath () );
  lstrcatW ( wszRenderName,
               L"TBFix_Render.ini" );

  render_ini->write (wszRenderName);

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }

    if (gamepad_ini != nullptr) {
      delete gamepad_ini;
      gamepad_ini = nullptr;
    }

    if (render_ini != nullptr) {
      delete render_ini;
      render_ini = nullptr;
    }
  }
}