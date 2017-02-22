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

#include "render.h"
#include "framerate.h"
#include "config.h"
#include "log.h"
#include "scanner.h"

#include "textures.h"

#include <stdint.h>

#include <d3d9.h>
#include <d3d9types.h>

#include <atlbase.h>

tbf::RenderFix::tbf_draw_states_s
  tbf::RenderFix::draw_state;

extern uint32_t current_tex [256];

struct smaa_constants_s
{
  struct
  {
    float inv_x, inv_y,
          width, height;
  } metrics;

  struct
  {
    float threshold,
          max_search_steps, max_search_steps_diag,
          corner_rounding;
  } general;

  struct
  {
    float threshold, scale, strength;
  } predication;

  struct
  {
    float weight;
  } reprojection;
};


typedef HRESULT (STDMETHODCALLTYPE *SetRenderState_pfn)
(
  IDirect3DDevice9*  This,
  D3DRENDERSTATETYPE State,
  DWORD              Value
);

typedef HRESULT (STDMETHODCALLTYPE *TestCooperativeLevel_pfn)
(
  IDirect3DDevice9* This
);

extern SetRenderState_pfn D3D9SetRenderState_Original;

TestCooperativeLevel_pfn                D3D9TestCooperativeLevel_Original            = nullptr;
DrawPrimitive_pfn                       D3D9DrawPrimitive_Original                   = nullptr;
DrawIndexedPrimitive_pfn                D3D9DrawIndexedPrimitive_Original            = nullptr;
DrawPrimitiveUP_pfn                     D3D9DrawPrimitiveUP_Original                 = nullptr;
DrawIndexedPrimitiveUP_pfn              D3D9DrawIndexedPrimitiveUP_Original          = nullptr;
SetStreamSource_pfn                     D3D9SetStreamSource_Original                 = nullptr;
SK_SetPresentParamsD3D9_pfn             SK_SetPresentParamsD3D9_Original             = nullptr;


extern bool pending_loads            (void);
extern void TBFix_LoadQueuedTextures (void);


enum reset_stage_s {
  Initiate = 0x0, // Fake device loss
  Respond  = 0x1, // Fake device not reset
  Clear    = 0x2  // Return status to normal
} trigger_reset;

bool needs_aspect     = false;

uint32_t
TBF_MakeShadowBitShift (uint32_t dim)
{
  uint32_t shift = abs (config.render.shadow_rescale);

  // If this is 64x64 and we want all shadows the same resolution, then add
  //   an extra shift.
  shift += ((config.render.shadow_rescale) < 0L) * (dim == 64UL);

  return shift;
}


void
TBF_ComputeAspectCoeffs (float& x, float& y, float& xoff, float& yoff)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction || config.render.blackbar_videos))
    return;

  float rescale = (1.77777778f / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    int width = (int)((16.0f / 9.0f) * tbf::RenderFix::height);
    int x_off = (tbf::RenderFix::width - width) / 2;

    x    = (float)tbf::RenderFix::width / (float)width;
    xoff = (float)x_off;
  } else {
    int height = (int)((9.0f / 16.0f) * tbf::RenderFix::width);
    int y_off  = (tbf::RenderFix::height - height) / 2;

    y    = (float)tbf::RenderFix::height / (float)height;
    yoff = (float)y_off;
  }
}

#include "hook.h"

typedef HRESULT (STDMETHODCALLTYPE *SetSamplerState_pfn)
(IDirect3DDevice9*   This,
  DWORD               Sampler,
  D3DSAMPLERSTATETYPE Type,
  DWORD               Value);

SetSamplerState_pfn D3D9SetSamplerState_Original = nullptr;

LPVOID SetSamplerState = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetSamplerState_Detour (IDirect3DDevice9*   This,
                            DWORD               Sampler,
                            D3DSAMPLERSTATETYPE Type,
                            DWORD               Value)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9SetSamplerState_Original (This, Sampler, Type, Value);

  //dll_log->Log ( L" [!] IDirect3DDevice9::SetSamplerState (%lu, %lu, %lu)",
                   //Sampler, Type, Value );

  if ( Type == D3DSAMP_MIPMAPLODBIAS )
  {
    //dll_log->Log (L" [!] IDirect3DDevice9::SetSamplerState (...)");

    if (Type == D3DSAMP_MIPMAPLODBIAS)
    {
      float fMax = config.fun_stuff.plastic_mode ? 20.0f : config.textures.lod_bias;

      Value = *reinterpret_cast <DWORD *> (&fMax);
    }
  }

  return D3D9SetSamplerState_Original (This, Sampler, Type, Value);
}

IDirect3DVertexShader9* g_pVS;
IDirect3DPixelShader9*  g_pPS;

static uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
crc32(uint32_t crc, const void *buf, size_t size)
{
  const uint8_t *p;

  p = (uint8_t *)buf;
  crc = crc ^ ~0U;

  while (size--)
    crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ ~0U;
}

#include <map>

// For now, let's just focus on stream0 and pretend nothing else exists...
IDirect3DVertexBuffer9* vb_stream0 = nullptr;

std::unordered_map <uint32_t, tbf::RenderFix::shader_disasm_s> vs_disassembly;
std::unordered_map <uint32_t, tbf::RenderFix::shader_disasm_s> ps_disassembly;

std::unordered_map <LPVOID, uint32_t>      vs_checksums;
std::unordered_map <LPVOID, uint32_t>      ps_checksums;

tbf::RenderFix::frame_state_s tbf::RenderFix::last_frame;


// Store the CURRENT shader's checksum instead of repeatedly
//   looking it up in the above hashmaps.
uint32_t vs_checksum = 0;
uint32_t ps_checksum = 0;

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD  (        QueryInterface)   (THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_ (ULONG,  AddRef)           (THIS) PURE;
    STDMETHOD_ (ULONG,  Release)          (THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_ (LPVOID, GetBufferPointer) (THIS) PURE;
    STDMETHOD_ (DWORD,  GetBufferSize)    (THIS) PURE;
};

typedef HRESULT (WINAPI *D3DXGetShaderConstantTable_pfn)(
  _In_  const DWORD                *pFunction,
  _Out_       LPD3DXCONSTANTTABLE *ppConstantTable
);

typedef HRESULT (WINAPI *D3DXDisassembleShader_pfn)(
  _In_  const DWORD         *pShader,
  _In_        BOOL            EnableColorCode,
  _In_        LPCSTR         pComments,
  _Out_       LPD3DXBUFFER *ppDisassembly
);

D3DXDisassembleShader_pfn D3DXDisassembleShader = nullptr;

void
SK_D3D9_DumpShader ( const wchar_t* wszPrefix,
                           uint32_t crc32,
                           LPVOID   pbFunc )
{
  static bool dump = config.render.dump_shaders;

  if (dump)
  {
    if (GetFileAttributes (L"TBFix_Res\\dump\\shaders") !=
         FILE_ATTRIBUTE_DIRECTORY) {
      CreateDirectoryW (L"TBFix_Res",                nullptr);
      CreateDirectoryW (L"TBFix_Res\\dump",          nullptr);
      CreateDirectoryW (L"TBFix_Res\\dump\\shaders", nullptr);
    }

    wchar_t wszDumpName [MAX_PATH] = { L'\0' };

    swprintf_s ( wszDumpName,
                   MAX_PATH, 
                     L"TBFix_Res\\dump\\shaders\\%s_%08x.html",
                       wszPrefix, crc32 );

    if ( D3DXDisassembleShader           != nullptr &&
         GetFileAttributes (wszDumpName) == INVALID_FILE_ATTRIBUTES )
    {
      CComPtr <ID3DXBuffer> pDisasm = nullptr;

      HRESULT hr =
        D3DXDisassembleShader ((DWORD *)pbFunc, TRUE, "", &pDisasm);

      if (SUCCEEDED (hr))
      {
        FILE* fDump = _wfsopen (wszDumpName,  L"wb", _SH_DENYWR);

        if (fDump != NULL) {
          fwrite ( pDisasm->GetBufferPointer (),
                     pDisasm->GetBufferSize  (),
                       1,
                         fDump );
          fclose (fDump);
        }
      }
    }
  }

  CComPtr <ID3DXBuffer> pDisasm = nullptr;

  HRESULT hr =
    D3DXDisassembleShader ((DWORD *)pbFunc, FALSE, "", &pDisasm);

  if (SUCCEEDED (hr) && strlen ((const char *)pDisasm->GetBufferPointer ()))
  {
    char* szDisasm = strdup ((const char *)pDisasm->GetBufferPointer ());

    char* comments_end  =                strstr (szDisasm,          "\n ");
    char* footer_begins = comments_end ? strstr (comments_end + 1, "\n\n") : nullptr;

    if (comments_end)  *comments_end  = '\0'; else (comments_end  = "  ");
    if (footer_begins) *footer_begins = '\0'; else (footer_begins = "  ");

    if (! wcsicmp (wszPrefix, L"ps"))
    {
      ps_disassembly.emplace ( crc32, tbf::RenderFix::shader_disasm_s {
                                        szDisasm,
                                          comments_end + 1,
                                            footer_begins + 1 }
                             );
    }

    else
    {
      vs_disassembly.emplace ( crc32, tbf::RenderFix::shader_disasm_s {
                                        szDisasm,
                                          comments_end + 1,
                                            footer_begins + 1 }
                             );
    }

    free (szDisasm);
  }
}


typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_pfn)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

