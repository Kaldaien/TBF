// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui/imgui.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "config.h"
#include "render.h"
#include "framerate.h"
#include "hook.h"


#include <string>
#include <vector>


bool show_config      = true;
bool show_test_window = false;

ImVec4 clear_col = ImColor(114, 144, 154);

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

int cursor_refs = 0;

void
TBFix_ToggleConfigUI (void)
{
  static int cursor_refs = 0;

  if (config.input.ui.visible) {
    while (ShowCursor (FALSE) > cursor_refs)
      ;
    while (ShowCursor (TRUE) < cursor_refs)
      ;
  }
  else {
    cursor_refs = (ShowCursor (FALSE) + 1);

    if (cursor_refs > 0) {
      while (ShowCursor (FALSE) > 0)
        ;
    }
  }

  config.input.ui.visible = (! config.input.ui.visible);

  SK_GetCommandProcessor ()->ProcessCommandLine (
    config.input.ui.visible ? "ImGui.Visible 1" :
                              "ImGui.Visible 0"
    );

  if (config.input.ui.pause)
    TBFix_PauseGame (config.input.ui.visible);
}


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

        if (! stricmp (gamepads.array [i], current_gamepad))
          gamepads.sel = i;
      }
    }
  }

  if (ImGui::BeginPopupModal ("Gamepad Config", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    int orig_sel = gamepads.sel;

    if (ImGui::ListBox ("Gamepad\nIcons", &gamepads.sel, gamepads.array.data (), gamepads.array.size (), 3))
    {
      if (orig_sel != gamepads.sel)
      {
        wchar_t pad[128] = { L'\0' };

        swprintf(pad, L"%hs", gamepads.array[gamepads.sel]);

        config.input.gamepad.texture_set = pad;

        tbf::RenderFix::need_reset.textures = true;
      }

      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup();
  }
}

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

IMGUI_API
void
ImGui_ImplDX9_NewFrame (void);


void
TBFix_DrawConfigUI (void)
{
  ImGui_ImplDX9_NewFrame ();

  ImGuiIO& io = ImGui::GetIO ();

  //ImGui::SetNextWindowPos             (ImVec2 ( 640, 360), ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
  ImGui::SetNextWindowSizeConstraints (ImVec2 (50, 50), ImGui::GetIO ().DisplaySize);

  // 1. Show a simple window
  // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    ImGui::Begin ("Tales of Berseria \"Fix\" Control Panel", &show_config, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.65f);

    if (tbf::RenderFix::need_reset.graphics || tbf::RenderFix::need_reset.textures) {
      ImGui::TextColored ( ImVec4 (0.8f, 0.8f, 0.1f, 1.0f),
                             "    You have made changes that will not apply until you change Screen Modes in Graphics Settings,\n"
                             "      or by performing Alt + Tab with the game set to Fullscreen mode." );
    }

    if (ImGui::CollapsingHeader ("Framerate Control", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
    {
      int limiter =
        config.framerate.replace_limiter ? 1 : 0;

      const char* szLabel = (limiter == 0 ? "Framerate Limiter  (choose something else!)" : "Framerate Limiter");

      ImGui::Combo (szLabel, &limiter, "Namco          (A.K.A. Stutterfest 2017)\0"
                                       "Special K      (Precision Timing For The Win!)\0\0" );

      static float values [120]  = { 0 };
      static int   values_offset =   0;

      values [values_offset] = 1000.0f * ImGui::GetIO ().DeltaTime;
      values_offset = (values_offset + 1) % IM_ARRAYSIZE (values);

      if (limiter != config.framerate.replace_limiter)
      {
        config.framerate.replace_limiter = limiter;

        if (config.framerate.replace_limiter)
          tbf::FrameRateFix::BlipFramerate    ();
        else
          tbf::FrameRateFix::DisengageLimiter ();

        float fZero = 0.0f;
        memset (values, *(reinterpret_cast <DWORD *> (&fZero)), sizeof (float) * 120);
        values_offset = 0;
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

      static char szAvg [512];

      sprintf ( szAvg,
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
                                       ImVec2 (0, 80) );

      ImGui::SameLine ();

      if (! config.framerate.replace_limiter) {
        ImGui::TextColored ( ImVec4 (1.0f, 1.0f, 0.0f, 1.0f),
                               "\n"
                               "\n"
                               " ... working limiters do not resemble EKGs!" );
      } else {
        ImGui::TextColored ( ImVec4 ( 0.2f, 1.0f, 0.2f, 1.0f),
                               "\n"
                               "\n"
                               "This is how a framerate limiter should work." );
      }

      //ImGui::Text ( "Application average %.3f ms/frame (%.1f FPS)",
                      //1000.0f / ImGui::GetIO ().Framerate,
                                //ImGui::GetIO ().Framerate );
    }

    if (ImGui::CollapsingHeader ("Texture Options"))
    {
      if (ImGui::Checkbox ("Dump Textures",    &config.textures.dump))     tbf::RenderFix::need_reset.graphics = true;
      if (ImGui::Checkbox ("Generate Mipmaps", &config.textures.remaster)) tbf::RenderFix::need_reset.graphics = true;

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates distant texture aliasing");

      if (config.textures.remaster) {
        ImGui::SameLine (150); 

        if (ImGui::Checkbox ("(Uncompressed)", &config.textures.uncompressed)) tbf::RenderFix::need_reset.graphics = true;

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Uses more VRAM, but avoids texture compression artifacts on generated mipmaps.");
      }

      ImGui::SliderFloat ("LOD Bias", &config.textures.lod_bias, -3.0f, config.textures.uncompressed ? 16.0f : 3.0f);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Controls texture sharpness;  -3 = Sharpest (WILL shimmer),  0 = Neutral,  16 = Laughably blurry");
        ImGui::EndTooltip   ();
      }


      ImGui::Checkbox ("Display Texture Resampling Indicator in OSD", &config.textures.show_loading_text);
    }

#if 0
    if (ImGui::CollapsingHeader ("Shader Options"))
    {
      ImGui::Checkbox ("Dump Shaders", &config.render.dump_shaders);
    }
#endif

    if (ImGui::CollapsingHeader ("Shadow Quality"))
    {
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

      ImGui::Combo ("Character Shadow Resolution",     &shadows.radio,     "Normal\0Enhanced\0High\0Ultra\0\0");
      ImGui::Combo ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0High\0Ultra\0\0");

      ImGui::TextColored ( ImVec4 ( 0.999f, 0.01f, 0.999f, 1.0f),
                             " * Changes to these settings will produce weird results until you change Screen Mode in-game...\n" );

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
    }

    if (ImGui::CollapsingHeader ("Audio Configuration"))
    {
      ImGui::Checkbox ("Enable 7.1 Channel Audio Fix", &config.audio.enable_fix);

      if (config.audio.enable_fix) {
        ImGui::RadioButton ("Stereo",       (int *)&config.audio.channels, 2);
        ImGui::RadioButton ("Quadraphonic", (int *)&config.audio.channels, 4);
        ImGui::RadioButton ("5.1 Surround", (int *)&config.audio.channels, 6);
      }
    }

    if (ImGui::Button ("Gamepad Config"))
      ImGui::OpenPopup ("Gamepad Config");

    TBFix_GamepadConfigDlg ();

    ImGui::SameLine ();

    //if (ImGui::Button ("Special K Config"))
      //show_gamepad_config ^= 1;

    if ( ImGui::Checkbox ("Pause Game While This Menu Is Open", &config.input.ui.pause) )
      TBFix_PauseGame (config.input.ui.pause);

    ImGui::SameLine (500);

    if (ImGui::Selectable ("...", show_test_window))
      show_test_window = (! show_test_window);

    ImGui::End ();

    if (show_test_window) {
      ImGui::SetNextWindowPos (ImVec2 (650, 20), ImGuiSetCond_FirstUseEver);
      ImGui::ShowTestWindow   (&show_test_window);
    }

  // Rendering
  tbf::RenderFix::pDevice->SetRenderState (D3DRS_ZENABLE,           false);
  tbf::RenderFix::pDevice->SetRenderState (D3DRS_ALPHABLENDENABLE,  false);
  tbf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, false);

  if ( SUCCEEDED (
         tbf::RenderFix::pDevice->BeginScene ()
       )
     )
  {
    ImGui::Render                     ();
    tbf::RenderFix::pDevice->EndScene ();
  }
}