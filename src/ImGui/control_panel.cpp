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

#include <imgui/imgui.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "DLL_VERSION.H"

#include "config.h"
#include "command.h"
#include "render.h"
#include "textures.h"
#include "framerate.h"
#include "sound.h"
#include "hook.h"
#include "input.h"
#include "keyboard.h"


#include <string>
#include <vector>


#include <algorithm>
#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <atlbase.h>

IAudioMeterInformation*
TBFix_GetAudioMeterInfo (void)
{
  CComPtr <IMMDeviceEnumerator> pDevEnum = nullptr;

  if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
    return nullptr;

  CComPtr <IMMDevice> pDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eConsole,
                                                 &pDevice )
              )
     ) return nullptr;

  IAudioMeterInformation* pMeterInfo = nullptr;

  HRESULT hr = pDevice->Activate ( __uuidof (IAudioMeterInformation),
                                     CLSCTX_ALL,
                                       nullptr,
                                         IID_PPV_ARGS_Helper (&pMeterInfo) );

  if (SUCCEEDED (hr))
    return pMeterInfo;

  return nullptr;
}



__declspec (dllimport)
bool
SK_ImGui_ControlPanel (void);


bool show_config          = true;
bool show_special_k_cfg   = false;
bool show_test_window     = false;
bool show_texture_mod_dlg = false;

ImVec4 clear_col = ImColor (114, 144, 154);

struct {
  std::vector <const char*> array;
  int                       sel   = 0;
} gamepads;

void
__TBF_DefaultSetOverlayState (bool)
{
}

void
TBFix_PauseGame (bool pause)
{
  typedef void (__stdcall *SK_SteamAPI_SetOverlayState_pfn)(bool active);

  static SK_SteamAPI_SetOverlayState_pfn
    SK_SteamAPI_SetOverlayState =
      (SK_SteamAPI_SetOverlayState_pfn)

    TBF_ImportFunctionFromSpecialK (
      "SK_SteamAPI_SetOverlayState",
        &__TBF_DefaultSetOverlayState
    );

  SK_SteamAPI_SetOverlayState (pause);
}

bool reset_frame_history = false;
int  cursor_refs         = 0;

typedef DWORD (*SK_ImGui_Toggle_pfn)(void);
SK_ImGui_Toggle_pfn SK_ImGui_Toggle_Original = nullptr;

extern void TBFix_ShowCursor    (void);
extern void TBFix_ReleaseCursor (void);
extern void TBFix_CaptureCursor (void);

void
TBFix_ToggleConfigUI (void)
{
  DWORD dwPos = GetMessagePos ();

  ImGuiIO& io =
    ImGui::GetIO ();

  if (! io.WantCaptureMouse)
    SendMessage (tbf::RenderFix::hWndDevice, WM_MOUSEMOVE, 0x00, dwPos);

  SK_ImGui_Toggle_Original ();

  reset_frame_history = true;

  bool* visible =
    (bool *)SK_GetCommandProcessor ()->ProcessCommandLine ("ImGui.Visible").getVariable()->getValuePointer ();

  config.input.ui.visible = *visible;

  if (config.input.ui.pause)
    TBFix_PauseGame (config.input.ui.visible);


  if (config.input.ui.visible)
  {
    TBFix_CaptureCursor ();
    TBFix_ShowCursor    ();
  } else {
    TBFix_ReleaseCursor ();
  }

  if (! io.WantCaptureMouse)
    SendMessage (tbf::RenderFix::hWndDevice, WM_MOUSEMOVE, 0x00, dwPos);

  TBF_SaveConfig ();
}

extern
bool
TBFix_TextureModDlg (void);