SetVertexShader_pfn D3D9SetVertexShader_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Detour (IDirect3DDevice9*       This,
                            IDirect3DVertexShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9SetVertexShader_Original (This, pShader);

#if 0
  static DWORD dwLastTid = GetCurrentThreadId ();

  if (dwLastTid != GetCurrentThreadId ()) {
    dll_log->Log ( L"[   D3D9   ]  >> WARNING:  Multi-threaded Rendering "
                                     L" {SetVertexShader}  "
                                     L"(last_tid=%x, new_tid=%x, r_tid=%x)",
                 dwLastTid, GetCurrentThreadId (), tbf::RenderFix::dwRenderThreadID );
    dwLastTid = GetCurrentThreadId ();
  }
#endif

  if (GetCurrentThreadId () != InterlockedExchangeAdd (&tbf::RenderFix::dwRenderThreadID, 0))
    return D3D9SetVertexShader_Original (This, pShader);


  if (g_pVS != pShader)
  {
    if (pShader != nullptr)
    {
      if (vs_checksums.find (pShader) == vs_checksums.end ())
      {
        UINT len;
        pShader->GetFunction (nullptr, &len);

        void* pbFunc = malloc (len);

        if (pbFunc != nullptr)
        {
          pShader->GetFunction (pbFunc, &len);

          vs_checksums [pShader] = crc32 (0, pbFunc, len);

          SK_D3D9_DumpShader (L"vs", vs_checksums [pShader], pbFunc);

          free (pbFunc);
        }
      }
    }

    else
      vs_checksum = 0;
  }

  vs_checksum = vs_checksums [pShader];
  g_pVS       = pShader;

  if (vs_checksum != 0x00)
  {
    tbf::RenderFix::last_frame.vertex_shaders.emplace (vs_checksum);
    
    if (tbf::RenderFix::tracked_rt.active)
      tbf::RenderFix::tracked_rt.vertex_shaders.emplace (vs_checksum);
    
    if (vs_checksum == tbf::RenderFix::tracked_vs.crc32)
    {
      tbf::RenderFix::tracked_vs.use (pShader);
    
      for (int i = 0; i < 16; i++)
        tbf::RenderFix::tracked_vs.current_textures [i] = 0x0;
    }
  }

  return D3D9SetVertexShader_Original (This, pShader);
}


typedef HRESULT (STDMETHODCALLTYPE *SetPixelShader_pfn)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

SetPixelShader_pfn D3D9SetPixelShader_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShader_Detour (IDirect3DDevice9*      This,
                           IDirect3DPixelShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9SetPixelShader_Original (This, pShader);

#if 0
  static DWORD dwLastTid = GetCurrentThreadId ();

  if (dwLastTid != GetCurrentThreadId ()) {
    dll_log->Log ( L"[   D3D9   ]  >> WARNING:  Multi-threaded Rendering "
                                     L" {SetPixelShader }  "
                                     L"(last_tid=%x, new_tid=%x, r_tid=%x)",
                 dwLastTid, GetCurrentThreadId (), tbf::RenderFix::dwRenderThreadID );
    dwLastTid = GetCurrentThreadId ();
  }
#endif

  if (GetCurrentThreadId () != InterlockedExchangeAdd (&tbf::RenderFix::dwRenderThreadID, 0))
    return D3D9SetPixelShader_Original (This, pShader);


  if (g_pPS != pShader)
  {
    if (pShader != nullptr)
    {
      if (ps_checksums.find (pShader) == ps_checksums.end ())
      {
        UINT len;
        pShader->GetFunction (nullptr, &len);

        void* pbFunc = malloc (len);

        if (pbFunc != nullptr)
        {
          pShader->GetFunction (pbFunc, &len);

          ps_checksums [pShader] = crc32 (0, pbFunc, len);

          SK_D3D9_DumpShader (L"ps", ps_checksums [pShader], pbFunc);

          free (pbFunc);
        }
      }
    }

    else
      ps_checksum = 0;
  }

  ps_checksum = ps_checksums [pShader];
  g_pPS       = pShader;

  if (ps_checksum != 0x00)
  {
    tbf::RenderFix::last_frame.pixel_shaders.emplace (ps_checksum);
    
    if (tbf::RenderFix::tracked_rt.active)
      tbf::RenderFix::tracked_rt.pixel_shaders.emplace (ps_checksum);
    
    if (ps_checksum == tbf::RenderFix::tracked_ps.crc32)
    {
      tbf::RenderFix::tracked_ps.use (pShader);
    
      for (int i = 0; i < 16; i++)
        tbf::RenderFix::tracked_ps.current_textures [i] = 0x0;
    }
  }

  return D3D9SetPixelShader_Original (This, pShader);
}


//
// Bink Video Vertex Shader
//
const uint32_t VS_CHECKSUM_BINK  = 3463109298UL;

const uint32_t PS_CHECKSUM_UI    =  363447431UL;
const uint32_t VS_CHECKSUM_UI    =  657093040UL;

bool
TBF_ShouldSkipRenderPass (void)
{
  const bool tracked_vs = ( vs_checksum == tbf::RenderFix::tracked_vs.crc32         );
  const bool tracked_ps = ( ps_checksum == tbf::RenderFix::tracked_ps.crc32         );
  const bool tracked_vb = { vb_stream0  == tbf::RenderFix::tracked_vb.vertex_buffer };

  if (tracked_vs)
  {
    tbf::RenderFix::tracked_vs.num_draws++;

    for (int i = 0; i < 16; i++)
      if (tbf::RenderFix::tracked_vs.current_textures [i] != 0)
        tbf::RenderFix::tracked_vs.used_textures.emplace (tbf::RenderFix::tracked_vs.current_textures [i]);


    //
    // TODO: Make generic and move into class -- must pass shader type to function
    //
    for ( auto&& it : tbf::RenderFix::tracked_vs.constants )
    {
      for ( auto&& it2 : it.struct_members )
      {
        if ( it2.Override ) 
          tbf::RenderFix::pDevice->SetVertexShaderConstantF ( it2.RegisterIndex, it2.Data, 1 );
      }

      if ( it.Override ) 
        tbf::RenderFix::pDevice->SetVertexShaderConstantF ( it.RegisterIndex, it.Data, 1 );
    }
  }

  if (tracked_ps)
  {
    tbf::RenderFix::tracked_ps.num_draws++;

    for (int i = 0; i < 16; i++)
      if (tbf::RenderFix::tracked_ps.current_textures [i] != 0)
        tbf::RenderFix::tracked_ps.used_textures.emplace (tbf::RenderFix::tracked_ps.current_textures [i]);

    //
    // TODO: Make generic and move into class -- must pass shader type to function
    //
    for ( auto&& it : tbf::RenderFix::tracked_ps.constants )
    {
      for ( auto&& it2 : it.struct_members )
      {
        if ( it2.Override ) 
          tbf::RenderFix::pDevice->SetPixelShaderConstantF ( it2.RegisterIndex, it2.Data, 1 );
      }

      if ( it.Override ) 
        tbf::RenderFix::pDevice->SetPixelShaderConstantF ( it.RegisterIndex, it.Data, 1 );
    }
  }


  bool clamp   = false;
  bool sharpen = false;

  if (ps_checksum == tbf::RenderFix::tracked_ps.crc32 && tbf::RenderFix::tracked_ps.clamp_coords)
    clamp = true;

  if (vs_checksum == tbf::RenderFix::tracked_vs.crc32 && tbf::RenderFix::tracked_vs.clamp_coords)
    clamp = true;

  if (config.textures.keep_ui_sharp && ps_checksum == 0x17c397fb) sharpen = true;

  if (                                                              clamp ||
       ( config.textures.clamp_skit_coords && ps_checksum == 0x872e7c85 ) ||
       ( config.textures.clamp_map_coords  && ps_checksum == 0xc954a649 ) )
  {
    sharpen = true;

    D3D9SetSamplerState_Original (tbf::RenderFix::pDevice, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    D3D9SetSamplerState_Original (tbf::RenderFix::pDevice, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    D3D9SetSamplerState_Original (tbf::RenderFix::pDevice, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
  }

  if (sharpen)
  {
    float fMin = -3.0f;
    D3D9SetSamplerState_Original (tbf::RenderFix::pDevice, 0, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast <DWORD *>(&fMin) );
  }




  if (tracked_vb)
  {
    tbf::RenderFix::tracked_vb.num_draws++;

    if (tbf::RenderFix::tracked_vb.wireframe)
      tbf::RenderFix::pDevice->SetRenderState (D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }

  else
  {
    if (tbf::RenderFix::tracked_vb.wireframe)
      tbf::RenderFix::pDevice->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);
  }

  if (tracked_vb && tbf::RenderFix::tracked_vb.cancel_draws)
    return true;


  // Do these sparate so that we can accurately count used textures even on cancelled passes.
  if (tracked_vs && tbf::RenderFix::tracked_vs.cancel_draws)
    return true;

  if (tracked_ps && tbf::RenderFix::tracked_ps.cancel_draws)
    return true;


  if (config.fun_stuff.hollow_eye_mode && vs_checksum == config.fun_stuff.hollow_eye_vs_crc32)
    return true;

  if (config.fun_stuff.disable_pause_dim && ps_checksum == config.fun_stuff.pause_dim_ps_crc32)
    return true;

  if (config.fun_stuff.disable_smoke   && ps_checksum == config.fun_stuff.smoke_ps_crc32 && current_tex [0] == 0x16f618a8)
    return true;



  if (config.render.validation) {
    DWORD dwPasses = 0;
    HRESULT hr = tbf::RenderFix::pDevice->ValidateDevice (&dwPasses);
    
    if (hr != S_OK) {
      dll_log->Log (L"[D3D9Valid] D3D9 Validation Failure: %x", hr);
      dll_log->Log (L"[D3D9Valid]  Current vs: %x, Current ps: %x",
                      vs_checksum, ps_checksum );
    }
  }


  return false;
}

typedef void (STDMETHODCALLTYPE *SK_BeginBufferSwap_pfn)(void);
SK_BeginBufferSwap_pfn SK_BeginBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *SK_EndBufferSwap_pfn)
  (HRESULT   hr,
   IUnknown* device);
SK_EndBufferSwap_pfn SK_EndBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *SetScissorRect_pfn)(
  IDirect3DDevice9* This,
  const RECT*       pRect);

SetScissorRect_pfn D3D9SetScissorRect_Original = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *EndScene_pfn)
(IDirect3DDevice9* This);

