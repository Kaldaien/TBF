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
  iSK_INI*   dll_ini      = nullptr;
static
  iSK_INI*   gamepad_ini  = nullptr;
static
  iSK_INI*   keyboard_ini = nullptr;
static
  iSK_INI*   render_ini   = nullptr;

tbf_config_t config;

tbf::ParameterFactory g_ParameterFactory;

struct {
  tbf::ParameterInt*     sample_rate;
  tbf::ParameterInt*     channels;
  tbf::ParameterBool*    compatibility;
  tbf::ParameterBool*    enable_fix;
} audio;

struct {
  tbf::ParameterBool*    replace_limiter;
  tbf::ParameterFloat*   tolerance;
  tbf::ParameterInt64*   limit_addr;
} framerate;

struct {
  tbf::ParameterInt*     fovy_addr;
  tbf::ParameterInt*     aspect_addr;
  tbf::ParameterFloat*   fovy;
  tbf::ParameterFloat*   aspect_ratio;
  tbf::ParameterBool*    aspect_correct_vids;
  tbf::ParameterBool*    aspect_correction;
  tbf::ParameterFloat*   postproc_ratio;
  tbf::ParameterBool*    clear_blackbars;

  tbf::ParameterInt*     rescale_shadows;
  tbf::ParameterInt*     rescale_env_shadows;

  tbf::ParameterBool*    dump_shaders;
} render;

struct {
  tbf::ParameterBool*    remaster;
  tbf::ParameterBool*    uncompressed;
  tbf::ParameterFloat*   lod_bias;
  tbf::ParameterBool*    cache;
  tbf::ParameterBool*    dump;
  tbf::ParameterInt*     cache_size;
  tbf::ParameterInt*     worker_threads;
  tbf::ParameterBool*    show_loading_text;
  tbf::ParameterBool*    quick_load;
} textures;


struct {
  tbf::ParameterBool*    allow_broadcasts;
} steam;


