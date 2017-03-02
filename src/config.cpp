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

#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"
#include "render.h"

#include "DLL_VERSION.H"

#include <algorithm>

extern HMODULE hInjectorDLL;

std::wstring TBF_VER_STR = TBF_VERSION_STR_W;
std::wstring DEFAULT_BK2 = L"RAW\\MOVIE\\AM_TOZ_OP_001.BK2";

bool         nv_profile_change = false;

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
  tbf::ParameterBool*    aspect_correction;
  tbf::ParameterStringW* aspect_ratio_keybind;
  tbf::ParameterBool*    clear_blackbars;

  tbf::ParameterFloat*   postproc_ratio;
  tbf::ParameterBool*    fix_map_res;
  tbf::ParameterBool*    high_res_bloom;
  tbf::ParameterBool*    high_res_reflection;

  tbf::ParameterInt*     rescale_shadows;
  tbf::ParameterInt*     rescale_env_shadows;
  tbf::ParameterBool*    half_precision_shadows;

  tbf::ParameterBool*    dump_shaders;
  tbf::ParameterBool*    pause_on_ui;
  tbf::ParameterBool*    show_osd_disclaimer;
  tbf::ParameterFloat*   ui_scale;
  tbf::ParameterBool*    auto_apply_changes;
  tbf::ParameterBool*    never_show_eula;

  struct {
    tbf::ParameterInt*   preset;
    tbf::ParameterBool*  override_game;
    tbf::ParameterFloat* threshold;
    tbf::ParameterInt*   max_search_steps;
    tbf::ParameterInt*   max_search_steps_diag;
    tbf::ParameterFloat* corner_rounding;
    tbf::ParameterFloat* predication_threshold;
    tbf::ParameterFloat* predication_scale;
    tbf::ParameterFloat* predication_strength;
    tbf::ParameterFloat* reprojection_weight;
  } smaa;

  tbf::ParameterStringW* nvidia_sgssaa;
  tbf::ParameterStringW* nvidia_aa_compat_bits;
} render;

struct {
  tbf::ParameterBool*    keep;
  tbf::ParameterBool*    add_to_steam;
  tbf::ParameterStringW* hudless_keybind;
  tbf::ParameterStringW* regular_keybind;
} screenshots;

struct {
  tbf::ParameterBool*    remaster;
  tbf::ParameterBool*    uncompressed;
  tbf::ParameterFloat*   lod_bias;
  tbf::ParameterBool*    cache;
  tbf::ParameterBool*    dump;
  tbf::ParameterBool*    dump_on_demand;
  tbf::ParameterInt*     cache_size;
  tbf::ParameterInt*     worker_threads;
  tbf::ParameterBool*    show_loading_text;
  tbf::ParameterBool*    quick_load;
  tbf::ParameterBool*    clamp_npot_coords;
  tbf::ParameterBool*    clamp_skit_coords;
  tbf::ParameterBool*    clamp_map_coords;
  tbf::ParameterBool*    clamp_text_coords;
  tbf::ParameterBool*    sharpen_ui;
} textures;


struct {
  tbf::ParameterBool*    allow_broadcasts;
} steam;


struct {
  tbf::ParameterStringW* swap_keys;
  tbf::ParameterBool*    swap_wasd;

  tbf::ParameterStringW* clear_enemies;
  tbf::ParameterStringW* respawn_enemies;
  tbf::ParameterStringW* battle_timestop;
} keyboard;


struct {
  struct {
    tbf::ParameterStringW* texture_set;
    tbf::ParameterInt*     virtual_controllers;
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

  textures.dump_on_demand =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Dump Textures on Demand")
      );
  textures.dump_on_demand->register_to_ini (
    render_ini,
      L"Texture.System",
        L"DumpOnDemand" );

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