EndScene_pfn D3D9EndScene_Original = nullptr;

int draw_count  = 0;
int next_draw   = 0;
int scene_count = 0;

std::string mod_text;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Detour (IDirect3DDevice9* This)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9EndScene_Original (This);

  if (GetCurrentThreadId () != InterlockedExchangeAdd (&tbf::RenderFix::dwRenderThreadID, 0))
    return D3D9EndScene_Original (This);

  // EndScene is invoked multiple times per-frame, but we
  //   are only interested in the first.
  if (scene_count++ > 0)
    return D3D9EndScene_Original (This);

  if (pending_loads ()) {
    TBFix_LoadQueuedTextures ();
  }

  tbf::RenderFix::draw_state.cegui_active = true;

  typedef BOOL (__stdcall *SKX_DrawExternalOSD_pfn)(const char* szAppName, const char* szText);

  static HMODULE               hMod =
    GetModuleHandle (config.system.injector.c_str ());
  static SKX_DrawExternalOSD_pfn SKX_DrawExternalOSD
    =
    (SKX_DrawExternalOSD_pfn)GetProcAddress (hMod, "SKX_DrawExternalOSD");

  if (config.render.osd_disclaimer)
  {
    SKX_DrawExternalOSD ("ToBFix", "\n"
                                   "  Press Ctrl + Shift + O         to toggle In-Game OSD\n"
                                   "  Press Ctrl + Shift + Backspace to access In-Game Config Menu\n\n"
                                   "   * This message will go away the first time you actually read it and successfully toggle the OSD.");
  }

  else
  {
    extern bool  __show_cache;
    extern DWORD last_queue_update;

    if (last_queue_update + 250 < timeGetTime ())
      mod_text = "";

    if (__show_cache)
    {
      std::string output;

      output  = "Texture Cache\n";
      output += "-------------\n";
      output += tbf::RenderFix::tex_mgr.osdStats ();

      if (! mod_text.empty ()) {
        output += "\n";
        output += mod_text;
      }

      SKX_DrawExternalOSD ("ToBFix", output.c_str ());

      output = "";
    }

    else if (config.textures.show_loading_text)
      SKX_DrawExternalOSD ("ToBFix", mod_text.c_str ());

    else
      SKX_DrawExternalOSD ("ToBFix", "");
  }

  HRESULT hr = D3D9EndScene_Original (This);

  game_state.in_skit = false;

  needs_aspect       = false;
  draw_count         = 0;
  next_draw          = 0;

  g_pPS              = nullptr;
  g_pVS              = nullptr;
  vs_checksum        = 0;
  ps_checksum        = 0;

  return hr;
}

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
D3D9EndFrame_Pre (void)
{
  if (GetCurrentThreadId () != InterlockedExchangeAdd (&tbf::RenderFix::dwRenderThreadID, 0))
    return SK_BeginBufferSwap();

  void TBFix_LogUsedTextures (void);
  TBFix_LogUsedTextures ();

  //if (! config.framerate.minimize_latency)
    //tbf::FrameRateFix::RenderTick ();

  if ( config.framerate.replace_limiter &&
       tbf::FrameRateFix::variable_speed_installed ) {
    tbf::FrameRateFix::RenderTick ();
  }

  SK_BeginBufferSwap ();

  tbf::RenderFix::draw_state.cegui_active = false;
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndFrame_Post (HRESULT hr, IUnknown* device)
{
  // Ignore anything that's not the primary render device.
  if (device != tbf::RenderFix::pDevice)
    return SK_EndBufferSwap (hr, device);

  scene_count = 0;

  InterlockedExchange (&tbf::RenderFix::dwRenderThreadID, GetCurrentThreadId ());

  if (trigger_reset == reset_stage_s::Clear)
    hr = SK_EndBufferSwap (hr, device);
  else
    hr = D3DERR_DEVICELOST;

  tbf::RenderFix::tex_mgr.resetUsedTextures       ();

  tbf::RenderFix::last_frame.clear ();
  tbf::RenderFix::tracked_rt.clear ();
  tbf::RenderFix::tracked_vs.clear ();
  tbf::RenderFix::tracked_ps.clear ();
  tbf::RenderFix::tracked_vb.clear ();

  //if (config.framerate.minimize_latency)
    //tbf::FrameRateFix::RenderTick ();

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *UpdateSurface_pfn)
  ( _In_       IDirect3DDevice9  *This,
    _In_       IDirect3DSurface9 *pSourceSurface,
    _In_ const RECT              *pSourceRect,
    _In_       IDirect3DSurface9 *pDestinationSurface,
    _In_ const POINT             *pDestinationPoint );

UpdateSurface_pfn D3D9UpdateSurface_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9UpdateSurface_Detour ( IDirect3DDevice9  *This,
                _In_       IDirect3DSurface9 *pSourceSurface,
                _In_ const RECT              *pSourceRect,
                _In_       IDirect3DSurface9 *pDestinationSurface,
                _In_ const POINT             *pDestinationPoint )
{
  HRESULT hr =
    D3D9UpdateSurface_Original ( This,
                                   pSourceSurface,
                                     pSourceRect,
                                       pDestinationSurface,
                                         pDestinationPoint );

//#define DUMP_TEXTURES
  if (SUCCEEDED (hr)) {
#ifdef DUMP_TEXTURES
    IDirect3DTexture9 *pBase = nullptr;

    HRESULT hr2 =
      pDestinationSurface->GetContainer (
            __uuidof (IDirect3DTexture9),
              (void **)&pBase
          );

    if (SUCCEEDED (hr2) && pBase != nullptr) {
      if (D3DXSaveTextureToFile == nullptr) {
        D3DXSaveTextureToFile =
          (D3DXSaveTextureToFile_t)
          GetProcAddress ( tbf::RenderFix::d3dx9_43_dll,
            "D3DXSaveTextureToFileW" );
      }

      if (D3DXSaveTextureToFile != nullptr) {
        wchar_t wszFileName [MAX_PATH] = { L'\0' };
        _swprintf ( wszFileName, L"textures\\UpdateSurface_%x.png",
          pBase );
        D3DXSaveTextureToFile (wszFileName, D3DXIFF_PNG, pBase, NULL);
      }

      pBase->Release ();
    }
#endif
  }

  return hr;
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetScissorRect_Detour (IDirect3DDevice9* This,
                     const RECT*             pRect)
{
  return D3D9SetScissorRect_Original (This, pRect);



  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9SetScissorRect_Original (This, pRect);

  // Let the mod's GUI render without any restrictions.
  if (tbf::RenderFix::draw_state.cegui_active)
    return D3D9SetScissorRect_Original (This, pRect);

  // If we don't care about aspect ratio, then just early-out
  if (! config.render.aspect_correction)
    return D3D9SetScissorRect_Original (This, pRect);

  // Otherwise, fix this because the UI's scissor rectangles are
  //   completely wrong after we start messing with viewport scaling.

  RECT fixed_scissor;
  fixed_scissor.bottom = pRect->bottom;
  fixed_scissor.top    = pRect->top;
  fixed_scissor.left   = pRect->left;
  fixed_scissor.right  = pRect->right;

  float x_scale, y_scale;
  float x_off,   y_off;
  TBF_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    float left_ndc  = 2.0f * ((float)pRect->left  / (float)tbf::RenderFix::width) - 1.0f;
    float right_ndc = 2.0f * ((float)pRect->right / (float)tbf::RenderFix::width) - 1.0f;

    int width = (int)((16.0f / 9.0f) * tbf::RenderFix::height);

    fixed_scissor.left  = (LONG)((left_ndc  * width + width) / 2.0f + x_off);
    fixed_scissor.right = (LONG)((right_ndc * width + width) / 2.0f + x_off);
  } else {
    float top_ndc    = 2.0f * ((float)pRect->top    / (float)tbf::RenderFix::height) - 1.0f;
    float bottom_ndc = 2.0f * ((float)pRect->bottom / (float)tbf::RenderFix::height) - 1.0f;

    int height = (int)((9.0f / 16.0f) * tbf::RenderFix::width);

    fixed_scissor.top    = (LONG)((top_ndc    * height + height) / 2.0f + y_off);
    fixed_scissor.bottom = (LONG)((bottom_ndc * height + height) / 2.0f + y_off);
  }

  return D3D9SetScissorRect_Original (This, &fixed_scissor);
}

typedef HRESULT (STDMETHODCALLTYPE *SetViewport_pfn)(
        IDirect3DDevice9* This,
  CONST D3DVIEWPORT9*     pViewport);

SetViewport_pfn D3D9SetViewport_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetViewport_Detour (IDirect3DDevice9* This,
                  CONST D3DVIEWPORT9*     pViewport)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice)
    return D3D9SetViewport_Original (This, pViewport);

  auto PostProcessMipmaps = [pViewport](void) ->
    void {
      if (config.render.force_post_mips)
      {
        CComPtr <IDirect3DBaseTexture9> pBaseTex0 = nullptr;
        CComPtr <IDirect3DBaseTexture9> pBaseTex1 = nullptr;
        
        if (SUCCEEDED (tbf::RenderFix::pDevice->GetTexture (0, &pBaseTex0))) {
          if (pBaseTex0) pBaseTex0->GenerateMipSubLevels ();

          tbf::RenderFix::pDevice->SetSamplerState (0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
          tbf::RenderFix::pDevice->SetSamplerState (0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
          tbf::RenderFix::pDevice->SetSamplerState (0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
        }

        if (SUCCEEDED (tbf::RenderFix::pDevice->GetTexture (1, &pBaseTex1))) {
          if (pBaseTex1) pBaseTex1->GenerateMipSubLevels ();

          tbf::RenderFix::pDevice->SetSamplerState (1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
          tbf::RenderFix::pDevice->SetSamplerState (1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
          tbf::RenderFix::pDevice->SetSamplerState (1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
        }
      }
    };

  // Bloom (only?)
  //
  //
  if ( config.render.high_res_reflection && (
       ( pViewport->Width == tbf::RenderFix::width/2  && pViewport->Height == tbf::RenderFix::height/2  ) ) )
  {
    D3DVIEWPORT9 rescaled_map = *pViewport;

    rescaled_map.Width  *= 2;
    rescaled_map.Height *= 2;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_map);
  }

  // Bloom (only?)
  //
  //
  if ( config.render.high_res_bloom && (
       ( pViewport->Width == tbf::RenderFix::width/4  && pViewport->Height == tbf::RenderFix::height/4  ) ) )
  {
    D3DVIEWPORT9 rescaled_map = *pViewport;

    rescaled_map.Width  *= 2;
    rescaled_map.Height *= 2;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_map);
  }

  // In-Game Map
  //
  //
  if ( config.render.fix_map_res && (
       ( pViewport->Width == tbf::RenderFix::width/3  && pViewport->Height == tbf::RenderFix::height/3  ) ||
       ( pViewport->Width == tbf::RenderFix::width/5  && pViewport->Height == tbf::RenderFix::height/5  ) ||
       ( pViewport->Width == tbf::RenderFix::width/6  && pViewport->Height == tbf::RenderFix::height/6  ) || 
       ( pViewport->Width == tbf::RenderFix::width/7  && pViewport->Height == tbf::RenderFix::height/7  ) ||
       ( pViewport->Width == tbf::RenderFix::width/8  && pViewport->Height == tbf::RenderFix::height/8  ) || 
       ( pViewport->Width == tbf::RenderFix::width/9  && pViewport->Height == tbf::RenderFix::height/9  ) ||
       ( pViewport->Width == tbf::RenderFix::width/10 && pViewport->Height == tbf::RenderFix::height/10 ) ) )
  {
    D3DVIEWPORT9 rescaled_map = *pViewport;

    rescaled_map.Width = tbf::RenderFix::width; rescaled_map.Height = tbf::RenderFix::height;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_map);
  }

  //
  // Adjust Character Drop Shadows
  //
  if (pViewport->Width == pViewport->Height &&
     ( pViewport->Width == 64 || pViewport->Width == 128 || pViewport->Width == 256))
{
    D3DVIEWPORT9 rescaled_shadow = *pViewport;

    uint32_t shift = TBF_MakeShadowBitShift (pViewport->Width);

    rescaled_shadow.Width  <<= shift;
    rescaled_shadow.Height <<= shift;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_shadow);
  }

  //
  // Environmental Shadows
  //
  if (  pViewport->Width == pViewport->Height && 
      ( pViewport->Width == 512  ||
        pViewport->Width == 1024 ||
        pViewport->Width == 2048 ) )
  {
    //tex_log->Log (L"[Shadow Mgr] (Env. Resolution: (%lu x %lu)", pViewport->Width, pViewport->Height);
    D3DVIEWPORT9 rescaled_shadow = *pViewport;

    rescaled_shadow.Width  <<= config.render.env_shadow_rescale;
    rescaled_shadow.Height <<= config.render.env_shadow_rescale;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_shadow);
  }

  //
  // Adjust Post-Processing (Depth of Field)
  //
  else if ( ( pViewport->Width  == TBF_NextPowerOfTwo (tbf::RenderFix::width / 2) &&
              pViewport->Height == TBF_NextPowerOfTwo (tbf::RenderFix::width / 2) / 2 ) &&
            config.render.postproc_ratio > 0.0f )
  {
    D3DVIEWPORT9 rescaled_post_proc = *pViewport;

    float scale_x, scale_y;
    float x_off,   y_off;
    scale_x = 1.0f; scale_y = 1.0f;
    x_off   = 0.0f;   y_off = 0.0f;
    //TBF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

    rescaled_post_proc.Width  = (DWORD)(tbf::RenderFix::width  * config.render.postproc_ratio * scale_x);
    rescaled_post_proc.Height = (DWORD)(tbf::RenderFix::height * config.render.postproc_ratio * scale_y);
    rescaled_post_proc.X     += (DWORD)x_off;
    rescaled_post_proc.Y     += (DWORD)y_off;

    PostProcessMipmaps ();

    return D3D9SetViewport_Original (This, &rescaled_post_proc);
  }

  HRESULT hr = D3D9SetViewport_Original (This, pViewport);

  return hr;
}