void
TBFix_GamepadConfigDlg (void)
{
  if (gamepads.array.size () == 0)
  {
    if (GetFileAttributesA ("TBFix_Res\\Gamepads\\") != INVALID_FILE_ATTRIBUTES)
    {
      std::vector <std::string> gamepads_;

      WIN32_FIND_DATAA fd;
      HANDLE           hFind  = INVALID_HANDLE_VALUE;
      int              files  = 0;
      LARGE_INTEGER    liSize = { 0 };

      hFind = FindFirstFileA ("TBFix_Res\\Gamepads\\*", &fd);

      if (hFind != INVALID_HANDLE_VALUE)
      {
        do {
          if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
               fd.cFileName[0]    != '.' )
          {
            gamepads_.push_back (fd.cFileName);
          }
        } while (FindNextFileA (hFind, &fd) != 0);

        FindClose (hFind);
      }

            char current_gamepad [128] = { '\0' };
      snprintf ( current_gamepad, 127,
                   "%ws",
                     config.input.gamepad.texture_set.c_str () );

      for (int i = 0; i < gamepads_.size (); i++)
      {
        gamepads.array.push_back (
          _strdup ( gamepads_ [i].c_str () )
        );

        if (! _stricmp (gamepads.array [i], current_gamepad))
          gamepads.sel = i;
      }
    }
  }

  if (ImGui::BeginPopupModal ("Gamepad Config", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    int orig_sel = gamepads.sel;

    if (ImGui::ListBox ("Gamepad\nIcons", &gamepads.sel, gamepads.array.data (), (int)gamepads.array.size (), 3))
    {
      if (orig_sel != gamepads.sel)
      {
        wchar_t pad [128] = { };

        swprintf_s ( pad, 128,
                       L"%hs",
                         gamepads.array [gamepads.sel]
                   );

        config.input.gamepad.texture_set    = pad;
        tbf::RenderFix::need_reset.textures = true;
      }

      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup();
  }
}

DWORD TBFix_Modal = 0UL;

void
TBFix_KeybindDialog (keybind_s* keybind)
{
  const  float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;
  static bool  was_open  = false;

  static BYTE bind_keys [256] = { 0 };

  ImGui::SetNextWindowSizeConstraints ( ImVec2 (font_size * 9, font_size * 3), ImVec2 (font_size * 30, font_size * 6));

  if (ImGui::BeginPopupModal (keybind->bind_name, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    ImGui::GetIO ().WantCaptureKeyboard = false;

    int keys = 256;

    if (! was_open)
    {
      //keybind->vKey = 0;

      for (int i = 0 ; i < keys ; i++ ) bind_keys [i] = (GetKeyState (i) & 0x8000) != 0;

      was_open = true;
    }

    BYTE active_keys [256];

    for (int i = 0 ; i < keys ; i++ ) active_keys [i] = (GetKeyState (i) & 0x8000) != 0;

    if (memcmp (active_keys, bind_keys, keys))
    {
      int i = 0;

      for (i = (VK_MENU + 1); i < keys; i++)
      {
        if ( i == VK_LCONTROL || i == VK_RCONTROL ||
             i == VK_LSHIFT   || i == VK_RSHIFT   ||
             i == VK_LMENU    || i == VK_RMENU )
          continue;

        if ( active_keys [i] != bind_keys [i] )
          break;
      }

      if (i != keys)
      {
        keybind->vKey = i;
        was_open      = false;
        ImGui::CloseCurrentPopup ();
      }

      //memcpy (bind_keys, active_keys, keys);
    }

    keybind->ctrl  = (GetAsyncKeyState (VK_CONTROL) & 0x8000) != 0; 
    keybind->shift = (GetAsyncKeyState (VK_SHIFT)   & 0x8000) != 0;
    keybind->alt   = (GetAsyncKeyState (VK_MENU)    & 0x8000) != 0;

    keybind->update ();

    ImGui::Text ("Binding:  %ws", keybind->human_readable.c_str ());

    TBFix_Modal = timeGetTime ();

    ImGui::EndPopup ();
  }
}

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

IMGUI_API
void
ImGui_ImplDX9_NewFrame (void);

void
TBFix_GPUPerfMonWidget (void)
{
  extern HMODULE hInjectorDLL;

 typedef uint32_t (__stdcall *SK_GPU_GetClockRateInkHz_pfn)    (int gpu);
 typedef uint32_t (__stdcall *SK_GPU_GetMemClockRateInkHz_pfn) (int gpu);
 typedef uint64_t (__stdcall *SK_GPU_GetMemoryBandwidth_pfn)   (int gpu);
 typedef float    (__stdcall *SK_GPU_GetMemoryLoad_pfn)        (int gpu);
 typedef float    (__stdcall *SK_GPU_GetGPULoad_pfn)           (int gpu);   
 typedef float    (__stdcall *SK_GPU_GetTempInC_pfn)           (int gpu);   
 typedef uint32_t (__stdcall *SK_GPU_GetFanSpeedRPM_pfn)       (int gpu);
 typedef uint64_t (__stdcall *SK_GPU_GetVRAMUsed_pfn)          (int gpu);  
 typedef uint64_t (__stdcall *SK_GPU_GetVRAMShared_pfn)        (int gpu);
 typedef uint64_t (__stdcall *SK_GPU_GetVRAMBudget_pfn)        (int gpu);

 static SK_GPU_GetClockRateInkHz_pfn   SK_GPU_GetClockRateInkHz =
   (SK_GPU_GetClockRateInkHz_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetClockRateInkHz");
 static SK_GPU_GetMemClockRateInkHz_pfn SK_GPU_GetMemClockRateInkHz =
   (SK_GPU_GetMemClockRateInkHz_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetMemClockRateInkHz");
 static SK_GPU_GetMemoryBandwidth_pfn   SK_GPU_GetMemoryBandwidth  =
   (SK_GPU_GetMemoryBandwidth_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetMemoryBandwidth");
 static SK_GPU_GetMemoryLoad_pfn        SK_GPU_GetMemoryLoad       =
   (SK_GPU_GetMemoryLoad_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetMemoryLoad");
 static SK_GPU_GetGPULoad_pfn           SK_GPU_GetGPULoad          =
   (SK_GPU_GetGPULoad_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetGPULoad");
 static SK_GPU_GetTempInC_pfn           SK_GPU_GetTempInC          =
   (SK_GPU_GetTempInC_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetTempInC");
 static SK_GPU_GetFanSpeedRPM_pfn       SK_GPU_GetFanSpeedRPM      =
   (SK_GPU_GetFanSpeedRPM_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetFanSpeedRPM");
 static SK_GPU_GetVRAMUsed_pfn          SK_GPU_GetVRAMUsed         =
   (SK_GPU_GetVRAMUsed_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetVRAMUsed");
 static SK_GPU_GetVRAMShared_pfn        SK_GPU_GetVRAMShared       =
   (SK_GPU_GetVRAMShared_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetVRAMShared");
 static SK_GPU_GetVRAMBudget_pfn        SK_GPU_GetVRAMBudget       =
   (SK_GPU_GetVRAMBudget_pfn)
     GetProcAddress (hInjectorDLL, "SK_GPU_GetVRAMBudget");

  struct {
    int   current_idx = 0;
    float percent [120];
  } static gpu_load;

  gpu_load.percent [gpu_load.current_idx] = SK_GPU_GetGPULoad (0);
  gpu_load.current_idx = (gpu_load.current_idx + 1) % IM_ARRAYSIZE (gpu_load.percent);
  
  if (ImGui::CollapsingHeader ("Peformance Monitor"))
  {
    ImGui::BeginGroup ();
    
    ImGui::PlotLines ( "",
                        gpu_load.percent,
                          IM_ARRAYSIZE (gpu_load.percent),
                            gpu_load.current_idx,
                              "",
                                   0.0f,
                                   100.0f,
                                    ImVec2 (ImGui::GetContentRegionAvailWidth (), 80) );
    
    ImGui::EndGroup ();
  }
}


bool
TBFix_DrawConfigUI (void)
{
  static bool need_restart = false;
  static bool was_reset    = false;

  ImGui_ImplDX9_NewFrame ();

  static bool first_frame = true;

    extern HMODULE hInjectorDLL;

    typedef void (__stdcall *SK_ImGui_DrawEULA_pfn)(LPVOID reserved);

    static SK_ImGui_DrawEULA_pfn SK_ImGui_DrawEULA =
      (SK_ImGui_DrawEULA_pfn)GetProcAddress ( hInjectorDLL,
                                                "SK_ImGui_DrawEULA" );

  struct {
    bool show             = true;
    bool never_show_again = config.input.ui.never_show_eula;
  } static show_eula;
  
  if (show_eula.show && (! show_eula.never_show_again)) {
    SK_ImGui_DrawEULA (&show_eula);

    if (show_eula.never_show_again) config.input.ui.never_show_eula = true;
  }


  ImGuiIO& io =
    ImGui::GetIO ();

  static int frame = 0;

  if (frame++ < 10)
    ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);

  if (io.DisplaySize.x != tbf::RenderFix::width ||
      io.DisplaySize.y != tbf::RenderFix::height)
  {
    frame = 0;

    io.DisplaySize.x = (float)tbf::RenderFix::width;
    io.DisplaySize.y = (float)tbf::RenderFix::height;

    ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);
  }

  ImGui::SetNextWindowSizeConstraints (ImVec2 (665, 50), ImVec2 ( ImGui::GetIO ().DisplaySize.x * 0.95f,
                                                                    ImGui::GetIO ().DisplaySize.y * 0.95f ) );

  if (was_reset) {
    ImGui::SetNextWindowSize (ImVec2 (665, 50), ImGuiSetCond_Always);
    was_reset = false;
  }

  static bool hook_marshall = true;
        bool show_config    = true;

  ImGui::Begin ( "Tales of Berseria \"Fix\" (v " TBF_VERSION_STR_A ") Control Panel",
                   &show_config,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

  ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

  if (ImGui::CollapsingHeader ("UI Scale"))
  {
    ImGui::TreePush    ("");

    ImGui::SliderFloat ("Scale (only 1.0 is officially supported)", &config.input.ui.scale, 1.0f, 3.0f);

    ImGui::TreePop     ();
  }

      io.FontGlobalScale = config.input.ui.scale;
  const  float font_size           =  ImGui::GetFont ()->FontSize                                     * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  if (ImGui::CollapsingHeader ("Framerate", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");
    int limiter =
      config.framerate.replace_limiter ? 1 : 0;

    ImGui::Combo ( "Framerate Limiter", &limiter, "Namco          (A.K.A. Stutterfest 2017)\0"
                                                    "Special K      (Precision Timing For The Win!)\0\0", 2 );

    static float values [120]  = { 0 };
    static int   values_offset =   0;

    values [values_offset] = 1000.0f * ImGui::GetIO ().DeltaTime;
    values_offset = (values_offset + 1) % IM_ARRAYSIZE (values);

    if ((bool)limiter != config.framerate.replace_limiter)
    {
      reset_frame_history              = true;
      config.framerate.replace_limiter = limiter;

      if (config.framerate.replace_limiter)
        tbf::FrameRateFix::BlipFramerate    ();
      else
        tbf::FrameRateFix::DisengageLimiter ();
    }

    if (reset_frame_history) {
      const float fZero = 0.0f;
      memset (values, *(reinterpret_cast <const DWORD *> (&fZero)), sizeof (float) * 120);

      values_offset       = 0;
      reset_frame_history = false;
      was_reset           = true;
    }

    float sum = 0.0f;

    float min = FLT_MAX;
    float max = 0.0f;

    for (float val : values) {
      sum += val;

      if (val > max)
        max = val;

      if (val < min)
        min = val;
    }

    static char szAvg [512] = { };

    sprintf_s ( szAvg,
                  512,
                    "Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
                    "    Extreme frametimes:      %6.3f min, %6.3f max\n\n\n\n"
                  "Variation:  %8.5f ms  ==>  %.1f FPS  +/-  %3.1f frames",
                    sum / 120.0f, tbf::FrameRateFix::GetTargetFrametime (),
                      min, max, max - min,
                        1000.0f / (sum / 120.0f), (max - min) / (1000.0f / (sum / 120.0f)) );

    ImGui::PlotLines ( "",
                         values,
                           IM_ARRAYSIZE (values),
                             values_offset,
                               szAvg,
                                 0.0f,
                                   2.0f * tbf::FrameRateFix::GetTargetFrametime (),
                                     ImVec2 (0, font_size * 7) );

    ImGui::SameLine ();

    if (! config.framerate.replace_limiter) {
      ImGui::BeginChild     ( "LimitDisclaimer", ImVec2 ( font_size * 25, font_size * 7 ) );
      ImGui::TextColored ( ImVec4 (1.0f, 1.0f, 0.0f, 1.0f),
                             "Working limiters do not resemble seismographs!" );

      ImGui::Separator      ();

      ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 ( 0.666f, 0.666f, 0.666f, 1.0f ) );
      ImGui::TextWrapped    ( "\nOnly use this limiter for reference purposes, or when making "
                              "changes to the in-game framerate setting." );
      ImGui::PopStyleColor  ();
      ImGui::EndChild       ();
    }

    else {
      ImGui::BeginChild     ( "LimitDisclaimer", ImVec2 ( font_size * 25, font_size * 7 ) );
      ImGui::TextColored    ( ImVec4 ( 0.2f, 1.0f, 0.2f, 1.0f),
                                "This is how a framerate limiter should work." );

      ImGui::Separator      ();

      ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 ( 0.666f, 0.666f, 0.666f, 1.0f ) );
      ImGui::TextWrapped    ( "\nSelect Namco's Limiter before making any changes to framerate limit in-game,"
                              " or the setting will not take effect." );
      ImGui::PopStyleColor  ();
      ImGui::EndChild       ();

      bool changed = ImGui::SliderFloat ( "Special K Framerate Tolerance", &config.framerate.tolerance, 0.005f, 0.5);

      if (ImGui::IsItemHovered ()) {
        ImGui::BeginTooltip ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
        ImGui::Text           ("Controls Framerate Smoothness\n\n");
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
        ImGui::Text           ("  Lower = Smoother, but setting ");
        ImGui::SameLine       ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 1.0f, 0.65f, 1.0f));
        ImGui::Text           ("too low");
        ImGui::SameLine       ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor(0.75f, 0.75f, 0.75f, 1.0f));
        ImGui::Text           (" will cause framerate instability...");
        ImGui::PopStyleColor  (4);
        ImGui::EndTooltip     ();
      }

      if (changed) {
        tbf::FrameRateFix::DisengageLimiter ();
        SK_GetCommandProcessor ()->ProcessCommandFormatted ( "LimiterTolerance %f", config.framerate.tolerance );
        tbf::FrameRateFix::BlipFramerate    ();
      }

      if (*(bool *)SK_GetCommandProcessor ()->ProcessCommandLine ("RenderHooks.D3D9Ex").getVariable ()->getValuePointer ())
      {
        bool* wait_for_vblank =
          (bool *)SK_GetCommandProcessor ()->ProcessCommandLine ("WaitForVBLANK").getVariable ()->getValuePointer ();

        ImGui::Checkbox ("Wait for VBLANK", wait_for_vblank);
        
        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
          ImGui::Text           ("Lower CPU Usage and Input Latency (but less stable framerate)\n\n");
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
          ImGui::BulletText     ("This aligns timing of the framerate limiter to your monitor's refresh rate -- it IS NOT VSYNC!");
          ImGui::BulletText     ("If you enable this, you will need to adjust the Limiter Tolerance slider (lower value) for best results.");
          ImGui::PopStyleColor  (2);
          ImGui::EndTooltip     ();
        }
      }
    }

    //ImGui::Text ( "Application average %.3f ms/frame (%.1f FPS)",
                    //1000.0f / ImGui::GetIO ().Framerate,
                              //ImGui::GetIO ().Framerate );
    ImGui::TreePop ();
  }

  //TBFix_GPUPerfMonWidget ();

  if (ImGui::CollapsingHeader ("Textures", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("UI Fixes"))
    {
      ImGui::TreePush ("");

      ImGui::Checkbox ("Clamp Non-Power-Of-Two Texture Coordinates", &config.textures.clamp_npot_coords);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("Fixes blurring and black line artifacts on key UI elements, but may break third-party overlays...");
      }

      ImGui::Checkbox ("Clamp Skit Texture Coordinates",  &config.textures.clamp_skit_coords); ImGui::SameLine ();
      //ImGui::Checkbox ("Clamp Map Texture Coordinates",   &config.textures.clamp_map_coords);  ImGui::SameLine ();
      //ImGui::Checkbox ("Clamp Text Texture Coordinates",  &config.textures.clamp_text_coords);

      ImGui::Checkbox ("Keep UI Textures Sharp", &config.textures.keep_ui_sharp);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("Prevent errant drivers from blurring the UI while anti-aliasing.");
      }

      ImGui::TreePop      ();
    }

    if (ImGui::CollapsingHeader ("Quality"))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Generate Mipmaps", &config.textures.remaster)) tbf::RenderFix::need_reset.graphics = true;

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates distant texture aliasing and shimmering caused by missing/incomplete mipmaps.");

      if (config.textures.remaster)
      {
        ImGui::TreePush ("");

        if (ImGui::Checkbox ("Do Not Compress Generated Mipmaps", &config.textures.uncompressed)) tbf::RenderFix::need_reset.graphics = true;

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Uses more VRAM, but avoids texture compression artifacts on generated mipmaps.");

        ImGui::Checkbox ("Show Loading Activity in OSD During Mipmap Generation", &config.textures.show_loading_text);
        ImGui::TreePop  ();
      }

      if (! *(bool *)SK_GetCommandProcessor ()->ProcessCommandLine ("d3d9.HasBrokenMipmapLODBias").getVariable ()->getValuePointer ())
      {
        ImGui::SliderFloat ("Mipmap LOD Bias", &config.textures.lod_bias, -3.0f, /*config.textures.uncompressed ? 16.0f :*/ 3.0f);
        
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Controls texture sharpness;  -3 = Sharpest (WILL shimmer),  0 = Neutral,  3 = Blurry");
          ImGui::EndTooltip   ();
        }
      }

      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("Performance", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
    {
      extern bool __remap_textures;

      ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 15.0f);
      ImGui::TreePush  ("");

      ImGui::BeginChild  ("Texture Details", ImVec2 ( font_size           * 70,
                                                      font_size_multiline * 6.1f ),
                                               true );

      ImGui::Columns   ( 3 );
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "       Savings" );
        ImGui::PopStyleColor  ();
      ImGui::Columns   ( 1 );

      ImGui::PushStyleColor
                       ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

      ImGui::Separator (   );

      ImGui::Columns   ( 3 );
        ImGui::Text    ( "%#6zu MiB Total",
                                                       tbf::RenderFix::tex_mgr.cacheSizeTotal () >> 20ULL ); ImGui::NextColumn (); 
        ImGui::TextColored
                       (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                         "%#5lu     Hits",             tbf::RenderFix::tex_mgr.getHitCount    ()          );  ImGui::NextColumn ();
        ImGui::Text    ( "Budget: %#7zu MiB  ",        tbf::RenderFix::pDevice->
                                                                       GetAvailableTextureMem ()  / 1048576UL );
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      ImColor  active   ( 1.0f,  1.0f,  1.0f, 1.0f);
      ImColor  inactive (0.75f, 0.75f, 0.75f, 1.0f);
      ImColor& color   = __remap_textures ? inactive : active;

      ImGui::Columns   ( 3 );
        ImGui::TextColored ( color,
                               "%#6zu MiB Base",
                                                       tbf::RenderFix::tex_mgr.cacheSizeBasic () >> 20ULL );  ImGui::NextColumn (); 
        if (ImGui::IsItemClicked ())
          __remap_textures = false;

        ImGui::TextColored
                       (ImVec4 (1.0f, 0.3f, 0.3f, 1.0f),
                         "%#5lu   Misses",             tbf::RenderFix::tex_mgr.getMissCount   ()          );  ImGui::NextColumn ();
        ImGui::Text    ( "Time:    %#6.04lf  s  ", tbf::RenderFix::tex_mgr.getTimeSaved       () / 1000.0f);
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      color = __remap_textures ? active : inactive;

      ImGui::Columns   ( 3 );
        ImGui::TextColored ( color,
                               "%#6zu MiB Injected",
                                                       tbf::RenderFix::tex_mgr.cacheSizeInjected () >> 20ULL ); ImGui::NextColumn (); 

        if (ImGui::IsItemClicked ())
          __remap_textures = true;

        ImGui::TextColored (ImVec4 (0.555f, 0.555f, 1.0f, 1.0f),
                         "%.2f  Hit/Miss",          (double)tbf::RenderFix::tex_mgr.getHitCount  () / 
                                                    (double)tbf::RenderFix::tex_mgr.getMissCount ()          ); ImGui::NextColumn ();
        ImGui::Text    ( "Driver: %#7zu MiB  ",    tbf::RenderFix::tex_mgr.getByteSaved          () >> 20ULL );

      ImGui::PopStyleColor
                       (   );
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      ImGui::TreePush  ("");
      ImGui::Checkbox  ("Enable Texture QuickLoad", &config.textures.quick_load);
      
      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  (  );
          ImGui::TextColored ( ImVec4 (0.9f, 0.9f, 0.9f, 1.f), 
                                 "Only read the first texture level-of-detail from disk and split generation of the rest across all available CPU cores." );
          ImGui::Separator   (  );
          ImGui::TreePush    ("");
            ImGui::Bullet    (  ); ImGui::SameLine ();
            ImGui::TextColored ( ImColor (0.95f, 0.75f, 0.25f, 1.0f), 
                                   "Typically this reduces hitching and load-times, but some pop-in may "
                                   "become visible on lower-end CPUs." );
            ImGui::Bullet    (  ); ImGui::SameLine ();
            ImGui::TextColored  ( ImColor (0.95f, 0.75f, 0.25f, 1.0f),
                                   "Ideally, this should be used with texture compression disabled." );
            ImGui::SameLine  (  );
            ImGui::Text      ("[Quality/Generate Mipmaps]");
          ImGui::TreePop     (  );
        ImGui::EndTooltip    (  );
      }

      if (config.textures.quick_load) {
        ImGui::SameLine ();

        
        if (ImGui::SliderInt ("# of Threads", &config.textures.worker_threads, 3, 9))
        {
          // Only allow odd numbers of threads
          config.textures.worker_threads += 1 - (config.textures.worker_threads & 0x1);

          need_restart = true;
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Lower is actually better, the only reason to adjust this would be if you have an absurd number of CPU cores and pop-in bothers you ;)");
      }
      ImGui::TreePop      ( );

      ImGui::EndChild     ( );
      ImGui::PopStyleVar  ( );

      if (ImGui::CollapsingHeader ("Thread Stats"))
      {
        std::vector <tbf_tex_thread_stats_s> stats =
          tbf::RenderFix::tex_mgr.getThreadStats ();

        int thread_id = 0;

        for ( auto it : stats ) {
          ImGui::Text ("Thread #%lu  -  %6lu jobs retired, %5lu MiB loaded  -  %.6f User / %.6f Kernel / %3.1f Idle",
                          thread_id++,
                            it.jobs_retired, it.bytes_loaded >> 20UL,
                              (double)ULARGE_INTEGER { it.runtime.user.dwLowDateTime,   it.runtime.user.dwHighDateTime   }.QuadPart / 10000000.0,
                              (double)ULARGE_INTEGER { it.runtime.kernel.dwLowDateTime, it.runtime.kernel.dwHighDateTime }.QuadPart / 10000000.0,
                              (double)ULARGE_INTEGER { it.runtime.idle.dwLowDateTime,   it.runtime.idle.dwHighDateTime   }.QuadPart / 10000000.0 );
        }
      }
      ImGui::TreePop      ( );
    }

    if (ImGui::Button ("  Texture Modding Tools  ")) {
      show_texture_mod_dlg = (! show_texture_mod_dlg);
    }

    ImGui::SameLine ();

    ImGui::SliderInt ("Texture Cache Size", &config.textures.max_cache_in_mib, 384, 2048, "%.0f MiB");

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Lower this if the reported budget (on the Performance tab) ever becomes negative.");

    ImGui::TreePop ();
  }

  static bool nv_hardware =
    ((BOOL (__stdcall *)(void))GetProcAddress (hInjectorDLL, "SK_NvAPI_IsInit"))();

  if (nv_hardware)
  {
    if (ImGui::CollapsingHeader ("Anti-Aliasing"))
    {
      ImGui::TreePush ("");
    
      if (ImGui::CollapsingHeader ("Sparse Grid Super-Sampling (NVIDIA)"))
      {
        ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.333f);
        bool sel_change =
          ImGui::Combo ("Mode (requires application restart)", (int *)&config.render.nv.sgssaa_mode, " Off\0"
                                                                                                     " 2x (Good Quality, Good Performance)\0"
                                                                                                     " 4x (Great Quality,  OK Performance)\0"
                                                                                                     " 8x (Are You Insane?!)\0\0", 4 );

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextColored (ImColor (0.95f, 0.75f, 0.25f, 1.0f), "Ultra High Quality Supersampling");
          ImGui::SameLine     ();
          ImGui::Text         (" (driver feature for NVIDIA systems)");
          ImGui::Separator    ();
          ImGui::BulletText   ("Do NOT combine this with in-game anti-aliasing (broken) or DSR.");
          ImGui::BulletText   ("Prefer this over DSR because this produces higher image quality with fewer compatibility problems.");
          ImGui::BulletText   ("The first time you enable this, the game will re-launch with admin privileges to setup a driver profile.");
          ImGui::EndTooltip   ();
        }

        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        if (ImGui::TreeNode ("Advanced Settings"))
        {
          int bits = wcstoul (config.render.nv.compat_bits.c_str (), nullptr, 16);

          ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.1666f);

          if (ImGui::InputInt ("Compatibility Bits", &bits, 1, 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
          {
             wchar_t wszBits [16] = { L'\0' };

             _snwprintf (wszBits, 10, L"0x%08x", bits);

             config.render.nv.compat_bits = wszBits;
             need_restart                 = true;
          }

          ImGui::PopItemWidth ();
          ImGui::TreePop      ();
        }

        ImGui::EndGroup ();

        if (sel_change)
        {
          ((void (__stdcall *)(const wchar_t * ))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAppFriendlyName"))     ( L"Tales of Berseria" );
          ((void (__stdcall *)(const wchar_t * ))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAppName"))             ( L"Tales of Berseria.exe" );

          need_restart = true;
        }

        ImGui::TreePush ("");

        if (ImGui::CollapsingHeader ("SGSSAA Best Practices"))
        {
          ImGui::TreePush ("");
          ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 16.0f);
          ImGui::BeginChild     ("SGSSAA Best Practices", ImVec2 (font_size * 75.0f, font_size_multiline * 5.5f), true);
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::Text           ( "Supersampling is unlike other forms of anti-aliasing and will anti-alias textures and lighting rather than blur them into oblivion!\n");
          ImGui::Separator      ();

          ImGui::Spacing     ();
          ImGui::Spacing     ();

          ImGui::TreePush    ("");
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.6f, 0.9f, 0.3f, 1.0f));
          ImGui::Text           ( "There are a few things you should know about supersampling for best results.\n");

          ImGui::Spacing     ();

          ImGui::TreePush    ("");
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.75, 0.75f, 0.75f, 1.0f));
          ImGui::BulletText ("It puts a lot of compute AND memory load on GPUs, you will want to tweak Environmental Shadow settings to balance things out.");
          ImGui::Spacing     ();
          ImGui::BulletText ("Post-processing effects like blur may be strengthened; working around this involves increasing Post-Processing Resolution.");
          ImGui::Spacing     ();
          ImGui::BulletText ("A negative Mipmap LOD Bias is suggested for best image quality, it will not create shimmer when supersampling.");

          ImGui::PopStyleColor (3);
          ImGui::TreePop       ();
          ImGui::TreePop       ();
          ImGui::EndChild      ();
          ImGui::PopStyleVar   ();
          ImGui::TreePop       ();
        }
        ImGui::TreePop ();
      }

      ImGui::PopItemWidth ();
      ImGui::TreePop      ();
    }