  textures.clamp_npot_coords = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Clamp Coordinates of NPOT Textures (cannot be filtered correctly)")
      );
  textures.clamp_npot_coords->register_to_ini (
    render_ini,
      L"Texture.System",
        L"ClampNonPowerOf2Coords" );

  textures.clamp_skit_coords = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Clamp Coordinates of Skit Textures")
      );
  textures.clamp_skit_coords->register_to_ini (
    render_ini,
      L"Texture.System",
        L"ClampSkitCoords" );

  textures.clamp_map_coords = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Clamp Coordinates of Minimap Textures")
      );
  textures.clamp_map_coords->register_to_ini (
    render_ini,
      L"Texture.System",
        L"ClampMinimapCoords" );

  textures.clamp_text_coords =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Clamp Coordinates of Primar (English) Text")
      );
  textures.clamp_text_coords->register_to_ini (
    render_ini,
      L"Texture.System",
        L"ClampTextCoords" );

  textures.sharpen_ui = 
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Keep UI Textures Sharp")
      );
  textures.sharpen_ui->register_to_ini (
    render_ini,
      L"Texture.System",
        L"SharpenUI" );


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

  render.half_precision_shadows =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Environmental Shadow Precision")
      );
  render.half_precision_shadows->register_to_ini (
    render_ini,
      L"Shadow.Quality",
        L"HalfFloatPrecision" );

  render.fix_map_res =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Fix crazy low-resolution map menu...")
    );
  render.fix_map_res->register_to_ini (
    render_ini,
      L"Resolution.PostProcess",
        L"FixMap" );

  render.high_res_bloom =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Increase Resolution of Bloom Effects")
    );
  render.high_res_bloom->register_to_ini (
    render_ini,
      L"Resolution.PostProcess",
        L"HighResBloom" );

  render.high_res_reflection =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Increase Resolution of Reflections")
    );
  render.high_res_reflection->register_to_ini (
    render_ini,
      L"Resolution.PostProcess",
        L"HighResReflection" );

  render.aspect_ratio =
    static_cast <tbf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Last Known Aspect Ratio")
    );
  render.aspect_ratio->register_to_ini (
    render_ini,
      L"Resolution.AspectRatio",
        L"LastKnownRatio"
  );

  render.aspect_correction =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Apply Aspect Ratio Correction")
    );
  render.aspect_correction->register_to_ini (
    render_ini,
      L"Resolution.AspectRatio",
        L"ApplyCorrection"
  );

  render.clear_blackbars =
    static_cast <tbf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Clear Blackbars (in video)")
    );
  render.clear_blackbars->register_to_ini (
    render_ini,
      L"Resolution.AspectRatio",
        L"ClearBlackbars"
  );

  render.postproc_ratio =
    static_cast <tbf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Post-Process Ratio")
      );
  render.postproc_ratio->register_to_ini (
    render_ini,
      L"Resolution.PostProcess",
        L"DepthOfField" );

  render.pause_on_ui =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Config Menu Pauses Game")
    );

  render.pause_on_ui->register_to_ini (
    render_ini,
      L"ImGui.Settings",
        L"PauseOnActivate" );

  render.ui_scale =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Config Menu Scale Factor")
    );

  render.ui_scale->register_to_ini (
    render_ini,
      L"ImGui.Settings",
        L"FontScale" );

  render.show_osd_disclaimer =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Show the OSD Disclaimer At Startup")
    );

  render.show_osd_disclaimer->register_to_ini (
    render_ini,
      L"ImGui.Settings",
        L"ShowOSDDisclaimer" );

  render.auto_apply_changes =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Automagically Apply Changes to Graphics Settings")
    );

  render.auto_apply_changes->register_to_ini (
    render_ini,
      L"ImGui.Settings",
        L"ApplyChangesImmediately" );

  render.never_show_eula =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Never show the EULA again")
    );

  render.never_show_eula->register_to_ini (
    render_ini,
      L"ImGui.Settings",
        L"UserClaimsToHaveReadEULA" );



  render.smaa.preset =
    static_cast <tbf::ParameterInt *>
    (g_ParameterFactory.create_parameter <int>(
      L"Preset")
    );

  render.smaa.preset->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"QualityPreset"
  );

  render.smaa.override_game =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
      L"Override Game's Settings")
    );

  render.smaa.override_game->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"OverrideGame"
  );

  render.smaa.threshold =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float>(
      L"Threshold")
    );

  render.smaa.threshold->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"Threshold"
  );

  render.smaa.max_search_steps =
    static_cast <tbf::ParameterInt *>
    (g_ParameterFactory.create_parameter <int>(
      L"Max Search Steps")
    );

  render.smaa.max_search_steps->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"MaxSearchSteps"
  );

  render.smaa.max_search_steps_diag =
    static_cast <tbf::ParameterInt *>
    (g_ParameterFactory.create_parameter <int>(
      L"Max Search Steps Diagonal")
    );

  render.smaa.max_search_steps_diag->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"MaxSearchStepsDiagonal"
  );

  render.smaa.corner_rounding =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Corner Rounding %")
    );

  render.smaa.corner_rounding->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"CornerRoundingPercent"
  );

  render.smaa.predication_threshold =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Predication Threshold")
    );

  render.smaa.predication_threshold->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"PredicationThreshold"
  );

  render.smaa.predication_scale =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Predication Scale")
    );

  render.smaa.predication_scale->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"PredicationScale"
  );

  render.smaa.predication_strength =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Predication Strength")
    );

  render.smaa.predication_strength->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"PredicationStrength"
  );

  render.smaa.reprojection_weight =
    static_cast <tbf::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Reprojection Weight")
    );

  render.smaa.reprojection_weight->register_to_ini(
    render_ini,
      L"AntiAliasing.SMAA",
        L"ReprojectionWeight"
  );


  render.nvidia_sgssaa =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring>(
       L"NVAPI Anti-Aliasing Settings")
    );
  render.nvidia_sgssaa->register_to_ini (
    render_ini,
      L"AntiAliasing.NV",
        L"SparseGridSuperSample"
  );

  render.nvidia_aa_compat_bits =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring> (
       L"NVAPI Anti-Aliasing Settings")
    );
  render.nvidia_aa_compat_bits->register_to_ini (
    render_ini,
      L"AntiAliasing.NV",
        L"CompatibilityBits"
  );

  render.aspect_ratio_keybind =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring> (
      L"Aspect Ratio Toggle Keybinding")
    );

  render.aspect_ratio_keybind->register_to_ini (
    render_ini,
      L"Resolution.AspectRatio",
        L"Keybind_Toggle" );



  screenshots.hudless_keybind =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring> (
      L"HUDless Screenshot Keybinding")
    );

  screenshots.hudless_keybind->register_to_ini (
    render_ini,
      L"Screenshot.Capture",
        L"KeybindNoHUD" );

  screenshots.keep =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Keep Original (High Quality) Screenshots")
    );

  screenshots.keep->register_to_ini (
    render_ini,
      L"Screenshot.Capture",
        L"KeepOriginal" );

  screenshots.add_to_steam =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Add Screenshot to Steam Library")
    );

  screenshots.add_to_steam->register_to_ini (
    render_ini,
      L"Screenshot.Capture",
        L"AddToSteamLibrary" );


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

  input.gamepad.virtual_controllers =
    static_cast <tbf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Virtual Gamepad Count")
      );
  input.gamepad.virtual_controllers->register_to_ini (
    gamepad_ini,
      L"Gamepad.Type",
        L"VirtualControllers" );


  keyboard.swap_wasd =
    static_cast <tbf::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(
         L"Swap the WASD and Arrow Keys")
    );
  keyboard.swap_wasd->register_to_ini (
    keyboard_ini,
      L"Keyboard.Remap",
        L"SwapArrowsWithWASD" );

  keyboard.swap_keys =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring>(
         L"List of Keys to Swap")
    );
  keyboard.swap_keys->register_to_ini (
    keyboard_ini,
      L"Keyboard.Remap",
        L"SwapKeys" );

  keyboard.clear_enemies =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring>(
         L"Clear Enemies On Screen")
    );
  keyboard.clear_enemies->register_to_ini (
    keyboard_ini,
      L"Keyboard.Debug",
        L"ClearEnemies" );

  keyboard.respawn_enemies =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring>(
         L"Respawn Enemies On Screen")
    );
  keyboard.respawn_enemies->register_to_ini (
    keyboard_ini,
      L"Keyboard.Debug",
        L"RespawnEnemies" );

  keyboard.battle_timestop =
    static_cast <tbf::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring> (
         L"Pause Battle")
    );
  keyboard.battle_timestop->register_to_ini (
    keyboard_ini,
      L"Keyboard.Debug",
        L"BattleTimestop" );



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

  input.gamepad.texture_set->load         (config.input.gamepad.texture_set);
  input.gamepad.virtual_controllers->load (config.input.gamepad.virtual_controllers);

  keyboard.swap_wasd->load                (config.keyboard.swap_wasd);
  keyboard.swap_keys->load                (config.keyboard.swap_keys);

  render.rescale_shadows->load            (config.render.shadow_rescale);
  render.rescale_env_shadows->load        (config.render.env_shadow_rescale);
  render.half_precision_shadows->load     (config.render.half_float_shadows);

  render.aspect_correction->load          (config.render.aspect_correction);
  render.aspect_ratio->load               (config.render.aspect_ratio);
  render.clear_blackbars->load            (config.render.clear_blackbars);

  render.dump_shaders->load               (config.render.dump_shaders);
  render.fix_map_res->load                (config.render.fix_map_res);
  render.high_res_bloom->load             (config.render.high_res_bloom);
  render.high_res_reflection->load        (config.render.high_res_reflection);
  render.postproc_ratio->load             (config.render.postproc_ratio);
  render.pause_on_ui->load                (config.input.ui.pause);
  render.show_osd_disclaimer->load        (config.render.osd_disclaimer);
  render.auto_apply_changes->load         (config.render.auto_apply_changes);
  render.ui_scale->load                   (config.input.ui.scale);
  render.never_show_eula->load            (config.input.ui.never_show_eula);

  render.smaa.preset->load                (config.render.smaa.quality_preset);
  render.smaa.override_game->load         (config.render.smaa.override_game);
  render.smaa.threshold->load             (config.render.smaa.threshold);
  render.smaa.max_search_steps->load      (config.render.smaa.max_search_steps);
  render.smaa.max_search_steps_diag->load (config.render.smaa.max_search_steps_diag);
  render.smaa.corner_rounding->load       (config.render.smaa.corner_rounding);
  render.smaa.predication_threshold->load (config.render.smaa.predication_threshold);
  render.smaa.predication_scale->load     (config.render.smaa.predication_scale);
  render.smaa.predication_strength->load  (config.render.smaa.predication_strength);
  render.smaa.reprojection_weight->load   (config.render.smaa.reprojection_weight);

  std::wstring sgssaa;

  render.nvidia_sgssaa->load         (sgssaa);
  render.nvidia_aa_compat_bits->load (config.render.nv.compat_bits
);

  if (sgssaa.length ())
  {
    if (sgssaa == L"2x")
      config.render.nv.sgssaa_mode = 1;

    else if (sgssaa == L"4x")
      config.render.nv.sgssaa_mode = 2;

    else if (sgssaa == L"8x")
      config.render.nv.sgssaa_mode = 3;

    else if (sgssaa == L"off")
      config.render.nv.sgssaa_mode = 0;

    // Only change the driver profile if settings change in-game or
    //   if a non-off mode is selected.
    if ( config.render.nv.sgssaa_mode != 0 )
      tbf::RenderFix::InstallSGSSAA ();
  }

  config.input.ui.scale = std::min (std::max (1.0f, config.input.ui.scale), 3.0f);

  framerate.replace_limiter->load  (config.framerate.replace_limiter);
  framerate.tolerance->load        (config.framerate.tolerance);
  //framerate.limit_addr->load     (config.framerate.limit_addr);

  screenshots.add_to_steam->load    (config.screenshots.import_to_steam);
  screenshots.keep->load            (config.screenshots.keep);
  screenshots.hudless_keybind->load (config.keyboard.hudless.human_readable);
  render.aspect_ratio_keybind->load (config.keyboard.aspect_ratio.human_readable);
  keyboard.clear_enemies->load      (config.keyboard.clear_enemies.human_readable);
  keyboard.respawn_enemies->load    (config.keyboard.respawn_enemies.human_readable);
  keyboard.battle_timestop->load    (config.keyboard.battle_timestop.human_readable);

  config.keyboard.hudless.parse         ();
  config.keyboard.aspect_ratio.parse    ();
  config.keyboard.clear_enemies.parse   ();
  config.keyboard.respawn_enemies.parse ();
  config.keyboard.battle_timestop.parse ();

  textures.remaster->load          (config.textures.remaster);
  textures.uncompressed->load      (config.textures.uncompressed);
  textures.lod_bias->load          (config.textures.lod_bias);
  textures.cache->load             (config.textures.cache);
  textures.dump->load              (config.textures.dump);
  textures.dump_on_demand->load    (config.textures.on_demand_dump);
  textures.cache_size->load        (config.textures.max_cache_in_mib);
  textures.worker_threads->load    (config.textures.worker_threads);
  textures.show_loading_text->load (config.textures.show_loading_text);
  textures.quick_load->load        (config.textures.quick_load);
  textures.clamp_npot_coords->load (config.textures.clamp_npot_coords);
  textures.clamp_skit_coords->load (config.textures.clamp_skit_coords);
  textures.clamp_map_coords->load  (config.textures.clamp_map_coords);
  textures.clamp_text_coords->load (config.textures.clamp_text_coords);
  textures.sharpen_ui->load        (config.textures.keep_ui_sharp);

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

  render.aspect_correction->store      (config.render.aspect_correction);
  render.aspect_ratio->store           (config.render.aspect_ratio);
  render.clear_blackbars->store        (config.render.clear_blackbars);

  render.rescale_shadows->store        (config.render.shadow_rescale);
  render.rescale_env_shadows->store    (config.render.env_shadow_rescale);
  render.half_precision_shadows->store (config.render.half_float_shadows);
  render.fix_map_res->store            (config.render.fix_map_res);
  render.high_res_bloom->store         (config.render.high_res_bloom);
  render.high_res_reflection->store    (config.render.high_res_reflection);
  render.postproc_ratio->store         (config.render.postproc_ratio);
  render.pause_on_ui->store            (config.input.ui.pause);
  render.show_osd_disclaimer->store    (config.render.osd_disclaimer);
  render.auto_apply_changes->store     (config.render.auto_apply_changes);
  render.never_show_eula->store        (config.input.ui.never_show_eula);
  render.ui_scale->store               (config.input.ui.scale);

  render.smaa.preset->store                (config.render.smaa.quality_preset);
  render.smaa.override_game->store         (config.render.smaa.override_game);
  render.smaa.threshold->store             (config.render.smaa.threshold);
  render.smaa.max_search_steps->store      (config.render.smaa.max_search_steps);
  render.smaa.max_search_steps_diag->store (config.render.smaa.max_search_steps_diag);
  render.smaa.corner_rounding->store       (config.render.smaa.corner_rounding);
  render.smaa.predication_threshold->store (config.render.smaa.predication_threshold);
  render.smaa.predication_scale->store     (config.render.smaa.predication_scale);
  render.smaa.predication_strength->store  (config.render.smaa.predication_strength);
  render.smaa.reprojection_weight->store   (config.render.smaa.reprojection_weight);

  std::wstring sgssaa_original, sgssaa_now;

  render.nvidia_sgssaa->load          (sgssaa_original);
  render.nvidia_sgssaa->store         ( config.render.nv.sgssaa_mode == 0 ? L"off" :
                                        config.render.nv.sgssaa_mode == 1 ? L"2x"  :
                                        config.render.nv.sgssaa_mode == 2 ? L"4x"  : L"8x" );
  render.nvidia_aa_compat_bits->store ( config.render.nv.compat_bits );
  render.nvidia_sgssaa->load          (sgssaa_now);

  if (sgssaa_original != sgssaa_now)
    nv_profile_change = true;

  textures.remaster->store          (config.textures.remaster);
  textures.uncompressed->store      (config.textures.uncompressed);
  textures.lod_bias->store          (config.textures.lod_bias);
  textures.cache->store             (config.textures.cache);
  textures.dump->store              (config.textures.dump);
  textures.dump_on_demand->store    (config.textures.on_demand_dump);
  textures.cache_size->store        (config.textures.max_cache_in_mib);
  textures.worker_threads->store    (config.textures.worker_threads);
  textures.show_loading_text->store (config.textures.show_loading_text);
  textures.quick_load->store        (config.textures.quick_load);
  textures.clamp_npot_coords->store (config.textures.clamp_npot_coords);
  textures.clamp_skit_coords->store (config.textures.clamp_skit_coords);
  textures.clamp_map_coords->store  (config.textures.clamp_map_coords);
  textures.clamp_text_coords->store (config.textures.clamp_text_coords);
  textures.sharpen_ui->store        (config.textures.keep_ui_sharp);

  input.gamepad.texture_set->store         (config.input.gamepad.texture_set);
  input.gamepad.virtual_controllers->store (config.input.gamepad.virtual_controllers);

  config.keyboard.hudless.update         ();
  config.keyboard.aspect_ratio.update    ();
  config.keyboard.clear_enemies.update   ();
  config.keyboard.respawn_enemies.update ();
  config.keyboard.battle_timestop.update ();

  screenshots.add_to_steam->store    (config.screenshots.import_to_steam);
  screenshots.keep->store            (config.screenshots.keep);
  screenshots.hudless_keybind->store (config.keyboard.hudless.human_readable);
  render.aspect_ratio_keybind->store (config.keyboard.aspect_ratio.human_readable);

  keyboard.swap_wasd->store        (config.keyboard.swap_wasd);
  keyboard.swap_keys->store        (config.keyboard.swap_keys);
  keyboard.clear_enemies->store    (config.keyboard.clear_enemies.human_readable);
  keyboard.respawn_enemies->store  (config.keyboard.respawn_enemies.human_readable);
  keyboard.battle_timestop->store  (config.keyboard.battle_timestop.human_readable);

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