void
TBF_AdjustViewport (IDirect3DDevice9* This, bool UI)
{
  D3DVIEWPORT9 vp9_orig;
  This->GetViewport (&vp9_orig);

  if (! UI) {
    vp9_orig.MinZ = 0.0f;
    vp9_orig.MaxZ = 1.0f;
    vp9_orig.X = 0;
    vp9_orig.Y = 0;
    vp9_orig.Width  = tbf::RenderFix::width;
    vp9_orig.Height = tbf::RenderFix::height;
    D3D9SetViewport_Original (This, &vp9_orig);
    return;
  }

  vp9_orig.X = 0;
  vp9_orig.Y = 0;
  vp9_orig.Width  = tbf::RenderFix::width;
  vp9_orig.Height = tbf::RenderFix::height;

  DWORD width  = vp9_orig.Width;
  DWORD height = static_cast <DWORD> (
                    (9.0f / 16.0f) * static_cast <float> (vp9_orig.Width)
                  );

  // We can't do this, so instead we need to sidebar the stuff
  if (height > vp9_orig.Height) {
    width  = (DWORD)((16.0f / 9.0f) * vp9_orig.Height);
    height = vp9_orig.Height;
  }

  if (height != vp9_orig.Height) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X;    vp9.Y      = vp9_orig.Y + (vp9_orig.Height - height) / 2;
    vp9.Width = width;         vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ; vp9.MaxZ   = vp9_orig.MaxZ;

    D3D9SetViewport_Original (This, &vp9);
  }

  // Sidebar Videos
  if (width != vp9_orig.Width) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X + (vp9_orig.Width - width) / 2; vp9.Y = vp9_orig.Y;
    vp9.Width = width;                                     vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ;                             vp9.MaxZ   = vp9_orig.MaxZ;

    D3D9SetViewport_Original (This, &vp9);
  }
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitive_Detour (IDirect3DDevice9* This,
                                 D3DPRIMITIVETYPE  Type,
                                 INT               BaseVertexIndex,
                                 UINT              MinVertexIndex,
                                 UINT              NumVertices,
                                 UINT              startIndex,
                                 UINT              primCount)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice) {
    return D3D9DrawIndexedPrimitive_Original ( This, Type,
                                                 BaseVertexIndex, MinVertexIndex,
                                                   NumVertices, startIndex,
                                                     primCount );
  }

  ++tbf::RenderFix::draw_state.draws;
  ++draw_count;


  if (TBF_ShouldSkipRenderPass ())
    return S_OK;


  return D3D9DrawIndexedPrimitive_Original ( This, Type,
                                              BaseVertexIndex, MinVertexIndex,
                                                NumVertices, startIndex,
                                                  primCount );
}


typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_pfn)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_pfn D3D9SetVertexShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Detour (IDirect3DDevice9* This,
                                     UINT              StartRegister,
                                     CONST float*      pConstantData,
                                     UINT              Vector4fCount)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice) {
    return D3D9SetVertexShaderConstantF_Original ( This,
                                                     StartRegister,
                                                       pConstantData,
                                                         Vector4fCount );
  }

  //
  // Model Shadows
  //
  else if (StartRegister == 240 && Vector4fCount == 1 && (pConstantData [0] == -pConstantData [1])) {
    uint32_t shift;
    uint32_t dim = 0;

    if (pConstantData [0] == -1.0f / 64.0f) {
      dim = 64UL;
      //dll_log->Log (L" 64x64 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    else if (pConstantData [0] == -1.0f / 128.0f) {
      dim = 128UL;
      //dll_log->Log (L" 128x128 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    else if (pConstantData [0] == -1.0f / 256.0f) {
      dim = 256UL;
      //dll_log->Log (L" 256x256 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    shift = TBF_MakeShadowBitShift (dim);

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    newData [0] = -1.0f / (dim << shift);
    newData [1] =  1.0f / (dim << shift);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log->Log (L"[   D3D9   ] Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    if (dim != 0) {
      return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
    }
  }

  //
  // Post-Processing
  //
  else if (StartRegister     == 240 &&
           Vector4fCount     == 1   &&
           ( pConstantData [0] == -1.0f / (float) TBF_NextPowerOfTwo (tbf::RenderFix::width / 2) &&
             pConstantData [1] ==  1.0f / (float)(TBF_NextPowerOfTwo (tbf::RenderFix::width / 2) / 2) ) &&
            config.render.postproc_ratio > 0.0f )
  {
    if (SUCCEEDED (This->GetRenderTarget (0, &tbf::RenderFix::pPostProcessSurface)))
      tbf::RenderFix::pPostProcessSurface->Release ();

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    float scale_x, scale_y;
    float x_off,   y_off;
    scale_x = 1.0f; scale_y = 1.0f;
    x_off   = 0.0f;   y_off = 0.0f;
    //TBF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

    newData [0] = -1.0f / ((float)tbf::RenderFix::width  * config.render.postproc_ratio * scale_x);
    newData [1] =  1.0f / ((float)tbf::RenderFix::height * config.render.postproc_ratio * scale_y);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log->Log (L"[   D3D9   ] Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
  }

  //
  // Env Shadow
  //
  if (StartRegister == 240 && Vector4fCount == 1 && (pConstantData [0] == -pConstantData [1])) {
    uint32_t shift;
    uint32_t dim = 0;

    if (pConstantData [0] == -1.0f / 512.0f) {
      dim = 512UL;
      //dll_log->Log (L" 512x512 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    else if (pConstantData [0] == -1.0f / 1024.0f) {
      dim = 1024UL;
      //dll_log->Log (L" 1024x1024 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    else if (pConstantData [0] == -1.0f / 2048.0f) {
      dim = 2048UL;
      //dll_log->Log (L" 2048x2048 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksum, ps_checksum);
    }

    shift = config.render.env_shadow_rescale;

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    newData [0] = -1.0f / (dim << shift);
    newData [1] =  1.0f / (dim << shift);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log->Log (L"[   D3D9   ] Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    if (dim != 0) {
      return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
    }
  }


  //
  // Model Shadows and Post-Processing
  //
  if (StartRegister == 0 && (Vector4fCount == 2 || Vector4fCount == 3)) {
    IDirect3DSurface9* pSurf = nullptr;

    if (This == tbf::RenderFix::pDevice && SUCCEEDED (This->GetRenderTarget (0, &pSurf)) && pSurf != nullptr)
    {
      D3DSURFACE_DESC desc;
      pSurf->GetDesc (&desc);
      pSurf->Release ();

      //
      // Post-Processing
      //
      if (config.render.postproc_ratio > 0.0f) {
        if (desc.Width  == tbf::RenderFix::width  &&
            desc.Height == tbf::RenderFix::height) {
          if (pSurf == tbf::RenderFix::pPostProcessSurface)
          {
            float newData [12];

            float scale_x, scale_y;
            float x_off, y_off;

            scale_x = 1.0f; scale_y = 1.0f;
            x_off   = 0.0f;   y_off = 0.0f;

            //TBF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

            float rescale_x = (float) TBF_NextPowerOfTwo (tbf::RenderFix::width / 2)
                               / ((float)tbf::RenderFix::width  * config.render.postproc_ratio * scale_x);
            float rescale_y = (float)(TBF_NextPowerOfTwo (tbf::RenderFix::width / 2) / 2)
                               / ((float)tbf::RenderFix::height * config.render.postproc_ratio * scale_y);

            for (int i = 0; i < 8; i += 2) {
              newData [i] = pConstantData [i] * rescale_x;
            }

            for (int i = 1; i < 8; i += 2) {
              newData [i] = pConstantData [i] * rescale_y;
            }

            //fix_aspect = true;

            return D3D9SetVertexShaderConstantF_Original (This, 0, newData, Vector4fCount);
          }
        }
      }

      //
      // Model Shadows
      //
      if (desc.Width == desc.Height && desc.Width > 64 && desc.Height < 4096) {
        float newData [12];

        uint32_t shift = TBF_MakeShadowBitShift (desc.Width);

        for (UINT i = 0; i < Vector4fCount * 4; i++) {
          newData [i] = pConstantData [i] / (float)(1 << shift);
        }

        return D3D9SetVertexShaderConstantF_Original (This, 0, newData, Vector4fCount);
      }
    }
  }


  if (config.render.smaa.override_game && ( vs_checksum == config.render.smaa.smaa_vs0_crc32 ||
                                            vs_checksum == config.render.smaa.smaa_vs1_crc32 ||
                                            vs_checksum == config.render.smaa.smaa_vs2_crc32 ))
  {
    //dll_log->Log (L"[SMAA Log] Crc32: %x, Start Register: %lu, Vec4Count: %lu ", vs_checksum, StartRegister, Vector4fCount);

    if (StartRegister == 16 && Vector4fCount == 3)
    {
      float override_params [12];
      memcpy (override_params, pConstantData, sizeof (float) * 12);

      smaa_constants_s* smaa_constants =
        (smaa_constants_s *)override_params;

      smaa_constants->general.threshold             =        config.render.smaa.threshold;
      smaa_constants->general.max_search_steps      = (float)config.render.smaa.max_search_steps;
      smaa_constants->general.max_search_steps_diag = (float)config.render.smaa.max_search_steps_diag;
      smaa_constants->general.corner_rounding       =        config.render.smaa.corner_rounding;

      smaa_constants->predication.threshold         =        config.render.smaa.predication_threshold;
      smaa_constants->predication.scale             =        config.render.smaa.predication_scale;
      smaa_constants->predication.strength          =        config.render.smaa.predication_strength;

      smaa_constants->reprojection.weight           =        config.render.smaa.reprojection_weight;

      return D3D9SetVertexShaderConstantF_Original (This, StartRegister, override_params, Vector4fCount);
    }
  }


  return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



typedef HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF_pfn)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_pfn  D3D9SetPixelShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShaderConstantF_Detour (IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount)
{
  if (config.render.smaa.override_game && ( ps_checksum == config.render.smaa.smaa_ps0_crc32 ||
                                            ps_checksum == config.render.smaa.smaa_ps1_crc32 ))
  {
    //dll_log->Log (L"[SMAA Log] Crc32: %x, Start Register: %lu, Vec4Count: %lu ", ps_checksum, StartRegister, Vector4fCount);

    if (StartRegister == 0 && Vector4fCount == 3)
    {
      float override_params [12];
      memcpy (override_params, pConstantData, sizeof (float) * 12);

      smaa_constants_s* smaa_constants =
        (smaa_constants_s *)override_params;

      smaa_constants->general.threshold             =        config.render.smaa.threshold;
      smaa_constants->general.max_search_steps      = (float)config.render.smaa.max_search_steps;
      smaa_constants->general.max_search_steps_diag = (float)config.render.smaa.max_search_steps_diag;
      smaa_constants->general.corner_rounding       =        config.render.smaa.corner_rounding;

      smaa_constants->predication.threshold         =        config.render.smaa.predication_threshold;
      smaa_constants->predication.scale             =        config.render.smaa.predication_scale;
      smaa_constants->predication.strength          =        config.render.smaa.predication_strength;

      smaa_constants->reprojection.weight           =        config.render.smaa.reprojection_weight;

      return D3D9SetPixelShaderConstantF_Original (This, StartRegister, override_params, Vector4fCount);
    }
  }

  return D3D9SetPixelShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}




COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitive_Detour (IDirect3DDevice9* This,
                          D3DPRIMITIVETYPE  PrimitiveType,
                          UINT              StartVertex,
                          UINT              PrimitiveCount)
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice) {
    dll_log->Log (L"[Render Fix] >> WARNING: DrawPrimitive came from unknown IDirect3DDevice9! << ");

    return D3D9DrawPrimitive_Original ( This, PrimitiveType,
                                                 StartVertex, PrimitiveCount );
  }

  tbf::RenderFix::draw_state.draws++;


  if (TBF_ShouldSkipRenderPass ())
    return S_OK;


