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

struct tbf_config_t
{
  struct {
    uint32_t sample_hz        = 48000;
    uint32_t channels         = 6; // OBSOLETE
    bool     compatibility    = false;
    bool     enable_fix       = true;
  } audio;

  struct {
    bool  yield_processor      = true;
    bool  allow_fake_sleep     = false;
    bool  minimize_latency     = false;
    DWORD speedresetcode_addr  = 0x0046C0F9; //0x0046C529;
    DWORD speedresetcode2_addr = 0x0056EB41; //0x0056E441;  0x217B464
    DWORD speedresetcode3_addr = 0x0056E03F; //0x0056D93F;
    DWORD limiter_branch_addr  = 0x00990F53; //0x00990873;
    bool  disable_limiter      = true;
    bool  auto_adjust          = false;
    int   target               = 60;
    int   battle_target        = 60;
    bool  battle_adaptive      = false;
    int   cutscene_target      = 30;
    bool  reshade_fix          = false;
  } framerate;

  struct {
    bool capture            = false;
  } file_io;

  struct {
    bool allow_broadcasts   = false;
  } steam;

  struct {
    bool fix_priest         = true;
  } lua;

  struct {
    float     fovy               = 0.785398f;
    float     aspect_ratio       = 1.777778f;
    uintptr_t aspect_addr        = 0x00D56494ULL;//0x00D52398;
    uintptr_t fovy_addr          = 0x00D56498ULL;//0x00D5239C;
    bool      blackbar_videos    = false;  // OBSOLETE
    bool      aspect_correction  = false;
    int32_t   shadow_rescale     = -2;
    float     postproc_ratio     =  0.0f;
    bool      clear_blackbars    = false;
    int32_t   env_shadow_rescale = 0;
  } render;

  struct {
    bool     dump                = false;
    bool     remaster            = true;
    bool     cache               = true;
    int32_t  max_cache_in_mib    = 2048L;
    int32_t  worker_threads      = 6;
  } textures;

  struct {
    struct {
      std::wstring texture_set   = L"XboxOne";
    } gamepad;
  } input;

  struct {
    std::wstring swap_keys = L"";
  } keyboard;

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