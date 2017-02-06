#define NOMINMAX

#include "DLL_VERSION.H"
#include "imgui\imgui.h"

#include <string>
#include <vector>
#include <algorithm>

#include "config.h"
#include "command.h"
#include "render.h"
#include "textures.h"

bool
TBFix_TextureModDlg (void)
{
  bool show_dlg = true;

  ImGui::Begin ( "Tales Engine Texture Mod Toolkit (v " TBF_VERSION_STR_A ")",
                   &show_dlg,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

  ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

  if (ImGui::CollapsingHeader ("Preliminary Documentation", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::BeginChild ("ModDescription", ImVec2 (750, 325), true);
      ImGui::TextColored    (ImVec4 (0.9f, 0.7f, 0.5f, 1.0f), "Texture Modding Overview"); ImGui::SameLine ();
      ImGui::Text           ("    (Full Writeup Pending)");

      ImGui::Separator      ();

      ImGui::TextWrapped    ("\nReplacement textures go in (TBFix_Res\\inject\\textures\\{blocking|streaming}\\<checksum>.dds)\n\n");

      ImGui::TreePush ("");
        ImGui::BulletText ("Blocking textures have a high performance penalty, but zero chance of visible pop-in.");
        ImGui::BulletText ("Streaming textures will replace the game's original texture whenever Disk/CPU loads finish.");
        ImGui::TreePush   ("");
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.6f, 0.9f, 0.2f, 1.0f));
          ImGui::BulletText     ("Use streaming whenever possible or performance will bite you in the ass.");
          ImGui::PopStyleColor  ();
        ImGui::TreePop    (  );
      ImGui::TreePop  ();

      ImGui::TextWrapped    ("\n\nLoading modified textures from separate files is inefficient; entire groups of textures may also be packaged into \".7z\" files (See TBFix_Res\\inject\\00_License.7z as an example, and use low/no compression ratio or you will kill the game's performance).\n");

      ImGui::Separator      ();

      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.6f, 0.3f, 1.0f));
      ImGui::TextWrapped    ( "\n\nA more detailed synopsis will follow in future versions, for now please refer to the GitHub release notes for Tales of Symphonia "
                              "\"Fix\" v 0.9.0 for a thorough description on authoring texture mods.\n\n" );
      ImGui::PopStyleColor  ();

      ImGui::Separator      ();

      ImGui::Bullet         (); ImGui::SameLine ();
      ImGui::TextWrapped    ( "If texture mods are enabled, you can click on the Injected and Base buttons on the texture cache "
                                "summary pannel to compare modified and unmodified." );
    ImGui::EndChild         ();
  }

  if (ImGui::CollapsingHeader ("Texture Selection", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    static LONG last_ht    = 256L;
    static LONG last_width = 256L;

    static std::vector <std::string> list_contents;
    static bool                      list_dirty     = false;
    static int                       sel            =     0;

    extern std::vector <uint32_t> textures_used_last_dump;
    extern              uint32_t  tex_dbg_idx;
    extern              uint32_t  debug_tex_id;

    ImGui::BeginChild ("ToolHeadings", ImVec2 (750, 32), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

    if (ImGui::Button ("Refresh Textures"))
    {
      SK_ICommandProcessor& command =
        *SK_GetCommandProcessor ();

      command.ProcessCommandLine ("Textures.Trace true");

      tbf::RenderFix::tex_mgr.updateOSD ();

      list_dirty = true;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Refreshes the set of texture checksums used in the last frame drawn.");

    ImGui::SameLine ();

    if (ImGui::Button ("Clear Debug"))
    {
      sel                         = -1;
      debug_tex_id                =  0;
      textures_used_last_dump.clear ();
      last_ht                     =  0;
      last_width                  =  0;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Enable On-Demand Texture Dumping",    &config.textures.on_demand_dump)) tbf::RenderFix::need_reset.graphics = true;
    
    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.7f, 0.3f, 1.f), "Enable dumping DXT compressed textures from VRAM.");
      ImGui::Separator    ();
      ImGui::Text         ("Drivers may not be able to manage texture memory as efficiently, and you should turn this option off when not modifying textures.\n\n");
      ImGui::BulletText   ("If this is your first time enabling this feature, the dump button will not work until you reload all textures in-game.");
      ImGui::EndTooltip   ();
    }

    ImGui::Separator ();
    ImGui::EndChild  ();

    if (list_dirty)
    {
           list_contents.clear ();
                sel = tex_dbg_idx;

      for ( auto it : textures_used_last_dump )
      {
        char szDesc [16] = { };

        sprintf (szDesc, "%08x", it);

        list_contents.push_back (szDesc);
      }
    }

    ImGui::BeginGroup ();

    ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

    ImGui::BeginChild ( "Item List",
                        ImVec2 ( 90, std::max (512L, last_ht) + 128 ),
                          true, ImGuiWindowFlags_AlwaysAutoResize );

   if (textures_used_last_dump.size ())
   {
     static      int last_sel = 0;
     static bool sel_changed  = false;

     if (sel != last_sel)
       sel_changed = true;

     last_sel = sel;

     for ( int line = 0; line < textures_used_last_dump.size (); line++)
     {
       if (line == sel)
       {
         bool selected = true;
         ImGui::Selectable (list_contents [line].c_str (), &selected);

         if (sel_changed)
         {
           ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
           sel_changed = false;
         }
       }

       else
       {
         bool selected = false;

         if (ImGui::Selectable (list_contents[line].c_str (), &selected))
         {
           sel_changed = true;
           tex_dbg_idx                 =  line;
           sel                         =  line;
           debug_tex_id                =  textures_used_last_dump [line];
         }
       }
     }
   }

   ImGui::EndChild ();

   if (ImGui::IsItemHovered ())
   {
     ImGui::BeginTooltip ();
     ImGui::TextColored (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "The \"debug\" texture will appear black to make identifying textures to modify easier.");
     ImGui::Separator  ();
     ImGui::BulletText ("Press Ctrl + Shift + [ to select the previous texture from this list");
     ImGui::BulletText ("Press Ctrl + Shift + ] to select the next texture from this list");
     ImGui::EndTooltip ();
   }

   ImGui::SameLine ();
   ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

   last_ht    = std::max (last_ht,    16L);
   last_width = std::max (last_width, 16L);

   ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));

   ImGui::BeginChild ( "Item Selection",
                       ImVec2 (std::max (256L, last_width + 24), std::max (512L, last_ht) + 128), true, ImGuiWindowFlags_AlwaysAutoResize );

    if (debug_tex_id != 0x00)
    {
      tbf::RenderFix::Texture* pTex =
        tbf::RenderFix::tex_mgr.getTexture (debug_tex_id);

      if (pTex != nullptr)
      {
         D3DSURFACE_DESC desc;

         if (SUCCEEDED (pTex->d3d9_tex->pTex->GetLevelDesc (0, &desc)))
         {
           last_width  = desc.Width;
           last_ht     = desc.Height;

           extern std::wstring
           SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


           ImGui::Text ( "Dimensions:   %lux%lu",
                           desc.Width, desc.Height/*,
                             pTex->d3d9_tex->GetLevelCount ()*/ );
           ImGui::Text ( "Format:       %ws",
                           SK_D3D9_FormatToStr (desc.Format).c_str () );
           ImGui::Text ( "Data Size:    %.2f MiB",
                           (double)pTex->d3d9_tex->tex_size / (1024.0f * 1024.0f) );
           ImGui::Text ( "Load Time:    %.6f Seconds",
                           pTex->load_time / 1000.0f );

           ImGui::Separator     ();

           if (! TBF_IsTextureDumped (debug_tex_id))
           {
             if ( ImGui::Button ("Dump Texture to Disk") )
             {
               TBF_DumpTexture (desc.Format, debug_tex_id, pTex->d3d9_tex->pTex);
             }
           }

           else
           {
             if ( ImGui::Button ("Delete Dumped Texture from Disk") )
             {
               TBF_DeleteDumpedTexture (desc.Format, debug_tex_id);
             }
           }

           ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
           ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
           ImGui::Image           ( pTex->d3d9_tex->pTex,
                                      ImVec2 ((float)desc.Width, (float)desc.Height),
                                        ImVec2  (0,0),             ImVec2  (1,1),
                                        ImColor (255,255,255,255), ImColor (255,255,255,128)
                                  );
           ImGui::EndChildFrame   ();
           ImGui::PopStyleColor   (1);
         }
      }
    }
    ImGui::EndChild      ();
    ImGui::PopStyleColor (2);

    if (debug_tex_id != 0x00)
    {
      tbf::RenderFix::Texture* pTex =
        tbf::RenderFix::tex_mgr.getTexture (debug_tex_id);

      if (pTex != nullptr && pTex->d3d9_tex->pTexOverride != nullptr)
      {
        ImGui::SameLine ();

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));

        ImGui::BeginChild ( "Item Selection2",
                            ImVec2 (std::max (256L, last_width + 24), std::max (512L, last_ht) + 128), true, ImGuiWindowFlags_AlwaysAutoResize );

         D3DSURFACE_DESC desc;

         if (SUCCEEDED (pTex->d3d9_tex->pTexOverride->GetLevelDesc (0, &desc)))
         {
           last_width  = desc.Width;
           last_ht     = desc.Height;

           extern std::wstring
           SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


           ImGui::Text ( "Dimensions:   %lux%lu",
                           desc.Width, desc.Height/*,
                             pTex->d3d9_tex->GetLevelCount ()*/ );
           ImGui::Text ( "Format:       %ws",
                           SK_D3D9_FormatToStr (desc.Format).c_str () );
           ImGui::Text ( "Data Size:    %.2f MiB",
                           (double)pTex->d3d9_tex->override_size / (1024.0f * 1024.0f) );
           ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), "{Injected} or {Resampled} Texture" );

           ImGui::Separator     ();

           if ( ImGui::Button ("TODO: Reload Button") )
           {
             //TBF_DumpTexture (desc.Format, debug_tex_id, pTex->d3d9_tex->pTex);
           }

           ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
           ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
           ImGui::Image           ( pTex->d3d9_tex->pTexOverride,
                                      ImVec2 ((float)desc.Width, (float)desc.Height),
                                        ImVec2  (0,0),             ImVec2  (1,1),
                                        ImColor (255,255,255,255), ImColor (255,255,255,128)
                                  );
           ImGui::EndChildFrame   ();
           ImGui::PopStyleColor   (1);
         }
        ImGui::EndChild      ();
      }
    }
    ImGui::PopStyleVar   (1);
    ImGui::EndGroup      ();
  }

  if (ImGui::CollapsingHeader ("Misc. Settings"))
  {
    ImGui::TreePush ("");
    if (ImGui::Checkbox ("Dump ALL Textures  (TBFix_Res\\dump\\textures\\<format>\\*.dds)",    &config.textures.dump))     tbf::RenderFix::need_reset.graphics = true;

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Enabling this will cause the game to run slower and waste disk space, only enable if you know what you are doing.");
      ImGui::EndTooltip   ();
    }

    ImGui::TreePop ();
  }

  ImGui::PopItemWidth ();
  ImGui::End          ();

  return show_dlg;
}