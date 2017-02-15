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
    float     fovy               = 0.785398f;
    float     aspect_ratio       = 1.777778f;
    uintptr_t aspect_addr        = 0x00D56494ULL;//0x00D52398;
    uintptr_t fovy_addr          = 0x00D56498ULL;//0x00D5239C;
    bool      blackbar_videos    = false;  // OBSOLETE
    bool      aspect_correction  = false;
    float     postproc_ratio     =  0.5f;
    bool      clear_blackbars    = false;
    int32_t   shadow_rescale     = -2;
    int32_t   env_shadow_rescale =  1;
    bool      dump_shaders       = false;
    bool      fix_map_res        = false;
    bool      osd_disclaimer     = true;
    bool      auto_apply_changes = false;
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
    bool     highlight_debug_tex =  true;
  } textures;

  struct {
    bool     hollow_eye_mode     =  false;
    uint32_t hollow_eye_vs_crc32 =  0xf063a9f3;
    bool     disable_smoke       =  false;
    uint32_t smoke_ps_crc32      =  0x868bd533;
    bool     disable_pause_dim   =  false;
    uint32_t pause_dim_ps_crc32  =  0x975d2194;
  } fun_stuff;

  struct {
    struct {
      std::wstring texture_set         = L"XboxOne";
      int          virtual_controllers = 0;
    } gamepad;

    struct {
      bool         visible       = false;
      bool         pause         = true;
      float        scale         = 1.0f;
    } ui;
  } input;

  struct {
    std::wstring swap_keys   = L"";
    bool         swap_wasd   = false;

    keybind_s hudless     { L"Ctrl+Shift+F10",       { true,  true,  false }, VK_F10  },
              screenshot  { L"F10",                  { false, false, false }, VK_F10  },
              config_menu { L"Ctrl+Shift+Backspace", { true,  true,  false }, VK_BACK };

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

#endif /* __TBF__CONFIG_H__ */