#if 0
    if (ImGui::CollapsingHeader ("SMAA Tuning Knobs"))
    {
      ImGui::Checkbox    ("Override Game's Tuning Fish", &config.render.smaa.override_game);

      if (config.render.smaa.override_game)
      {
        ImGui::TreePush ("");

        bool changed = ImGui::Combo ( "SMAA Quality Preset", &config.render.smaa.quality_preset,
                                        " Low\0"
                                        " Medium\0"
                                        " High\0"
                                        " Ultra\0"
                                        " Custom\00", 5 );

        if (changed)
        {
          switch (config.render.smaa.quality_preset)
          {
            // Low
            case 0:
              config.render.smaa.threshold             = 0.15f;
              config.render.smaa.max_search_steps      =     4;
              config.render.smaa.max_search_steps_diag =     0;
              config.render.smaa.corner_rounding       =     0;
              break;

            // Medium
            case 1:
              config.render.smaa.threshold             = 0.1f;
              config.render.smaa.max_search_steps      =    8;
              config.render.smaa.max_search_steps_diag =    0;
              config.render.smaa.corner_rounding       =    0;
              break;

            // High
            case 2:
              config.render.smaa.threshold             = 0.1f;
              config.render.smaa.max_search_steps      =   16;
              config.render.smaa.max_search_steps_diag =    8;
              config.render.smaa.corner_rounding       =   25;
              break;

            // Ultra
            case 3:
              config.render.smaa.threshold             = 0.05f;
              config.render.smaa.max_search_steps      =    32;
              config.render.smaa.max_search_steps_diag =    16;
              config.render.smaa.corner_rounding       =    25;
              break;
          }
        }

        bool customized = false;

        ImGui::TreePush ("");

        customized |= ImGui::SliderFloat ("Edge Sensitivity",         &config.render.smaa.threshold,             0.0f, 0.5f);
        customized |= ImGui::SliderInt   ("Lateral Search Depth",     &config.render.smaa.max_search_steps,      0,     112);
        customized |= ImGui::SliderInt   ("Diagonal Search Depth",    &config.render.smaa.max_search_steps_diag, 0,      20);
        customized |= ImGui::SliderFloat ("Corner Rounding",          &config.render.smaa.corner_rounding,       0,     100, "%.2f%%");

        bool expanded = ImGui::TreeNode ("Advanced SMAA Settings");  if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("As if the other stuff wasn't advanced =P");

        if (expanded)
        {
          ImGui::TreePop ();
          customized |= ImGui::SliderFloat ("Predication Threshold",    &config.render.smaa.predication_threshold, 0.0f,  100.0f);
          customized |= ImGui::SliderFloat ("Predication Scale",        &config.render.smaa.predication_scale,     1.0f,    5.0f);
          customized |= ImGui::SliderFloat ("Predication Strength",     &config.render.smaa.predication_strength,  0.0f,    1.0f);
          customized |= ImGui::SliderFloat ("Reprojection Weight",      &config.render.smaa.reprojection_weight,   0.0f,   80.0f);
        }

        ImGui::TreePop ();

        if (customized)
          config.render.smaa.quality_preset = 4; // User is no longer using a quality preset...

        ImGui::TreePop ();
      }
    }