#include <unordered_map>

std::unordered_map <std::wstring, BYTE> humanKeyNameToVirtKeyCode;
std::unordered_map <BYTE, std::wstring> virtKeyCodeToHumanKeyName;

#include <queue>

void
keybind_s::update (void)
{
  human_readable = L"";

  std::wstring key_name = virtKeyCodeToHumanKeyName [vKey];

  if (! key_name.length ())
    return;

  std::queue <std::wstring> words;

  if (ctrl)
    words.push (L"Ctrl");

  if (alt)
    words.push (L"Alt");

  if (shift)
    words.push (L"Shift");

  words.push (key_name);

  while (! words.empty ())
  {
    human_readable += words.front ();
    words.pop ();

    if (! words.empty ())
      human_readable += L"+";
  }
}

void
keybind_s::parse (void)
{
  vKey = 0x00;

  static bool init = false;

  if (! init)
  {
    for (int i = 0; i < 0xFF; i++)
    {
      wchar_t name [32] = { L'\0' };
    
      GetKeyNameText ( (MapVirtualKey (i, MAPVK_VK_TO_VSC) & 0xFF) << 16,
                         name,
                           32 );
    
      if (i != VK_CONTROL && i != VK_MENU && i != VK_SHIFT && i != VK_OEM_PLUS && i != VK_OEM_MINUS)
      {
        humanKeyNameToVirtKeyCode [   name] = (BYTE)i;
        virtKeyCodeToHumanKeyName [(BYTE)i] =    name;
      }
    }
    
    humanKeyNameToVirtKeyCode [L"Plus"]  = VK_OEM_PLUS;
    humanKeyNameToVirtKeyCode [L"Minus"] = VK_OEM_MINUS;
    humanKeyNameToVirtKeyCode [L"Ctrl"]  = VK_CONTROL;
    humanKeyNameToVirtKeyCode [L"Alt"]   = VK_MENU;
    humanKeyNameToVirtKeyCode [L"Shift"] = VK_SHIFT;
    
    virtKeyCodeToHumanKeyName [VK_CONTROL]    = L"Ctrl";
    virtKeyCodeToHumanKeyName [VK_MENU]       = L"Alt";
    virtKeyCodeToHumanKeyName [VK_SHIFT]      = L"Shift";
    virtKeyCodeToHumanKeyName [VK_OEM_PLUS]   = L"Plus";
    virtKeyCodeToHumanKeyName [VK_OEM_MINUS]  = L"Minus";

    init = true;
  }

  wchar_t wszKeyBind [128] = { L'\0' };

  wcsncpy (wszKeyBind, human_readable.c_str (), 128);

  wchar_t* wszBuf;
  wchar_t* wszTok = std::wcstok (wszKeyBind, L"+", &wszBuf);

  ctrl  = false;
  alt   = false;
  shift = false;

  if (wszTok == nullptr)
  {
    vKey  = humanKeyNameToVirtKeyCode [wszKeyBind];
  }

  while (wszTok)
  {
    BYTE vKey_ = humanKeyNameToVirtKeyCode [wszTok];

    if (vKey_ == VK_CONTROL)
      ctrl = true;
    else if (vKey_ == VK_SHIFT)
      shift = true;
    else if (vKey_ == VK_MENU)
      alt = true;
    else
      vKey = vKey_;

    wszTok = std::wcstok (nullptr, L"+", &wszBuf);
  }
}


bool
TBF_NV_DriverProfileChanged (void)
{
  return nv_profile_change;
}