#if 0
  if (tsf::RenderFix::tracer.log) {
    dll_log->Log ( L"[FrameTrace] DrawPrimitive - %X, StartVertex: %lu, PrimitiveCount: %lu",
                      PrimitiveType, StartVertex, PrimitiveCount );
  }
#endif

  return D3D9DrawPrimitive_Original ( This, PrimitiveType,
                                        StartVertex, PrimitiveCount );
}

const wchar_t*
SK_D3D9_PrimitiveTypeToStr (D3DPRIMITIVETYPE pt)
{
  switch (pt)
  {
    case D3DPT_POINTLIST             : return L"D3DPT_POINTLIST";
    case D3DPT_LINELIST              : return L"D3DPT_LINELIST";
    case D3DPT_LINESTRIP             : return L"D3DPT_LINESTRIP";
    case D3DPT_TRIANGLELIST          : return L"D3DPT_TRIANGLELIST";
    case D3DPT_TRIANGLESTRIP         : return L"D3DPT_TRIANGLESTRIP";
    case D3DPT_TRIANGLEFAN           : return L"D3DPT_TRIANGLEFAN";
  }

  return L"Invalid Primitive";
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitiveUP_Detour ( IDirect3DDevice9* This,
                             D3DPRIMITIVETYPE  PrimitiveType,
                             UINT              PrimitiveCount,
                             const void       *pVertexStreamZeroData,
                             UINT              VertexStreamZeroStride )
{
#if 0
  if (tsf::RenderFix::tracer.log && This == tsf::RenderFix::pDevice) {
    dll_log->Log ( L"[FrameTrace] DrawPrimitiveUP   (Type: %s) - PrimitiveCount: %lu"/*
                   L"                         [FrameTrace]                 -"
                   L"    BaseIdx:     %5li, MinVtxIdx:  %5lu,\n"
                   L"                         [FrameTrace]                 -"
                   L"    NumVertices: %5lu, startIndex: %5lu,\n"
                   L"                         [FrameTrace]                 -"
                   L"    primCount:   %5lu"*/,
                     SK_D3D9_PrimitiveTypeToStr (PrimitiveType),
                       PrimitiveCount/*,
                       BaseVertexIndex, MinVertexIndex,
                         NumVertices, startIndex, primCount*/ );
  }
#endif

  tbf::RenderFix::draw_state.draws++;


  if (TBF_ShouldSkipRenderPass ())
    return S_OK;


  return
    D3D9DrawPrimitiveUP_Original ( This,
                                     PrimitiveType,
                                       PrimitiveCount,
                                         pVertexStreamZeroData,
                                           VertexStreamZeroStride );
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitiveUP_Detour ( IDirect3DDevice9* This,
                                    D3DPRIMITIVETYPE  PrimitiveType,
                                    UINT              MinVertexIndex,
                                    UINT              NumVertices,
                                    UINT              PrimitiveCount,
                                    const void       *pIndexData,
                                    D3DFORMAT         IndexDataFormat,
                                    const void       *pVertexStreamZeroData,
                                    UINT              VertexStreamZeroStride )
{
#if 0
  if (tsf::RenderFix::tracer.log && This == tsf::RenderFix::pDevice) {
    dll_log->Log ( L"[FrameTrace] DrawIndexedPrimitiveUP   (Type: %s) - NumVertices: %lu, PrimitiveCount: %lu"/*
                   L"                         [FrameTrace]                 -"
                   L"    BaseIdx:     %5li, MinVtxIdx:  %5lu,\n"
                   L"                         [FrameTrace]                 -"
                   L"    NumVertices: %5lu, startIndex: %5lu,\n"
                   L"                         [FrameTrace]                 -"
                   L"    primCount:   %5lu"*/,
                     SK_D3D9_PrimitiveTypeToStr (PrimitiveType),
                       NumVertices, PrimitiveCount/*,
                       BaseVertexIndex, MinVertexIndex,
                         NumVertices, startIndex, primCount*/ );
  }
#endif

  tbf::RenderFix::draw_state.draws++;


  if (TBF_ShouldSkipRenderPass ())
    return S_OK;


  return
    D3D9DrawIndexedPrimitiveUP_Original (
      This,
        PrimitiveType,
          MinVertexIndex,
            NumVertices,
              PrimitiveCount,
                pIndexData,
                  IndexDataFormat,
                    pVertexStreamZeroData,
                      VertexStreamZeroStride );
}


void
tbf::RenderFix::Reset ( IDirect3DDevice9      *This,
                        D3DPRESENT_PARAMETERS *pPresentationParameters )
{
  static volatile
    ULONG reset_count = 0UL;

  ULONG count = InterlockedIncrement (&reset_count);

  if (count == 1UL) {
    tex_mgr.Hook         ();
    TBF_ApplyQueuedHooks ();
  }

  else {
    if (pending_loads ())
      TBFix_LoadQueuedTextures ();

    tex_mgr.reset              ();

    need_reset.textures = false;
  }

  tbf::RenderFix::tex_mgr.resetUsedTextures ();

  need_reset.graphics = false;

  vs_checksums.clear ();
  ps_checksums.clear ();

  g_pPS   = nullptr;
  g_pVS   = nullptr;

  pDevice = This;

  width   = pPresentationParameters->BackBufferWidth;
  height  = pPresentationParameters->BackBufferHeight;

  fullscreen = (! pPresentationParameters->Windowed);
}

typedef HRESULT (__stdcall *Reset_pfn)(
  IDirect3DDevice9     *This,
 D3DPRESENT_PARAMETERS *pPresentationParameters
);

Reset_pfn D3D9Reset_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
__stdcall
D3D9Reset_Detour ( IDirect3DDevice9      *This,
                   D3DPRESENT_PARAMETERS *pPresentationParameters )
{
  tbf::RenderFix::Reset (This, pPresentationParameters);

  HRESULT hr =
    D3D9Reset_Original (This, pPresentationParameters);

  trigger_reset = reset_stage_s::Clear;

  tbf::FrameRateFix::need_reset = true;

  return hr;
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9TestCooperativeLevel_Detour ( IDirect3DDevice9 *This )
{
  if (trigger_reset == reset_stage_s::Initiate)
  {
    trigger_reset = reset_stage_s::Respond;
    return D3DERR_DEVICELOST;
  }

  else if (trigger_reset == reset_stage_s::Respond)
  {
    return D3DERR_DEVICENOTRESET;
  }

  return D3D9TestCooperativeLevel_Original (This);
}

#if 0
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateVertexBuffer_Detour (
  _In_  IDirect3DDevice9        *This,
  _In_  UINT                     Length,
  _In_  DWORD                    Usage,
  _In_  DWORD                    FVF,
  _In_  D3DPOOL                  Pool,
  _Out_ IDirect3DVertexBuffer9 **ppVertexBuffer,
  _In_  HANDLE                  *pSharedHandle )
{

}
#endif

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetStreamSource_Detour
(
  IDirect3DDevice9       *This,
  UINT                    StreamNumber,
  IDirect3DVertexBuffer9 *pStreamData,
  UINT                    OffsetInBytes,
  UINT                    Stride )
{
  // Ignore anything that's not the primary render device.
  if (This != tbf::RenderFix::pDevice) {
    return
      D3D9SetStreamSource_Original ( This,
                                       StreamNumber,
                                         pStreamData,
                                           OffsetInBytes,
                                             Stride );
  }

  HRESULT hr =
      D3D9SetStreamSource_Original ( This,
                                       StreamNumber,
                                         pStreamData,
                                           OffsetInBytes,
                                             Stride );

  if (SUCCEEDED (hr))
  {
    tbf::RenderFix::last_frame.vertex_buffers.emplace (pStreamData);

    if (StreamNumber == 0)
      vb_stream0 = pStreamData;
  }

  return hr;
}

__declspec (noinline)
D3DPRESENT_PARAMETERS*
__stdcall
SK_SetPresentParamsD3D9_Detour (IDirect3DDevice9*      device,
                                 D3DPRESENT_PARAMETERS* pparams)
{
  ////static D3DPRESENT_PARAMETERS present_params;

  //
  // TODO: Figure out what the hell is doing this when RTSS is allowed to use
  //         custom D3D libs. 1x1@0Hz is obviously NOT for rendering!
  //
  if ( pparams->BackBufferWidth            == 1 &&
       pparams->BackBufferHeight           == 1 &&
       pparams->FullScreen_RefreshRateInHz == 0 )
  {
    dll_log->Log (L"[Render Fix] * Fake D3D9Ex Device Detected... Ignoring!");
    return SK_SetPresentParamsD3D9_Original (device, pparams);
  }

  if (device != nullptr) 
  {
    dll_log->Log ( L"[Render Fix] %% Caught D3D9 Swapchain :: Fullscreen=%s "
                   L" (%lux%lu @%#3lu Hz) "
                   L" [Device Window: 0x%p, Pointer: %ph]",
                     pparams->Windowed ? L"False" :

                      L"True",
                       pparams->BackBufferWidth,
                         pparams->BackBufferHeight,
                           pparams->FullScreen_RefreshRateInHz,
                             pparams->hDeviceWindow,
                               device );
  }

  if (pparams->hDeviceWindow != nullptr)
    tbf::RenderFix::hWndDevice = pparams->hDeviceWindow;

  tbf::RenderFix::fullscreen = (! pparams->Windowed);

  tbf::RenderFix::pDevice = device;
                          
  tbf::RenderFix::width   = pparams->BackBufferWidth;
  tbf::RenderFix::height  = pparams->BackBufferHeight;

  // Reset will fail without this
  if (pparams != nullptr && pparams->Windowed)
  {
    BringWindowToTop (tbf::RenderFix::hWndDevice);
    SetActiveWindow  (tbf::RenderFix::hWndDevice);
    pparams->FullScreen_RefreshRateInHz = 0;
  }

  return SK_SetPresentParamsD3D9_Original (device, pparams);
}


void
tbf::RenderFix::Init (void)
{
  last_frame.vertex_shaders.reserve (256);
  last_frame.pixel_shaders.reserve  (256);
  ps_disassembly.reserve            (512);
  vs_disassembly.reserve            (512);
  vs_checksums.reserve              (8192);
  ps_checksums.reserve              (8192);

  trigger_reset = reset_stage_s::Clear;

  d3dx9_43_dll = LoadLibrary (L"D3DX9_43.DLL");

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetSamplerState_Override",
                      D3D9SetSamplerState_Detour,
            (LPVOID*)&D3D9SetSamplerState_Original,
                     &SetSamplerState );

  // Needed for shadow re-scaling
  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetViewport_Override",
                       D3D9SetViewport_Detour,
             (LPVOID*)&D3D9SetViewport_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetPixelShaderConstantF_Override",
                       D3D9SetPixelShaderConstantF_Detour,
             (LPVOID*)&D3D9SetPixelShaderConstantF_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetVertexShaderConstantF_Override",
                       D3D9SetVertexShaderConstantF_Detour,
             (LPVOID*)&D3D9SetVertexShaderConstantF_Original );


  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetVertexShader_Override",
                       D3D9SetVertexShader_Detour,
             (LPVOID*)&D3D9SetVertexShader_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetPixelShader_Override",
                       D3D9SetPixelShader_Detour,
             (LPVOID*)&D3D9SetPixelShader_Original );

  // Needed for UI re-scaling
//  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9SetScissorRect_Override",
//                       D3D9SetScissorRect_Detour,
//             (LPVOID*)&D3D9SetScissorRect_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "D3D9EndScene_Override",
                       D3D9EndScene_Detour,
             (LPVOID*)&D3D9EndScene_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9Reset_Override",
                       D3D9Reset_Detour,
            (LPVOID *)&D3D9Reset_Original );

  user32_dll   = LoadLibrary (L"User32.dll");


  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "SK_BeginBufferSwap",
                       D3D9EndFrame_Pre,
             (LPVOID*)&SK_BeginBufferSwap );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (), "SK_EndBufferSwap",
                       D3D9EndFrame_Post,
             (LPVOID*)&SK_EndBufferSwap );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "SK_SetPresentParamsD3D9",
                        SK_SetPresentParamsD3D9_Detour,
             (LPVOID *)&SK_SetPresentParamsD3D9_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9TestCooperativeLevel_Override",
                        D3D9TestCooperativeLevel_Detour,
             (LPVOID *)&D3D9TestCooperativeLevel_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9SetStreamSource_Override",
                        D3D9SetStreamSource_Detour,
             (LPVOID *)&D3D9SetStreamSource_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9DrawPrimitive_Override",
                        D3D9DrawPrimitive_Detour,
              (LPVOID*)&D3D9DrawPrimitive_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9DrawIndexedPrimitive_Override",
                        D3D9DrawIndexedPrimitive_Detour,
              (LPVOID*)&D3D9DrawIndexedPrimitive_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9DrawPrimitiveUP_Override",
                        D3D9DrawPrimitiveUP_Detour,
              (LPVOID*)&D3D9DrawPrimitiveUP_Original );

  TBF_CreateDLLHook2 ( config.system.injector.c_str (),
                       "D3D9DrawIndexedPrimitiveUP_Override",
                        D3D9DrawIndexedPrimitiveUP_Detour,
              (LPVOID*)&D3D9DrawIndexedPrimitiveUP_Original );

  D3DXDisassembleShader =
    (D3DXDisassembleShader_pfn)
      GetProcAddress ( tbf::RenderFix::d3dx9_43_dll,
                         "D3DXDisassembleShader" );

  CommandProcessor* comm_proc = CommandProcessor::getInstance ();

  tex_mgr.Init ();
}