#endif
  }

  if (ImGui::CollapsingHeader ("Aspect Ratio"))
  {
    ImGui::TreePush ("");

    ImGui::Checkbox ("Enable Aspect Ratio Correction", &config.render.aspect_correction);
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("The first time you enable this, return to the title screen to have the camera's aspect ratio corrected.\n\n"
                         "Also turn off the game's Aspect Ratio Correction feature.");

    ImGui::Checkbox ("Clear Black Bars During Videos", &config.render.clear_blackbars);

#if 0
    if (ImGui::SliderFloat ("Field of View", aspect_ratio.fov_addrs [aspect_ratio.selector], 45.0f, 90.0f)) {
      //aspect_ratio.setFOV (fov);
    }

    ImGui::SliderInt ("Field of View Selector", &aspect_ratio.selector, 0, aspect_ratio.fov_count-1);
    ImGui::Text ("FoV Addr: %p", aspect_ratio.fov_addrs [aspect_ratio.selector]);
#endif

    ImGui::Text ("Aspect Ratio Toggle Keybinding:  %ws",  config.keyboard.aspect_ratio.human_readable.c_str ());

    if (ImGui::IsItemHovered ()) {
      ImGui::SetTooltip ("Click here to change.");
    }

    if (ImGui::IsItemClicked ()) {
      ImGui::OpenPopup (config.keyboard.aspect_ratio.bind_name);
    }

    TBFix_KeybindDialog (&config.keyboard.aspect_ratio);

    extern float original_aspect;

    if ( original_aspect > config.render.aspect_ratio + 0.001f ||
         original_aspect < config.render.aspect_ratio - 0.001f ) {
      ImGui::Separator ();

      ImGui::Bullet (); ImGui::SameLine ();

      ImGui::TextColored (ImVec4 (1.0f, 0.8f, 0.2f, 1.0f), "Aspect Ratio Has Changed Since Startup, for best results return to the title screen.");
    }

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Post-Processing"))
  {
    ImGui::TreePush ("");

    int sel = config.render.postproc_ratio == 0.0f ?
                0 : 1;
    if (sel == 1) {
      if (config.render.postproc_ratio < 1.0)
        sel = 1;
      else if (config.render.postproc_ratio < 2.0)
        sel = 2;
      else
        sel = 3;
    }

    if ( ImGui::Combo ( "Depth of Field", &sel,
                        " Normal (Game Default: Variable) -- Will cause shimmering\0"
                        " Low    ( 50% Screen Resolution)\0"
                        " Medium (100% Screen Resolution)\0"
                        " High   (200% Screen Resolution)\0\0", 4 ) )
    {
      switch (sel) {
        case 0:
          config.render.postproc_ratio = 0.0f;
          break;
        default:
        case 1:
          config.render.postproc_ratio = 0.5f;
          break;
        case 2:
          config.render.postproc_ratio = 1.0f;
          break;
        case 3:
          config.render.postproc_ratio = 2.0f;
          break;
      }

      tbf::RenderFix::need_reset.textures = true;
    }

    if (ImGui::IsItemHovered ())
    {
        ImGui::BeginTooltip ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
        ImGui::Text           ("Fixes problems caused by the game performing post-processing at a fixed resolution.");
        ImGui::Separator      ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
        ImGui::BulletText     ("50%% most closely matches the game's original behavior");
        ImGui::BulletText     ("Higher values will actually reduce the strength of post-processing effects such as atmospheric bloom");
        ImGui::PopStyleColor  (2);
        ImGui::EndTooltip     ();
    }

    sel = config.render.high_res_bloom ? 1 : 0;

    if ( ImGui::Combo ( "Bloom", &sel,
                        " Normal ( 25% Screen Resolution) -- May Flicker\0"
                        " High   ( 50% Screen Resolution)\0\0", 2 ) )
    {
      config.render.high_res_bloom = (sel == 1);
      tbf::RenderFix::need_reset.textures = true;
    }

    sel = config.render.high_res_reflection ? 1 : 0;

    if ( ImGui::Combo ( "Reflection", &sel,
                        " Normal ( 50% Screen Resolution)\0"
                        " High   (100% Screen Resolution)\0\0", 2 ) )
    {
      config.render.high_res_reflection = (sel == 1);
      tbf::RenderFix::need_reset.textures = true;
    }

    tbf::RenderFix::need_reset.graphics |=
      ImGui::Checkbox ("Fix Blocky Map Menu", &config.render.fix_map_res);

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("This feature is a work-in-progress and generally only works at 16:9 resolutions.");
      ImGui::EndTooltip   ();
    }

    tbf::RenderFix::need_reset.graphics |=
      ImGui::Checkbox ("Force Mipmap Filtering on Post-Processed Render Passes", &config.render.force_post_mips);

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Reduce Aliasing AND Increase Performance on Post-Processing");
      ImGui::BulletText   ("Slight performance gain assuming drivers / third-party software / untested render passes do not explode.");
      ImGui::BulletText   ("Still extremely experimental; not saved in config file.");
      ImGui::EndTooltip   ();
    }

    ImGui::TreePop ();
  }

  D3DCAPS9 caps;
  if (SUCCEEDED (tbf::RenderFix::pDevice->GetDeviceCaps (&caps)))
  {
    if (ImGui::CollapsingHeader ("Shadows"))
    {

      ImGui::TreePush ("");
      struct shadow_imp_s
      {
        shadow_imp_s (int scale)
        {
          scale = std::abs (scale);

          if (scale > 3)
            scale = 3;

          radio    = scale;
          last_sel = radio;
        }

        int radio    = 0;
        int last_sel = 0;
      };

      static shadow_imp_s shadows     (config.render.shadow_rescale);
      static shadow_imp_s env_shadows (config.render.env_shadow_rescale);



      ImGui::Combo      ("Character Shadow Resolution",     &shadows.radio,     "Normal\0Enhanced\0High\0Ultra\0\0");

      if (caps.MaxTextureWidth >= 8192 && caps.MaxTextureHeight >= 8192) {
        env_shadows.radio = std::min (env_shadows.radio, 3);
        ImGui::Combo      ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0High\0Ultra\0\0");
      }
      else if (caps.MaxTextureWidth >= 4096 && caps.MaxTextureHeight >= 4096) {
        env_shadows.radio = std::min (env_shadows.radio, 2);
        ImGui::Combo      ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0High\0\0");
      }
      else {
        env_shadows.radio = std::min (env_shadows.radio, 1);
        ImGui::Combo      ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0\0");
      }

      if (env_shadows.radio != env_shadows.last_sel) {
        config.render.env_shadow_rescale    = env_shadows.radio;
        env_shadows.last_sel                = env_shadows.radio;
        tbf::RenderFix::need_reset.graphics = true;
      }

      if (shadows.radio != shadows.last_sel) {
        config.render.shadow_rescale        = -shadows.radio;
        shadows.last_sel                    =  shadows.radio;
        tbf::RenderFix::need_reset.graphics = true;
      }

      int quality = config.render.half_float_shadows ? 0 : 1;

      if ( ImGui::Combo ( "Shadow Map Precision", &quality,
                                                "Low  (16-bit half-float)\0"
                                                "High (32-bit single-float)\0\0", 2 ) )
      {
        config.render.half_float_shadows    = quality == 0 ? true :
                                                             false;
        tbf::RenderFix::need_reset.textures = true;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ( "Game's default is 32-bit, but that burns through GPU memory bandwidth outdoors.\n\n"
                            "  The difference in quality between 16-bit and 32-bit is minor; performance wise the difference is massive." );

      if (config.render.env_shadow_rescale > 1 && (! config.render.half_float_shadows))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.4f, 0.15f, 1.0f));
        ImGui::BulletText     ("WARNING: ");
        ImGui::SameLine       ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 0.05f, 1.0f));
        ImGui::TextWrapped    ("Ultra Quality Environmental Shadows are VERY taxing outdoors and can destabilize framerate even on high-end hardware.");
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.666f, 0.666f, 0.666f, 1.0f));
        ImGui::Text           ("      (Consider changing the precision from High to Low if performance suffers.)");
        ImGui::PopStyleColor  (3);
      }

      ImGui::TreePop ();
    }
  }

  if (ImGui::CollapsingHeader ("Input"))
  {
    ImGui::TreePush  ("");

    if (ImGui::CollapsingHeader ("Keyboard"))
      tbf::KeyboardFix::DrawControlPanel ();

    if (ImGui::CollapsingHeader ("Gamepad / AI"))
    {
      ImGui::PushItemWidth (300);

      if (ImGui::SliderInt ("Number of Virtual Controllers (AI Fix)", &config.input.gamepad.virtual_controllers, 0, 3)) {
        extern void
        TBF_InitSDLOverride (void);
      
        TBF_InitSDLOverride ();    
      }
        
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Map Players 2-4 to individual TBFix Dummy Controllers under Controller Settings, then set Control Mode = Auto for Players 2-4.");
      
      ImGui::PopItemWidth ();
    }

    ImGui::TreePop   (  );
  }

  if (ImGui::CollapsingHeader ("Screenshots"))
  {
    if (ImGui::IsItemHovered ())
    {
         char szWorkingDir [MAX_PATH];
      GetCurrentDirectoryA (MAX_PATH, szWorkingDir);

      ImGui::SetTooltip ("Screenshots are stored in '%s\\Screenshots'", szWorkingDir);
    }

    ImGui::TreePush ("");

    ImGui::Text ("No HUD Keybinding:  %ws",         config.keyboard.hudless.human_readable.c_str ());
    if (ImGui::IsItemHovered ()) {
      ImGui::SetTooltip ("Click here to change.");
    }

    if (ImGui::IsItemClicked ()) {
      ImGui::OpenPopup (config.keyboard.hudless.bind_name);
    }

    TBFix_KeybindDialog (&config.keyboard.hudless);

    ImGui::Checkbox ("Import Screenshot to Steam", &config.screenshots.import_to_steam);

    if (config.screenshots.import_to_steam)
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("Keep High Quality PNG Screenshot After Import", &config.screenshots.keep);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Screenshots will register in Steam much quicker with this disabled.");

      ImGui::TreePop  ();
    }
    else
      config.screenshots.keep = true;

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Audio"))
  { 
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Volume Levels"))
    {
      IAudioMeterInformation* pMeterInfo =
        TBFix_GetAudioMeterInfo ();

      if (pMeterInfo != nullptr)
      {
        UINT channels = 0;

        if (SUCCEEDED (pMeterInfo->GetMeteringChannelCount (&channels)))
        {
          static float channel_peaks_    [32];

          if (channels < 4)
          {
            ImGui::TextColored ( ImVec4 (0.9f, 0.7f, 0.2f, 1.0f),
                                   "WARNING: Do not select Surround in-game, you will be missing center channel audio on your hardware!" );
            ImGui::Separator   ();
          }

          struct
          {
            struct {
              float inst_min = FLT_MAX;  DWORD dwMinSample = 0;  float disp_min = FLT_MAX;
              float inst_max = FLT_MIN;  DWORD dwMaxSample = 0;  float disp_max = FLT_MIN;
            } vu_peaks;

            float peaks [120];
            int   current_idx;
          } static history [32];

          #define VUMETER_TIME 300

          ImGui::Columns (2);

          for (UINT i = 0 ; i < std::min (config.audio.channels, channels); i++)
          {
            if (SUCCEEDED (pMeterInfo->GetChannelsPeakValues (channels, channel_peaks_)))
            {
              history [i].vu_peaks.inst_min = std::min (history [i].vu_peaks.inst_min, channel_peaks_ [i]);
              history [i].vu_peaks.inst_max = std::max (history [i].vu_peaks.inst_max, channel_peaks_ [i]);

              history [i].vu_peaks.disp_min    = history [i].vu_peaks.inst_min;

              if (history [i].vu_peaks.dwMinSample < timeGetTime () - VUMETER_TIME * 3) {
                history [i].vu_peaks.inst_min    = channel_peaks_ [i];
                history [i].vu_peaks.dwMinSample = timeGetTime ();
              }

              history [i].vu_peaks.disp_max    = history [i].vu_peaks.inst_max;

              if (history [i].vu_peaks.dwMaxSample < timeGetTime () - VUMETER_TIME * 3) {
                history [i].vu_peaks.inst_max    = channel_peaks_ [i];
                history [i].vu_peaks.dwMaxSample = timeGetTime ();
              }

              history [i].peaks [history [i].current_idx] = channel_peaks_ [i];
              history [i].current_idx = (history [i].current_idx + 1) % IM_ARRAYSIZE (history [i].peaks);

              ImGui::BeginGroup ();

              ImGui::PlotLines ( "",
                                  history [i].peaks,
                                    IM_ARRAYSIZE (history [i].peaks),
                                      history [i].current_idx,
                                        "",
                                             history [i].vu_peaks.disp_min,
                                             1.0f,
                                              ImVec2 (ImGui::GetContentRegionAvailWidth (), 80) );

              //char szName [64];
              //sprintf (szName, "Channel: %lu", i);

              ImGui::PushStyleColor (ImGuiCol_PlotHistogram,     ImVec4 (0.9f, 0.1f, 0.1f, 0.15f));
              ImGui::ProgressBar    (history [i].vu_peaks.disp_max, ImVec2 (-1.0f, 0.0f));
              ImGui::PopStyleColor  ();

              ImGui::ProgressBar    (channel_peaks_ [i],          ImVec2 (-1.0f, 0.0f));

              ImGui::PushStyleColor (ImGuiCol_PlotHistogram,     ImVec4 (0.1f, 0.1f, 0.9f, 0.15f));
              ImGui::ProgressBar    (history [i].vu_peaks.disp_min, ImVec2 (-1.0f, 0.0f));
              ImGui::PopStyleColor  ();
              ImGui::EndGroup ();

              if (! (i % 2))
              {
                ImGui::SameLine (); ImGui::NextColumn ();
              } else {
                ImGui::Columns   ( 1 );
                ImGui::Separator (   );
                ImGui::Columns   ( 2 );
              }
            }
          }

          ImGui::Columns (1);
        }

        pMeterInfo->Release ();
      }
    }

    if (ImGui::CollapsingHeader ("Mix Format"))
    {
      if (tbf::SoundFix::wasapi_init)
      {
        ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 16.0f);
        ImGui::BeginChild  ("Audio Details", ImVec2 (0.0f, font_size * 6.0f), true);

          ImGui::Columns   (3);
          ImGui::Text      ("");                                                                     ImGui::NextColumn ();
          ImGui::Text      ("Sample Rate");                                                          ImGui::NextColumn ();
          ImGui::Text      ("Channel Setup");
          ImGui::Columns   (1);
          ImGui::Separator ();
          ImGui::Columns   (3);
          ImGui::Text      ( "Game (SoundCore)");                                                    ImGui::NextColumn ();
          ImGui::Text      ( "%.2f kHz @ %lu-bit", (float)tbf::SoundFix::snd_core_fmt.nSamplesPerSec / 1000.0f,
                                                     tbf::SoundFix::snd_core_fmt.wBitsPerSample );   ImGui::NextColumn ();
          ImGui::Text      ( "%lu",                  tbf::SoundFix::snd_core_fmt.nChannels );
          ImGui::Columns   (1);
          ImGui::Separator ();
          ImGui::Columns   (3);
          ImGui::Text      ( "Device");                                                              ImGui::NextColumn ();
          ImGui::Text      ( "%.2f kHz @ %lu-bit", (float)tbf::SoundFix::snd_device_fmt.nSamplesPerSec / 1000.0f,
                                                     tbf::SoundFix::snd_device_fmt.wBitsPerSample ); ImGui::NextColumn ();
          ImGui::Text      ( "%lu",                  tbf::SoundFix::snd_device_fmt.nChannels );
          ImGui::Columns   (1);

        ImGui::EndChild    ();
        ImGui::PopStyleVar ();
      }

      ImGui::Separator ();

      need_restart |= ImGui::Checkbox ("Enable Override", &config.audio.enable_fix);

      if (config.audio.enable_fix)
      {
        IAudioMeterInformation* pMeterInfo =
          TBFix_GetAudioMeterInfo ();

        UINT channels = 2;

        if (pMeterInfo != nullptr)
        {
          if (FAILED (pMeterInfo->GetMeteringChannelCount (&channels)))
            channels = 2;

          pMeterInfo->Release ();
        }

        ImGui::TreePush ("");
          need_restart |= ImGui::RadioButton ("Stereo",       (int *)&config.audio.channels, 2);

          if (channels >= 4)
          {
            ImGui::SameLine();  need_restart |= ImGui::RadioButton ("Quadraphonic",   (int *)&config.audio.channels, 4);

            if (channels >= 6) {
              ImGui::SameLine (); need_restart |= ImGui::RadioButton ("5.1 Surround", (int *)&config.audio.channels, 6);
            }
          }
        ImGui::TreePop  (  );
        
        ImGui::TreePush ("");
        int sel;
        
             if (config.audio.sample_hz == 48000) sel = 2;
        else if (config.audio.sample_hz == 44100) sel = 1;
        else                                      sel = 0;
        
        need_restart |= ImGui::Combo ("Sample Rate", &sel, " Unlimited (expect distortion)\0 44.1 kHz\0 48.0 kHz\0\0", 3);
        
             if (sel == 0) config.audio.sample_hz = -1;
        else if (sel == 1) config.audio.sample_hz = 44100;
        else               config.audio.sample_hz = 48000;
           
        
        need_restart |= ImGui::Checkbox ("Use Compatibility Mode", &config.audio.compatibility);
        
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("May reduce audio quality, but can help with some weird USB headsets and Windows 7 / Older.");
        ImGui::TreePop ();
      }
    }
    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Fun Stuff"))
  {
    ImGui::TreePush ("");

    ImGui::Columns (3);

    ImGui::Checkbox ("Hollow Eye Mode", &config.fun_stuff.hollow_eye_mode);
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("The stuff nightmares are made from.");

    ImGui::NextColumn ();

    ImGui::Checkbox ("Plastic Mode", &config.fun_stuff.plastic_mode);
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("It's fantastic.");

    ImGui::NextColumn ();

    ImGui::Checkbox ("Disable Smoke",   &config.fun_stuff.disable_smoke);
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Does not disable all smoke... just the super obnoxious concentric ring (banding from hell) variety that makes you want to gouge your eyes out in dungeons.");

    ImGui::NextColumn ();

    ImGui::Checkbox ("Disable Pause Screen Dimming", &config.fun_stuff.disable_pause_dim);
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Potentially useful for texture modders who need stuff to stand still but do not want the screen dimmed -- use with the mod's Auto-Pause feature.");

    ImGui::NextColumn ();

    float* fSpeed = (float *)(0x1409C64F0);

    DWORD dwOriginal;
    VirtualProtect ((LPVOID)fSpeed, sizeof (float), PAGE_EXECUTE_READWRITE, &dwOriginal);

    bool humming_bird = *fSpeed == 12.0f;

    if (ImGui::Checkbox ("Speed Run Mode", &humming_bird)) {
      if (humming_bird) *fSpeed = 12.0f;
      else              *fSpeed = 60.0f;
    }

    VirtualProtect((LPVOID)fSpeed, sizeof (float), dwOriginal, &dwOriginal);

    ImGui::Columns (1);

    ImGui::TreePop  ();
  }

  if (ImGui::CollapsingHeader ("Debug"))
  {
    ImGui::TreePush ("");

    ImGui::BeginGroup   ();
    ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 15.0f);
    ImGui::BeginChild   ("Debug Flags", ImVec2 ( font_size           * 35,
                                                 font_size_multiline * ( 1.3f + ( (MAX_FLAGS / 4) +
                                                                                  (MAX_FLAGS % 4 > 0 ? 1 : 0) ) ) ),
                                             true );
    for (int i = 0; i < MAX_FLAGS; i++)
    {
      char szDesc [32] = { '\0' };
      if (i == 0)
        sprintf (szDesc, "Base %06x", (uint32_t)START_FLAG + i);
      else
        sprintf (szDesc, "Flag [%#4lu]", i);

      extern uintptr_t
      TBF_GetBaseAddr (void);

      uint8_t* flag = (uint8_t *)(TBF_GetBaseAddr () + START_FLAG + i);

      bool bFlag = *flag != 0;

      if (ImGui::Checkbox (szDesc, &bFlag))
        *flag = (bFlag ? 1 : 0);

      if (((i + 1) % 4) != 0 && i != (MAX_FLAGS - 1))
        ImGui::SameLine ();
    }

    ImGui::EndChild ();
    ImGui::EndGroup ();

    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    tbf::RenderFix::need_reset.graphics |=
      ImGui::Checkbox ("Enable D3D9 Debug Validation Layer", &config.render.validation);

    ImGui::Checkbox ("Marshall UI Draws Through Hooked Procedures", &hook_marshall);

    ImGui::Separator ();

    ImGui::Text ("  Clear Enemies Keybinding:  %ws",  config.keyboard.clear_enemies.human_readable.c_str ());

    if (ImGui::IsItemHovered ()) {
      ImGui::SetTooltip ("Click here to change.");
    }

    if (ImGui::IsItemClicked ()) {
      ImGui::OpenPopup (config.keyboard.clear_enemies.bind_name);
    }

    TBFix_KeybindDialog (&config.keyboard.clear_enemies);

    ImGui::Text ("Respawn Enemies Keybinding:  %ws",  config.keyboard.respawn_enemies.human_readable.c_str ());

    if (ImGui::IsItemHovered ()) {
      ImGui::SetTooltip ("Click here to change.");
    }

    if (ImGui::IsItemClicked ()) {
      ImGui::OpenPopup (config.keyboard.respawn_enemies.bind_name);
    }

    TBFix_KeybindDialog (&config.keyboard.respawn_enemies);

    ImGui::Text ("Battle Timestop Keybinding:  %ws",  config.keyboard.battle_timestop.human_readable.c_str ());

    if (ImGui::IsItemHovered ()) {
      ImGui::SetTooltip ("Click here to change.");
    }

    if (ImGui::IsItemClicked ()) {
      ImGui::OpenPopup (config.keyboard.battle_timestop.bind_name);
    }

    TBFix_KeybindDialog (&config.keyboard.battle_timestop);

    ImGui::EndGroup ();

    ImGui::TreePop ();
  }

  ImGui::PopItemWidth ();

  ImGui::Separator (   );
  ImGui::Columns   ( 2 );

  if (ImGui::Button ("   Gamepad Config   "))
    ImGui::OpenPopup ("Gamepad Config");

  TBFix_GamepadConfigDlg ();

  ImGui::SameLine      ();

  if (ImGui::Button ("   Special K Config   "))
    show_special_k_cfg = (! show_special_k_cfg);

  ImGui::SameLine (0.0f, 60.0f);

  if (ImGui::Selectable (" ... ", show_test_window))
    show_test_window = (! show_test_window);

  ImGui::NextColumn ();

  ImGui::Checkbox ("Apply Changes Immediately ", &config.render.auto_apply_changes);

  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
    ImGui::Text           ("Third-Party Software May Not Like This\n\n");
    ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
    ImGui::BulletText     ("You also may not when it takes 5 seconds to change some settings ;)");
    ImGui::PopStyleColor  (2);
    ImGui::EndTooltip     ();
  }

  ImGui::SameLine ();

  if ( ImGui::Checkbox ("Pause Game While Menu Is Open ", &config.input.ui.pause) )
    TBFix_PauseGame (config.input.ui.pause);

  bool extra_details = false;

  if (need_restart || tbf::RenderFix::need_reset.graphics || tbf::RenderFix::need_reset.textures)
    extra_details = true;

  if (extra_details)
  {
    ImGui::Columns    ( 1 );
    ImGui::Separator  (   );

    if (need_restart)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.4f, 0.15f, 1.0f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }
    
    if (tbf::RenderFix::need_reset.graphics || tbf::RenderFix::need_reset.textures)
    {
      if (! config.render.auto_apply_changes)
      {
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Bullet          ( ); ImGui::SameLine ();
        
        ImGui::TextWrapped     ( "You have made changes to settings that require the game to re-load resources...");
        ImGui::Button          ( "  Apply Now  " );
        
        if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Click this before complaining that things look weird.");
        if (ImGui::IsItemClicked ()) tbf::RenderFix::TriggerReset ();
        
        ImGui::PopStyleColor   ( );
        ImGui::PopTextWrapPos  ( );
      }

      else
      {
        ImGui::TextColored           (ImVec4 (0.95f, 0.65f, 0.15f, 1.0f), "Applying Changes...");
        tbf::RenderFix::TriggerReset ();
      }
    }
  }
  
  ImGui::End ();


  if (show_test_window)
  {
    ImGui::SetNextWindowPos (ImVec2 (650, 20), ImGuiSetCond_FirstUseEver);
    ImGui::ShowTestWindow   (&show_test_window);
  }

  if (show_special_k_cfg)  show_special_k_cfg = SK_ImGui_ControlPanel ();

  if (show_texture_mod_dlg)
  {
    show_texture_mod_dlg = TBFix_TextureModDlg ();
  }

  extern EndScene_pfn   D3D9EndScene;
  extern BeginScene_pfn D3D9BeginScene;

  if ( SUCCEEDED (
         hook_marshall ? tbf::RenderFix::pDevice->BeginScene () : D3D9BeginScene (tbf::RenderFix::pDevice)
       )
     )
  {
    ImGui::Render                     ();
    hook_marshall ? tbf::RenderFix::pDevice->EndScene () : D3D9EndScene (tbf::RenderFix::pDevice);
  }

  if (! show_config)
  {
    TBFix_ToggleConfigUI ();
  }

  return show_config;
}


typedef DWORD (*SK_ImGui_DrawFrame_pfn)(DWORD dwFlags, void* user);
SK_ImGui_DrawFrame_pfn SK_ImGui_DrawFrame_Original = nullptr;

DWORD
TBFix_ImGui_DrawFrame (DWORD dwFlags, void* user)
{
  TBFix_DrawConfigUI ();

  return 1;
}


void
TBFix_ImGui_Init (void)
{
  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                        "SK_ImGui_Toggle",
                           TBFix_ToggleConfigUI,
             (LPVOID *)&SK_ImGui_Toggle_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                         "SK_ImGui_DrawFrame",
                      TBFix_ImGui_DrawFrame,
              (LPVOID *)&SK_ImGui_DrawFrame_Original );
}


#define ID_TIMER  1
#define TIMER_PERIOD  125