struct {
  tbf::ParameterStringW* swap_keys;
  tbf::ParameterBool*    swap_wasd;
} keyboard;


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
  wchar_t wszFullName     [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszPadName      [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszRenderName   [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszKeyboardName [ MAX_PATH + 2 ] = { L'\0' };

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

  lstrcatW (wszKeyboardName,  SK_GetConfigPath ());
  lstrcatW (wszKeyboardName, L"TBFix_Keyboard.ini");
  keyboard_ini = TBF_CreateINI (wszKeyboardName);

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
    render_ini,
      L"Texture.System",
        L"Cache" );

  textures.dump =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Dump Textures as they are loaded")
      );
  textures.dump->register_to_ini (
    render_ini,
      L"Texture.System",
        L"Dump" );

  textures.remaster =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Various Fixes to Eliminate Texture Aliasing")
      );
  textures.remaster->register_to_ini (
    render_ini,
      L"Texture.System",
        L"Remaster" );

  textures.uncompressed =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Do Not Re-Compress Remastered Textures")
      );
  textures.uncompressed->register_to_ini (
    render_ini,
      L"Texture.System",
        L"UncompressedRemasters" );

  textures.lod_bias =
    static_cast <tbf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Texture LOD Bias")
      );
  textures.lod_bias->register_to_ini (
    render_ini,
      L"Texture.System",
        L"LODBias" );

  textures.cache_size = 
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Size of Texture Cache")
      );
  textures.cache_size->register_to_ini (
    render_ini,
      L"Texture.System",
        L"MaxCacheInMiB" );

  textures.worker_threads = 
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Number of Worker Threads")
      );
  textures.worker_threads->register_to_ini (
    render_ini,
      L"Texture.System",
        L"WorkerThreads" );

  textures.show_loading_text =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show a loading indicator in OSD")
      );
  textures.show_loading_text->register_to_ini (
    render_ini,
      L"Texture.System",
        L"ShowLoadingIndicator" );

  textures.quick_load = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Skip Mips")
      );
  textures.quick_load->register_to_ini (
    render_ini,
      L"Texture.System",
        L"QuickLoad" );


  render.dump_shaders = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Dump D3D9 Shaders")
      );
  render.dump_shaders->register_to_ini (
    render_ini,
      L"Shader.System",
        L"Dump" );


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

  framerate.replace_limiter =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Replace Namco's Framerate Limiter")
      );
  framerate.replace_limiter->register_to_ini (
    render_ini,
      L"Framerate.Fix",
        L"ReplaceLimiter" );

  framerate.tolerance =
    static_cast <tbf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Replace Namco's Framerate Limiter")
      );
  framerate.tolerance->register_to_ini (
    render_ini,
      L"Framerate.Fix",
        L"LimiterTolerance" );

  framerate.limit_addr =
    static_cast <tbf::ParameterInt64 *>
      (g_ParameterFactory.create_parameter <int64_t> (
        L"User-Selected Framerate Limit Address")
      );
  framerate.limit_addr->register_to_ini (
    dll_ini,
       L"TBFIX.System",
         L"LimitAddress" );


  input.gamepad.texture_set =
    static_cast <tbf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Gamepad Type")
      );
  input.gamepad.texture_set->register_to_ini (
    gamepad_ini,
      L"Gamepad.Type",
        L"TextureSet" );


  keyboard.swap_wasd =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
         L"Swap the WASD and Arrow Keys")
    );
  keyboard.swap_wasd->register_to_ini (
    keyboard_ini,
      L"Keyboard.Remap",
        L"SwapArrowsWithWASD" );


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
  //sys.intro_video->load (config.system.intro_video);
  sys.injector->load    (config.system.injector);

  if (gamepad_ini->get_sections ().empty () || render_ini->get_sections ().empty () || keyboard_ini->get_sections ().empty ())
  {
    TBF_SaveConfig (name, false);

    gamepad_ini->parse  ();
    render_ini->parse   ();
    keyboard_ini->parse ();
  }

  input.gamepad.texture_set->load  (config.input.gamepad.texture_set);
  keyboard.swap_wasd->load         (config.keyboard.swap_wasd);

  render.rescale_shadows->load     (config.render.shadow_rescale);
  render.rescale_env_shadows->load (config.render.env_shadow_rescale);

  render.dump_shaders->load        (config.render.dump_shaders);

  framerate.replace_limiter->load  (config.framerate.replace_limiter);
  framerate.tolerance->load        (config.framerate.tolerance);
  framerate.limit_addr->load       (config.framerate.limit_addr);

  textures.remaster->load          (config.textures.remaster);
  textures.uncompressed->load      (config.textures.uncompressed);
  textures.lod_bias->load          (config.textures.lod_bias);
  textures.cache->load             (config.textures.cache);
  textures.dump->load              (config.textures.dump);
  textures.cache_size->load        (config.textures.max_cache_in_mib);
  textures.worker_threads->load    (config.textures.worker_threads);
  textures.show_loading_text->load (config.textures.show_loading_text);
  textures.quick_load->load        (config.textures.quick_load);

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

  framerate.replace_limiter->store  (config.framerate.replace_limiter);
  framerate.tolerance->store        (config.framerate.tolerance);
  framerate.limit_addr->store       (config.framerate.limit_addr);

  render.dump_shaders->store        (config.render.dump_shaders);

  render.rescale_shadows->store     (config.render.shadow_rescale);
  render.rescale_env_shadows->store (config.render.env_shadow_rescale);

  textures.remaster->store          (config.textures.remaster);
  textures.uncompressed->store      (config.textures.uncompressed);
  textures.lod_bias->store          (config.textures.lod_bias);
  textures.cache->store             (config.textures.cache);
  textures.dump->store              (config.textures.dump);
  textures.cache_size->store        (config.textures.max_cache_in_mib);
  textures.worker_threads->store    (config.textures.worker_threads);
  textures.show_loading_text->store (config.textures.show_loading_text);
  textures.quick_load->store        (config.textures.quick_load);

  input.gamepad.texture_set->store (config.input.gamepad.texture_set);

  keyboard.swap_wasd->store        (config.keyboard.swap_wasd);

  sys.version->store             (TBF_VER_STR);
  //sys.intro_video->store         (config.system.intro_video);
  sys.injector->store            (config.system.injector);

  wchar_t wszFullName     [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszPadName      [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszRenderName   [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszKeyboardName [ MAX_PATH + 2 ] = { L'\0' };

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

  lstrcatW ( wszKeyboardName,
               SK_GetConfigPath () );
  lstrcatW ( wszKeyboardName,
               L"TBFix_Keyboard.ini" );

  keyboard_ini->write (wszKeyboardName);

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

    if (keyboard_ini != nullptr) {
      delete keyboard_ini;
      keyboard_ini = nullptr;
    }
  }
}