void
tbf::RenderFix::Shutdown (void)
{
  tex_mgr.Shutdown ();
}

tbf::RenderFix::CommandProcessor::CommandProcessor (void)
{
  SK_ICommandProcessor& command =
    *SK_GetCommandProcessor ();

  //fovy_         = TBF_CreateVar (SK_IVariable::Float, &config.render.fovy,         this);
  //aspect_ratio_ = TBF_CreateVar (SK_IVariable::Float, &config.render.aspect_ratio, this);

  SK_IVariable* aspect_correct_vids = TBF_CreateVar (SK_IVariable::Boolean, &config.render.blackbar_videos);
  SK_IVariable* aspect_correction   = TBF_CreateVar (SK_IVariable::Boolean, &config.render.aspect_correction);

  SK_IVariable* postproc_ratio      = TBF_CreateVar (SK_IVariable::Float,   &config.render.postproc_ratio);
  SK_IVariable* clear_blackbars     = TBF_CreateVar (SK_IVariable::Boolean, &config.render.clear_blackbars);

  SK_IVariable* remaster_textures   = TBF_CreateVar (SK_IVariable::Boolean, &config.textures.remaster);
  SK_IVariable* lod_bias            = TBF_CreateVar (SK_IVariable::Float,   &config.textures.lod_bias);
  SK_IVariable* uncompressed        = TBF_CreateVar (SK_IVariable::Boolean, &config.textures.uncompressed);

  SK_IVariable* rescale_shadows     = TBF_CreateVar (SK_IVariable::Int,     &config.render.shadow_rescale);
  SK_IVariable* rescale_env_shadows = TBF_CreateVar (SK_IVariable::Int,     &config.render.env_shadow_rescale);

  //command.AddVariable ("AspectRatio",         aspect_ratio_);
  //command.AddVariable ("FOVY",                fovy_);

  command.AddVariable ("Textures.Remaster",     remaster_textures);
  command.AddVariable ("Textures.LODBias",      lod_bias);
  command.AddVariable ("Textures.Uncompressed", uncompressed);

  command.AddVariable ("Shadows.Rescale",       rescale_shadows);
  command.AddVariable ("Shadows.RescaleEnv",    rescale_env_shadows);

  command.AddVariable ("AspectCorrectVideos",   aspect_correct_vids);
  command.AddVariable ("AspectCorrection",      aspect_correction);
  command.AddVariable ("PostProcessRatio",      postproc_ratio);
  command.AddVariable ("ClearBlackbars",        clear_blackbars);

#if 0
   uint8_t signature [] = { 0x39, 0x8E, 0xE3, 0x3F,
                            0xDB, 0x0F, 0x49, 0x3F };

  if (*(float *)(uintptr_t)config.render.aspect_addr != 16.0f / 9.0f) {
    void* addr = TBF_Scan (signature, sizeof (float) * 2, nullptr);
    if (addr != nullptr) {
      dll_log->Log (L"[Asp. Ratio] Scanned Aspect Ratio Address: %06Xh", addr);
      config.render.aspect_addr = (DWORD)addr;
      dll_log->Log (L"[Asp. Ratio] Scanned FOVY Address: %06Xh", (float *)addr + 1);
      config.render.fovy_addr = (DWORD)((float *)addr + 1);
    }
    else {
      dll_log->Log (L"[Asp. Ratio]  >> ERROR: Unable to find Aspect Ratio Address!");
    }
  }
#endif
}

