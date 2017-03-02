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
#ifndef __TBF__CONFIG_H__
#define __TBF__CONFIG_H__

#include <Windows.h>
#include <string>

extern std::wstring TBF_VER_STR;
extern std::wstring DEFAULT_BK2;

struct keybind_s
{
  const char*  bind_name;
  std::wstring human_readable;

  struct {
    bool ctrl,
         shift,
         alt;
  };

  BYTE vKey;

  void parse  (void);
  void update (void);
};

struct tbf_config_t
{
  struct {
    uint32_t sample_hz       = 48000;
    uint32_t channels        =  6;
    bool     compatibility   = true;
    bool     enable_fix      = false;
  } audio;

  struct {
    bool     replace_limiter = true;
    int      target          = 60;
    float    tolerance       = 0.05f;
    bool     reshade_fix     = false;
    int64_t  limit_addr      = 0xBCC3FCULL;
  } framerate;

  struct {
    bool capture             = false;
  } file_io;

  struct {
    bool allow_broadcasts    = false;
  } steam;

  struct {
    float     fovy                = 0.785398f;
    float     aspect_ratio        = 1.777778f;
    uintptr_t aspect_addr         = 0x00D56494ULL;//0x00D52398;
    uintptr_t fovy_addr           = 0x00D56498ULL;//0x00D5239C;
    bool      blackbar_videos     = false;
    bool      aspect_correction   = false;
    float     postproc_ratio      =  0.5f;
    bool      clear_blackbars     =  true;
    int32_t   shadow_rescale      = -2;
    int32_t   env_shadow_rescale  =  1;
    bool      half_float_shadows  = false;
    bool      dump_shaders        = false;
    bool      force_post_mips     = true; // Mipmap generation on post-processing
    bool      fix_map_res         = true;
    bool      high_res_reflection = false;
    bool      high_res_bloom      = false;
    bool      osd_disclaimer      = true;
    bool      auto_apply_changes  = false;

    bool      validation          = false;

    struct {
      int     quality_preset        =     3;
      bool    override_game         = false;

      float   threshold             =  0.2f;
      int     max_search_steps      =    16;
      int     max_search_steps_diag =     8;
      float   corner_rounding       =    25;

      float   predication_threshold = 0.01f;
      float   predication_scale     =     2;
      float   predication_strength  =  0.4f;
      float   reprojection_weight   = 30.0f;

     uint32_t smaa_vs0_crc32        = 0x75967358;
     uint32_t smaa_vs1_crc32        = 0x524c55c7;
     uint32_t smaa_vs2_crc32        = 0x3f685075;
     uint32_t smaa_ps0_crc32        = 0x2ee6cfb6;
     uint32_t smaa_ps1_crc32        = 0x031ff454;

    } smaa;

    struct {
      uint32_t     sgssaa_mode      = 0;
      std::wstring compat_bits      = L"0x084012C5";
    } nv;
  } render;

  struct {
    bool     dump                =  false;
    bool     on_demand_dump      =  false;
    bool     remaster            =  true;
    bool     cache               =  true;
    bool     uncompressed        =  false;
    float    lod_bias            = -0.1333f;
    int32_t  max_cache_in_mib    =  2048L;
    int32_t  worker_threads      =  3;
    int32_t  max_decomp_jobs     =  3;
    bool     show_loading_text   =  false;
    bool     quick_load          =  false;
    bool     clamp_npot_coords   =  true;
    bool     clamp_skit_coords   =  true;
    bool     clamp_map_coords    =  false;
    bool     clamp_text_coords   =  false;
    bool     keep_ui_sharp       =  true;
    bool     highlight_debug_tex =  true;
  } textures;

  struct {
    bool     hollow_eye_mode     =  false;
    uint32_t hollow_eye_vs_crc32 =  0xf063a9f3;
    bool     disable_smoke       =  false;
    uint32_t smoke_ps_crc32      =  0x868bd533;
    bool     disable_pause_dim   =  false;
    uint32_t pause_dim_ps_crc32  =  0x975d2194;
    bool     plastic_mode        =  false;
  } fun_stuff;

  struct {
    struct {
      std::wstring texture_set         = L"XboxOne";
      int          virtual_controllers = 0;
    } gamepad;

    struct {
      bool      visible         = false;
      bool      pause           = true;
      float     scale           = 1.0f;
      bool      never_show_eula = false;
    } ui;
  } input;

  struct {
    std::wstring swap_keys   = L"";
    bool         swap_wasd   = false;

    keybind_s hudless         { "Hudless",        L"Ctrl+Shift+F10",        { true,  true,  false }, VK_F10        },
              screenshot      { "Screenshot",     L"F10",                   { false, false, false }, VK_F10        },
              config_menu     { "CfgMenu",        L"Ctrl+Shift+Backspace",  { true,  true,  false }, VK_BACK       },
              aspect_ratio    { "AspectRatio",    L"Ctrl+Alt+Shift+.",      { true,  true,   true }, VK_OEM_PERIOD },
              clear_enemies   { "ClearEnemies",   L"Ctrl+Shift+F1",         { true,  true,  false }, VK_F1         },
              respawn_enemies { "RespawnEnemies", L"Ctrl+Shift+F2",         { true,  true,  false }, VK_F2         },
              battle_timestop { "BattleTimestop", L"Ctrl+Shift+/",          { true,  true,  false }, 0             };

  } keyboard;

  struct {
    bool keep            = true;
    bool import_to_steam = true;
  } screenshots;

  struct {
    std::wstring
            version         = TBF_VER_STR;
    std::wstring
            intro_video     = DEFAULT_BK2;
    std::wstring
            injector        = L"d3d9.dll";
  } system;
};

extern tbf_config_t config;

bool TBF_LoadConfig (std::wstring name         = L"tbfix");
void TBF_SaveConfig (std::wstring name         = L"tbfix",
                     bool         close_config = false);

bool TBF_NV_DriverProfileChanged (void);

#endif /* __TBF__CONFIG_H__ */