bool
tbf::RenderFix::CommandProcessor::OnVarChange (SK_IVariable* var, void* val)
{
#if 0
  DWORD dwOld;

  if (var == aspect_ratio_) {
    VirtualProtect ((LPVOID)config.render.aspect_addr, 4, PAGE_READWRITE, &dwOld);
    float original = *((float *)config.render.aspect_addr);

    if (((original < 1.777778f + 0.01f && original > 1.777778f - 0.01f) ||
         (original == config.render.aspect_ratio))
            && val != nullptr) {
      config.render.aspect_ratio = *(float *)val;

      if (fabs (original - config.render.aspect_ratio) > 0.01f) {
        dll_log->Log ( L"[Asp. Ratio]  * Changing Aspect Ratio from %f to %f",
                          original,
                           config.render.aspect_ratio );
        *((float *)config.render.aspect_addr) = config.render.aspect_ratio;
       }
    }
    else {
      if (val != nullptr)
        dll_log->Log ( L"[Asp. Ratio]  * Unable to change Aspect Ratio, invalid memory address... (%f)",
                         *((float *)config.render.aspect_addr) );
    }
  }

  if (var == fovy_) {
    VirtualProtect ((LPVOID)config.render.fovy_addr, 4, PAGE_READWRITE, &dwOld);
    float original = *((float *)config.render.fovy_addr);

    if (((original < 0.785398f + 0.01f && original > 0.785398f - 0.01f) ||
         (original == config.render.fovy))
            && val != nullptr) {
      config.render.fovy = *(float *)val;
      dll_log->Log ( L"[Asp. Ratio]  * Changing FOVY from %f to %f",
                        original,
                          config.render.fovy );
      *((float *)config.render.fovy_addr) = config.render.fovy;
    }
    else {
      if (val != nullptr)
        dll_log->Log ( L"[Asp. Ratio]  * Unable to change FOVY, invalid memory address... (%f)",
                         *((float *)config.render.fovy_addr) );
    }
  }
#endif

  return true;
}


void
tbf::RenderFix::TriggerReset (void)
{
  trigger_reset = reset_stage_s::Initiate;
}

extern HMODULE hInjectorDLL;

bool
tbf::RenderFix::InstallSGSSAA (void)
{
  ((void (__stdcall *)(const wchar_t * ))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAppFriendlyName"))     ( L"Tales of Berseria" );
  ((void (__stdcall *)(const wchar_t * ))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAppName"))             ( L"Tales of Berseria.exe" );

  wchar_t wszBits [16] = { L'\0' };

  wcsncpy (wszBits, config.render.nv.compat_bits.c_str (), 16);
  
  if (config.render.nv.sgssaa_mode == 1)
  {
    const wchar_t* props [] = { L"CompatibilityBits", wszBits,
                                L"Method",            L"2xMSAA",
                                L"ReplayMode",        L"2xSGSSAA",
                                L"AntiAliasFix",      L"Off",
                                L"AutoBiasAdjust",    L"Off",
                                L"Override",          L"On",
                                nullptr,              nullptr };
    return ((BOOL (__stdcall *)(const wchar_t **))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAntiAliasingOverride"))( props );
  }
  
  else if (config.render.nv.sgssaa_mode == 2)
  {
    const wchar_t* props [] = { L"CompatibilityBits", wszBits,
                                L"Method",            L"4xMSAA",
                                L"ReplayMode",        L"4xSGSSAA",
                                L"AntiAliasFix",      L"Off",
                                L"AutoBiasAdjust",    L"Off",
                                L"Override",          L"On",
                                nullptr,              nullptr };
    return ((BOOL (__stdcall *)(const wchar_t **))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAntiAliasingOverride"))( props );
  }
  
  else if (config.render.nv.sgssaa_mode == 3)
  {
    const wchar_t* props [] = { L"CompatibilityBits", wszBits,
                                L"Method",            L"8xMSAA",
                                L"ReplayMode",        L"8xSGSSAA",
                                L"AntiAliasFix",      L"Off",
                                L"AutoBiasAdjust",    L"Off",
                                L"Override",          L"On",
                                nullptr,              nullptr };
    return ((BOOL (__stdcall *)(const wchar_t **))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAntiAliasingOverride"))( props );
  }
  
  else
  {
    const wchar_t* props [] = { L"Method",            L"0x00000000",
                                L"ReplayMode",        L"0x00000000",
                                L"AutoBiasAdjust",    L"On",
                                L"Override",          L"No",
                                nullptr,              nullptr };
    return ((BOOL (__stdcall *)(const wchar_t **))GetProcAddress (hInjectorDLL, "SK_NvAPI_SetAntiAliasingOverride"))( props );
  }

  return FALSE;
}


tbf::RenderFix::CommandProcessor*
                   tbf::RenderFix::CommandProcessor::pCommProc
                                                       = nullptr;

HWND               tbf::RenderFix::hWndDevice          = NULL;
IDirect3DDevice9*  tbf::RenderFix::pDevice             = nullptr;

bool               tbf::RenderFix::fullscreen          = false;

         uint32_t  tbf::RenderFix::width               = 0UL;
         uint32_t  tbf::RenderFix::height              = 0UL;
volatile ULONG     tbf::RenderFix::dwRenderThreadID    = 0UL;

IDirect3DSurface9* tbf::RenderFix::pPostProcessSurface = nullptr;
bool               tbf::RenderFix::bink                = false;

HMODULE            tbf::RenderFix::user32_dll          = 0;

tbf::RenderFix::reset_state_s
                   tbf::RenderFix::need_reset;

tbf::RenderFix::render_target_tracking_s
                   tbf::RenderFix::tracked_rt;

tbf::RenderFix::shader_tracking_s
                   tbf::RenderFix::tracked_vs;

tbf::RenderFix::shader_tracking_s
                   tbf::RenderFix::tracked_ps;

tbf::RenderFix::vertex_buffer_tracking_s
                   tbf::RenderFix::tracked_vb;

void
EnumConstant ( tbf::RenderFix::shader_tracking_s* pShader,
               ID3DXConstantTable*                pConstantTable,
               D3DXHANDLE                         hConstant,
               tbf::RenderFix::shader_tracking_s::
                               shader_constant_s& constant,
               std::vector <
                 tbf::RenderFix::shader_tracking_s::
                             shader_constant_s >& list )
{
  UINT one = 1;
  
  D3DXCONSTANT_DESC constant_desc;
  if (SUCCEEDED (pConstantTable->GetConstantDesc (hConstant, &constant_desc, &one)))
  {
    strncpy (constant.Name, constant_desc.Name, 128);
    constant.Class         = constant_desc.Class;
    constant.Type          = constant_desc.Type;
    constant.RegisterSet   = constant_desc.RegisterSet;
    constant.RegisterIndex = constant_desc.RegisterIndex;
    constant.RegisterCount = constant_desc.RegisterCount;
    constant.Rows          = constant_desc.Rows;
    constant.Columns       = constant_desc.Columns;
    //constant.Elements      = constant_desc.Elements;

    //if (constant_desc.DefaultValue != nullptr)
      //memcpy (constant.Data, constant_desc.DefaultValue, std::min ((size_t)constant_desc.Bytes, sizeof (float) * 4UL));

    for ( UINT j = 0; j < constant_desc.StructMembers; j++ )
    {
      D3DXHANDLE hConstantStruct =
        pConstantTable->GetConstant (hConstant, j);
  
      tbf::RenderFix::shader_tracking_s::shader_constant_s struct_constant = { };
  
      EnumConstant (pShader, pConstantTable, hConstantStruct, struct_constant, constant.struct_members );
    }
  
    list.push_back (constant);
  }
};



void
tbf::RenderFix::shader_tracking_s::use (IUnknown *pShader)
{
  if (shader_obj != pShader)
  {
    constants.clear ();

    shader_obj = pShader;

    static D3DXGetShaderConstantTable_pfn D3DXGetShaderConstantTable =
      (D3DXGetShaderConstantTable_pfn)
        GetProcAddress (d3dx9_43_dll, "D3DXGetShaderConstantTable");

    UINT len;
    if (SUCCEEDED (((IDirect3DVertexShader9 *)pShader)->GetFunction (nullptr, &len)))
    {
      void* pbFunc = malloc (len);
      
      if (pbFunc != nullptr)
      {
        if ( SUCCEEDED ( ((IDirect3DVertexShader9 *)pShader)->GetFunction ( pbFunc,
                                                                              &len )
                       )
           )
        {
          CComPtr <ID3DXConstantTable> pConstantTable = nullptr;

          if (SUCCEEDED (D3DXGetShaderConstantTable ((DWORD *)pbFunc, &pConstantTable)))
          {
            D3DXCONSTANTTABLE_DESC ct_desc;

            if (SUCCEEDED (pConstantTable->GetDesc (&ct_desc)))
            {
              UINT constant_count = ct_desc.Constants;

              for (UINT i = 0; i < constant_count; i++)
              {
                D3DXHANDLE hConstant =
                  pConstantTable->GetConstant (nullptr, i);

                shader_constant_s constant = { };

                EnumConstant (this, pConstantTable, hConstant, constant, constants);
              }
            }
          }
        }
      
        free (pbFunc);
      }
    }
  }
}
