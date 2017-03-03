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

#ifndef __TBF__RENDER_H__
#define __TBF__RENDER_H__
#define NOMINMAX

#include "command.h"

#include <d3d9.h>
#include <d3d9types.h>
#include <Windows.h>
#include <unordered_set>

//---------------------------------------------------------------------------
// D3DXTX_VERSION:
// --------------
// Version token used to create a procedural texture filler in effects
// Used by D3DXFill[]TX functions
//---------------------------------------------------------------------------
#define D3DXTX_VERSION(_Major,_Minor) (('T' << 24) | ('X' << 16) | ((_Major) << 8) | (_Minor))



//----------------------------------------------------------------------------
// D3DXSHADER flags:
// -----------------
// D3DXSHADER_DEBUG
//   Insert debug file/line/type/symbol information.
//
// D3DXSHADER_SKIPVALIDATION
//   Do not validate the generated code against known capabilities and
//   constraints.  This option is only recommended when compiling shaders
//   you KNOW will work.  (ie. have compiled before without this option.)
//   Shaders are always validated by D3D before they are set to the device.
//
// D3DXSHADER_SKIPOPTIMIZATION 
//   Instructs the compiler to skip optimization steps during code generation.
//   Unless you are trying to isolate a problem in your code using this option 
//   is not recommended.
//
// D3DXSHADER_PACKMATRIX_ROWMAJOR
//   Unless explicitly specified, matrices will be packed in row-major order
//   on input and output from the shader.
//
// D3DXSHADER_PACKMATRIX_COLUMNMAJOR
//   Unless explicitly specified, matrices will be packed in column-major 
//   order on input and output from the shader.  This is generally more 
//   efficient, since it allows vector-matrix multiplication to be performed
//   using a series of dot-products.
//
// D3DXSHADER_PARTIALPRECISION
//   Force all computations in resulting shader to occur at partial precision.
//   This may result in faster evaluation of shaders on some hardware.
//
// D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT
//   Force compiler to compile against the next highest available software
//   target for vertex shaders.  This flag also turns optimizations off, 
//   and debugging on.  
//
// D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT
//   Force compiler to compile against the next highest available software
//   target for pixel shaders.  This flag also turns optimizations off, 
//   and debugging on.
//
// D3DXSHADER_NO_PRESHADER
//   Disables Preshaders. Using this flag will cause the compiler to not 
//   pull out static expression for evaluation on the host cpu
//
// D3DXSHADER_AVOID_FLOW_CONTROL
//   Hint compiler to avoid flow-control constructs where possible.
//
// D3DXSHADER_PREFER_FLOW_CONTROL
//   Hint compiler to prefer flow-control constructs where possible.
//
//----------------------------------------------------------------------------

#define D3DXSHADER_DEBUG                          (1 << 0)
#define D3DXSHADER_SKIPVALIDATION                 (1 << 1)
#define D3DXSHADER_SKIPOPTIMIZATION               (1 << 2)
#define D3DXSHADER_PACKMATRIX_ROWMAJOR            (1 << 3)
#define D3DXSHADER_PACKMATRIX_COLUMNMAJOR         (1 << 4)
#define D3DXSHADER_PARTIALPRECISION               (1 << 5)
#define D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT        (1 << 6)
#define D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT        (1 << 7)
#define D3DXSHADER_NO_PRESHADER                   (1 << 8)
#define D3DXSHADER_AVOID_FLOW_CONTROL             (1 << 9)
#define D3DXSHADER_PREFER_FLOW_CONTROL            (1 << 10)
#define D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY (1 << 12)
#define D3DXSHADER_IEEE_STRICTNESS                (1 << 13)
#define D3DXSHADER_USE_LEGACY_D3DX9_31_DLL        (1 << 16)


// optimization level flags
#define D3DXSHADER_OPTIMIZATION_LEVEL0            (1 << 14)
#define D3DXSHADER_OPTIMIZATION_LEVEL1            0
#define D3DXSHADER_OPTIMIZATION_LEVEL2            ((1 << 14) | (1 << 15))
#define D3DXSHADER_OPTIMIZATION_LEVEL3            (1 << 15)



//----------------------------------------------------------------------------
// D3DXCONSTTABLE flags:
// -------------------

#define D3DXCONSTTABLE_LARGEADDRESSAWARE          (1 << 17)



//----------------------------------------------------------------------------
// D3DXHANDLE:
// -----------
// Handle values used to efficiently reference shader and effect parameters.
// Strings can be used as handles.  However, handles are not always strings.
//----------------------------------------------------------------------------

#ifndef D3DXFX_LARGEADDRESS_HANDLE
typedef LPCSTR D3DXHANDLE;
#else
typedef UINT_PTR D3DXHANDLE;
#endif
typedef D3DXHANDLE *LPD3DXHANDLE;


//----------------------------------------------------------------------------
// D3DXMACRO:
// ----------
// Preprocessor macro definition.  The application pass in a NULL-terminated
// array of this structure to various D3DX APIs.  This enables the application
// to #define tokens at runtime, before the file is parsed.
//----------------------------------------------------------------------------

typedef struct _D3DXMACRO
{
    LPCSTR Name;
    LPCSTR Definition;

} D3DXMACRO, *LPD3DXMACRO;


//----------------------------------------------------------------------------
// D3DXSEMANTIC:
//----------------------------------------------------------------------------

typedef struct _D3DXSEMANTIC
{
    UINT Usage;
    UINT UsageIndex;

} D3DXSEMANTIC, *LPD3DXSEMANTIC;



//----------------------------------------------------------------------------
// D3DXREGISTER_SET:
//----------------------------------------------------------------------------

typedef enum _D3DXREGISTER_SET
{
    D3DXRS_BOOL,
    D3DXRS_INT4,
    D3DXRS_FLOAT4,
    D3DXRS_SAMPLER,

    // force 32-bit size enum
    D3DXRS_FORCE_DWORD = 0x7fffffff

} D3DXREGISTER_SET, *LPD3DXREGISTER_SET;


//----------------------------------------------------------------------------
// D3DXPARAMETER_CLASS:
//----------------------------------------------------------------------------

typedef enum _D3DXPARAMETER_CLASS
{
    D3DXPC_SCALAR,
    D3DXPC_VECTOR,
    D3DXPC_MATRIX_ROWS,
    D3DXPC_MATRIX_COLUMNS,
    D3DXPC_OBJECT,
    D3DXPC_STRUCT,

    // force 32-bit size enum
    D3DXPC_FORCE_DWORD = 0x7fffffff

} D3DXPARAMETER_CLASS, *LPD3DXPARAMETER_CLASS;


//----------------------------------------------------------------------------
// D3DXPARAMETER_TYPE:
//----------------------------------------------------------------------------

typedef enum _D3DXPARAMETER_TYPE
{
    D3DXPT_VOID,
    D3DXPT_BOOL,
    D3DXPT_INT,
    D3DXPT_FLOAT,
    D3DXPT_STRING,
    D3DXPT_TEXTURE,
    D3DXPT_TEXTURE1D,
    D3DXPT_TEXTURE2D,
    D3DXPT_TEXTURE3D,
    D3DXPT_TEXTURECUBE,
    D3DXPT_SAMPLER,
    D3DXPT_SAMPLER1D,
    D3DXPT_SAMPLER2D,
    D3DXPT_SAMPLER3D,
    D3DXPT_SAMPLERCUBE,
    D3DXPT_PIXELSHADER,
    D3DXPT_VERTEXSHADER,
    D3DXPT_PIXELFRAGMENT,
    D3DXPT_VERTEXFRAGMENT,
    D3DXPT_UNSUPPORTED,

    // force 32-bit size enum
    D3DXPT_FORCE_DWORD = 0x7fffffff

} D3DXPARAMETER_TYPE, *LPD3DXPARAMETER_TYPE;


//----------------------------------------------------------------------------
// D3DXCONSTANTTABLE_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXCONSTANTTABLE_DESC
{
    LPCSTR Creator;                     // Creator string
    DWORD Version;                      // Shader version
    UINT Constants;                     // Number of constants

} D3DXCONSTANTTABLE_DESC, *LPD3DXCONSTANTTABLE_DESC;


//----------------------------------------------------------------------------
// D3DXCONSTANT_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXCONSTANT_DESC
{
    LPCSTR Name;                        // Constant name

    D3DXREGISTER_SET RegisterSet;       // Register set
    UINT RegisterIndex;                 // Register index
    UINT RegisterCount;                 // Number of registers occupied

    D3DXPARAMETER_CLASS Class;          // Class
    D3DXPARAMETER_TYPE Type;            // Component type

    UINT Rows;                          // Number of rows
    UINT Columns;                       // Number of columns
    UINT Elements;                      // Number of array elements
    UINT StructMembers;                 // Number of structure member sub-parameters

    UINT Bytes;                         // Data size, in bytes
    LPCVOID DefaultValue;               // Pointer to default value

} D3DXCONSTANT_DESC, *LPD3DXCONSTANT_DESC;



//----------------------------------------------------------------------------
// ID3DXConstantTable:
//----------------------------------------------------------------------------

typedef interface ID3DXConstantTable ID3DXConstantTable;
typedef interface ID3DXConstantTable *LPD3DXCONSTANTTABLE;

// {AB3C758F-093E-4356-B762-4DB18F1B3A01}
DEFINE_GUID(IID_ID3DXConstantTable, 
0xab3c758f, 0x93e, 0x4356, 0xb7, 0x62, 0x4d, 0xb1, 0x8f, 0x1b, 0x3a, 0x1);


#undef INTERFACE
#define INTERFACE ID3DXConstantTable

DECLARE_INTERFACE_(ID3DXConstantTable, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Buffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXCONSTANTTABLE_DESC *pDesc) PURE;
    STDMETHOD(GetConstantDesc)(THIS_ D3DXHANDLE hConstant, D3DXCONSTANT_DESC *pConstantDesc, UINT *pCount) PURE;
    STDMETHOD_(UINT, GetSamplerIndex)(THIS_ D3DXHANDLE hConstant) PURE;

    // Handle operation
    STDMETHOD_(D3DXHANDLE, GetConstant)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantByName)(THIS_ D3DXHANDLE hConstant, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantElement)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;

    // Set Constants
    STDMETHOD(SetDefaults)(THIS_ LPDIRECT3DDEVICE9 pDevice) PURE;
    STDMETHOD(SetValue)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, BOOL b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, INT n) PURE;
    STDMETHOD(SetIntArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, FLOAT f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID* ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST LPVOID* ppMatrix, UINT Count) PURE;
};


namespace tbf
{
  namespace RenderFix
  {
    void Init     (void);
    void Shutdown (void);

    void Reset    ( IDirect3DDevice9      *This,
                    D3DPRESENT_PARAMETERS *pPresentationParameters );

    void TriggerReset (void);

    bool InstallSGSSAA (void);

    void ClearBlackBars (void);

    // Indicates whether setting changes need a device reset
    struct reset_state_s {
      bool textures = false;
      bool graphics = false;
    } extern need_reset;

    class CommandProcessor : public SK_IVariableListener {
    public:
      CommandProcessor (void);

      virtual bool OnVarChange (SK_IVariable* var, void* val = NULL);

      static CommandProcessor* getInstance (void)
      {
        if (pCommProc == NULL)
          pCommProc = new CommandProcessor ();

        return pCommProc;
      }

    protected:
      SK_IVariable* fovy_;
      SK_IVariable* aspect_ratio_;

    private:
      static CommandProcessor* pCommProc;
    };

    extern bool               fullscreen;

    extern uint32_t           width;
    extern uint32_t           height;

    extern IDirect3DSurface9* pPostProcessSurface;
    extern IDirect3DDevice9*  pDevice;
    extern HWND               hWndDevice;

    extern volatile ULONG     dwRenderThreadID;
    extern bool               bink; // True if a bink video is playing

    extern HMODULE            d3dx9_43_dll;
    extern HMODULE            user32_dll;

    struct tbf_draw_states_s {
      bool            has_aniso       = false; // Has he game even once set anisotropy?!
      int             max_aniso       = 4;

      D3DVIEWPORT9    vp              = { 0 };
      bool            postprocessing  = false;
      bool            fullscreen      = false;

      // Most of these states are not tracked
      DWORD           srcblend        = 0;
      DWORD           dstblend        = 0;
      DWORD           srcalpha        = 0;     // Separate Alpha Blend Eq: Src
      DWORD           dstalpha        = 0;     // Separate Alpha Blend Eq: Dst
      bool            alpha_test      = false; // Test Alpha?
      DWORD           alpha_ref       = 0;     // Value to test.
      bool            zwrite          = false; // Depth Mask

      // This one is
      bool            scissor_test    = false;

      int             last_vs_vec4    = 0; // Number of vectors in the last call to
                                          //   set vertex shader constant...

      int             draws           = 0; // Number of draw calls
      int             frames          = 0;

      bool            cegui_active    = false;
      
      bool            fullscreen_blit = false;
      bool            fix_scissor     = true;
      int             instances       = 0;

      uint32_t        current_tex  [256];
      float           mat4_0       [16] = { 0.0f };
      float           viewport_off [4]  = { 0.0f }; // Most vertex shaders use this and we can
                                              //   test the set values to determine if a draw
                                              //     is HUD or world-space.
    } extern draw_state;

    struct frame_state_s
    {
      void clear (void) { pixel_shaders.clear (); vertex_shaders.clear (); vertex_buffers.dynamic.clear (); vertex_buffers.immutable.clear (); }
    
      std::unordered_set <uint32_t>                 pixel_shaders;
      std::unordered_set <uint32_t>                 vertex_shaders;

      struct {
        std::unordered_set <IDirect3DVertexBuffer9 *> dynamic;
        std::unordered_set <IDirect3DVertexBuffer9 *> immutable;
      } vertex_buffers;
    } extern last_frame;

    struct known_objects_s
    {
      void clear (void) { static_vbs.clear (); dynamic_vbs.clear (); };

      std::unordered_set <IDirect3DVertexBuffer9 *> static_vbs;
      std::unordered_set <IDirect3DVertexBuffer9 *> dynamic_vbs;
    } extern known_objs;

    struct render_target_class_s
    {
      struct shadow_s
      {
        struct resolution_s
        {
          const UINT Low, Med, High;             //   Low      Medium     High
        } const None        { 0,      0,    0 }, //------------------------------
                Character   { 64,   128,  256 }, //  64x64,   128x128,   256x256
                Environment { 512, 1024, 2048 }; // 512x512, 1024x1024, 2048x2048
    
        const resolution_s* type = &None;
      } shadow;
    
      struct {
        enum {
          None         = 0x0,
          DepthOfField = 0x1,
          Bloom        = 0x2,
          Reflection   = 0x4,
          Map          = 0x8
        } type = None;
      } postproc;
    };

    struct aspect_ratio_data_s
    {
      struct {
        std::unordered_set <uint32_t> pixel_shaders;
        std::unordered_set <uint32_t> vertex_shaders;
        std::unordered_set <uint32_t> textures;
      } whitelist, blacklist;
      
    } extern aspect_ratio_data;

    struct render_target_tracking_s
    {
      void clear (void) { pixel_shaders.clear (); vertex_shaders.clear (); active = false; }

      IDirect3DBaseTexture9*        tracking_tex  = nullptr;

      std::unordered_set <uint32_t> pixel_shaders;
      std::unordered_set <uint32_t> vertex_shaders;

      bool                          active        = false;
    } extern tracked_rt;

    struct shader_tracking_s
    {
      void clear (void) {
        active    = false;
        num_draws = 0;
        used_textures.clear ();

        for (int i = 0; i < 16; i++)
          current_textures [i] = 0x00;
      }

      void use (IUnknown* pShader);

      uint32_t                      crc32        =  0x00;
      bool                          cancel_draws = false;
      bool                          clamp_coords = false;
      bool                          active       = false;
      int                           num_draws    =     0;
      std::unordered_set <uint32_t>    used_textures;
                          uint32_t  current_textures [16];

      //std::vector <IDirect3DBaseTexture9 *> samplers;

      IUnknown*                     shader_obj  = nullptr;
      ID3DXConstantTable*           ctable      = nullptr;

      struct shader_constant_s
      {
        char                Name [128];
        D3DXREGISTER_SET    RegisterSet;
        UINT                RegisterIndex;
        UINT                RegisterCount;
        D3DXPARAMETER_CLASS Class;
        D3DXPARAMETER_TYPE  Type;
        UINT                Rows;
        UINT                Columns;
        UINT                Elements;
        std::vector <shader_constant_s>
                            struct_members;
        bool                Override;
        float               Data [4]; // TEMP HACK
      };

      std::vector <shader_constant_s> constants;
    } extern tracked_vs, tracked_ps;

    struct vertex_buffer_tracking_s
    {
      void clear (void) {
        active = false; num_draws = 0; instances = 0;

        vertex_shaders.clear ();
        pixel_shaders.clear  ();
        textures.clear       ();

        for (auto it : vertex_decls) it->Release ();

        vertex_decls.clear ();
      }

      void use (void)
      {
        extern uint32_t vs_checksum, ps_checksum;

        vertex_shaders.emplace (vs_checksum);
        pixel_shaders.emplace  (ps_checksum);

        IDirect3DVertexDeclaration9* decl = nullptr;

        if (SUCCEEDED (tbf::RenderFix::pDevice->GetVertexDeclaration (&decl)))
        {
          static D3DVERTEXELEMENT9 elem_decl [MAXD3DDECLLENGTH];
          static UINT              num_elems;

          // Run through the vertex decl and figure out which samplers have texcoords,
          //   that is a pretty good indicator as to which textures are actually used...
          if (SUCCEEDED (decl->GetDeclaration (elem_decl, &num_elems)))
          {
            for ( UINT i = 0; i < num_elems; i++ )
            {
              if (elem_decl [i].Usage == D3DDECLUSAGE_TEXCOORD)
                textures.emplace (draw_state.current_tex [elem_decl [i].UsageIndex]);
            }
          }

          if (! vertex_decls.count (decl))
            vertex_decls.emplace (decl);
          else
            decl->Release ();
        }
      }

      IDirect3DVertexBuffer9*       vertex_buffer = nullptr;

      std::unordered_set <
        IDirect3DVertexDeclaration9*
      >                             vertex_decls;

      //uint32_t                      crc32        =  0x00;
      bool                          cancel_draws  = false;
      bool                          wireframe     = false;
      bool                          active        = false;
      int                           num_draws     =     0;
      int                           instanced     =     0;
      int                           instances     =     1;

      std::unordered_set <uint32_t> vertex_shaders;
      std::unordered_set <uint32_t> pixel_shaders;
      std::unordered_set <uint32_t> textures;

      std::unordered_set <IDirect3DVertexBuffer9 *>
                                    wireframes;
    } extern tracked_vb;

    struct shader_disasm_s {
      std::string header;
      std::string code;
      std::string footer;
    };

    extern std::unordered_set <IDirect3DBaseTexture9 *> ui_map_rts;
    extern bool                                         ui_map_active;
  }
}

const uint32_t START_FLAG = 0xF1D19C;
const int      MAX_FLAGS  = 40;

extern uintptr_t
TBF_GetBaseAddr (void);

__forceinline
uintptr_t
TBF_GetFlagBase (void)
{
  return TBF_GetBaseAddr () + START_FLAG;
}

__forceinline
uint8_t*
TBF_GetFlagFromIdx (int idx)
{
  return (uint8_t *)TBF_GetFlagBase () + idx;
}

struct game_state_t {
  BYTE*  base_addr    =  (BYTE *)0xF1D19C;
  bool   in_skit      = false;

  struct {
    bool     want  = false;
    uint64_t frame = 0;

    void set (void) {
      want  = true;
      frame = 0;
    }

    void clear (void) {
      want  = false;
      frame = 0;
    }

    bool test (void) {
      return want;
    }
  } clear_enemies, respawn_enemies;

  // Applies to Tales of Zestiria only, flags have been
  //   shifted around in Berseria
  struct data_t {
    /*  0 */ BYTE  Unknown0;
    /*  1 */ BYTE  Unknown1;
    /*  2 */ BYTE  Unknown2;
    /*  3 */ BYTE  Unknown3;
    /*  4 */ BYTE  Unknown4;
    /*  5 */ BYTE  Unknown5;
    /*  6 */ BYTE  FMV0;
    /*  7 */ BYTE  TitleScreen;
    /*  8 */ BYTE  FieldCamera; // ??
    /*  9 */ BYTE  FieldPause;
    /* 10 */ BYTE  Unknown10;
    /* 11 */ BYTE  Unknown11;
    /* 12 */ BYTE  Explanation; // Also used in skits // Also when talking to an NPC
    /* 13 */ BYTE  Menu; 
    /* 14 */ BYTE  BattleCamera;
    /* 15 */ BYTE  Unknown15;
    /* 16 */ BYTE  BattleResults;
    /* 17 */ BYTE  Unknown17;
    /* 18 */ BYTE  Unknown18;
    /* 19 */ BYTE  Cutscene;
    /* 20 */ BYTE  Unknown20;
    /* 21 */ BYTE  Unknown21;
    /* 22 */ BYTE  Unknown22;
    /* 23 */ BYTE  Unknown23;
    /* 24 */ BYTE  Loading0;
    /* 25 */ BYTE  Loading1;
    /* 26 */ BYTE  Unknown26;
    /* 27 */ BYTE  Unknown27;
    /* 28 */ BYTE  Unknown28;
    /* 29 */ BYTE  Unknown29;
    /* 30 */ BYTE  GameMenu;
    /* 31 */ BYTE  FMV1;
    /* 32 */ BYTE  PartySpeak0;
    /* 33 */ BYTE  PartySpeak1; 
    /* 34 */ BYTE  Skit;
  };

#if 0
  BYTE*  Title        =  (BYTE *) base_addr;       // Title
  BYTE*  OpeningMovie =  (BYTE *)(base_addr + 1);  // Opening Movie

  BYTE*  Game         =  (BYTE *)(base_addr + 2);  // Game
  BYTE*  GamePause    =  (BYTE *)(base_addr + 3);  // Game Pause

  SHORT* Loading      = (SHORT *)(base_addr + 4);  // Why are there 2 states for this?

  BYTE*  Explanation  =  (BYTE *)(base_addr + 6);  // Explanation (+ Bink)?
  BYTE*  Menu         =  (BYTE *)(base_addr + 7);  // Menu

  BYTE*  Unknown0     =  (BYTE *)(base_addr + 8);  // Unknown
  BYTE*  Unknown1     =  (BYTE *)(base_addr + 9);  // Unknown - Appears to be battle related
  BYTE*  Unknown2     =  (BYTE *)(base_addr + 10); // Unknown

  BYTE*  Battle       =  (BYTE *)(base_addr + 11); // Battle
  BYTE*  BattlePause  =  (BYTE *)(base_addr + 12); // Battle Pause

  BYTE*  Unknown3     =  (BYTE *)(base_addr + 13); // Unknown
  BYTE*  Unknown4     =  (BYTE *)(base_addr + 14); // Unknown
  BYTE*  Unknown5     =  (BYTE *)(base_addr + 15); // Unknown
  BYTE*  Unknown6     =  (BYTE *)(base_addr + 16); // Unknown
  BYTE*  Unknown7     =  (BYTE *)(base_addr + 17); // Unknown
  BYTE*  Unknown8     =  (BYTE *)(base_addr + 18); // Unknown

  BYTE*  Cutscene     =  (BYTE *)(base_addr + 19); // Cutscene
#endif

  bool inBattle      (void) { return ((data_t *)base_addr)->BattleCamera & 0x1; }
  bool inCutscene    (void) { return ((data_t *)base_addr)->Cutscene     & 0x1; }
  bool inExplanation (void) { return ((data_t *)base_addr)->Explanation  & 0x1; }
  bool inSkit        (void) { return in_skit;                                  }
  bool inMenu        (void) { return ((data_t *)base_addr)->Menu        & 0x1; }
  bool isLoading     (void) { return (((data_t *)base_addr)->Loading0 & 0x1) ||
                                     (((data_t *)base_addr)->Loading1 & 0x1); }

  bool hasFixedAspect (void) {
    return false;
    //if (((data_t *)base_addr)->OpeningMovie ||
        //((data_t *)base_addr)->Title        ||
        //((data_t *)base_addr)->Menu         ||
        //in_skit)
      //return true;
    //return false;
  }
  bool needsFixedMouseCoords(void) {
    return false;
    //return (((data_t *)base_addr)->GamePause    ||
            //((data_t *)base_addr)->Menu         ||
            //((data_t *)base_addr)->BattlePause  ||
            //((data_t *)base_addr)->Title);
  }
} extern game_state;

#include "config.h"

struct aspect_ratio_s {
  float* addrs [8] = { nullptr };
  int    count     =     0;

  float* fov_addrs [32] = { nullptr };
  int    fov_count      =     0;
  float  fov_orig       = 60.0f;

  int    selector       = 0;


  void setFOV (float fov)
  {
    DWORD dwOld;

    //for (int i = 0; i < fov_count; i++)
    //{
      //if (*fov_addrs [i] == fov_orig)
      //{
        VirtualProtect (fov_addrs [selector], sizeof (float), PAGE_READWRITE, &dwOld);
        *((float *)fov_addrs [selector]) = fov;
        VirtualProtect (fov_addrs [selector], sizeof (float), dwOld,          &dwOld);
      //}
    //}

    fov_orig = fov;
  };

  void setAspectRatio (float aspect)
  {
    config.render.aspect_ratio = aspect;

    if (! config.render.aspect_correction)
      return;

    DWORD dwOld;
    //for (int i = 0; i < count; i++)
    //{
      VirtualProtect (addrs [1], sizeof (float), PAGE_READWRITE, &dwOld);
      *((float *)addrs [1]) = aspect;
      VirtualProtect (addrs [1], sizeof (float), dwOld,          &dwOld);
    //}
  };
} extern aspect_ratio;


typedef HRESULT (STDMETHODCALLTYPE *BeginScene_pfn)(
  IDirect3DDevice9 *This
);

typedef HRESULT (STDMETHODCALLTYPE *EndScene_pfn)(
  IDirect3DDevice9 *This
);

#include <d3d9.h>

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

typedef D3DPRESENT_PARAMETERS* (__stdcall *SK_SetPresentParamsD3D9_pfn)
(
  IDirect3DDevice9      *pDevice,
  D3DPRESENT_PARAMETERS *pParams
);

typedef HRESULT (STDMETHODCALLTYPE *DrawPrimitive_pfn)
(
  IDirect3DDevice9 *This,
  D3DPRIMITIVETYPE  PrimitiveType,
  UINT              StartVertex,
  UINT              PrimitiveCount
);

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive_pfn)
(
  IDirect3DDevice9 *This,
  D3DPRIMITIVETYPE  Type,
  INT               BaseVertexIndex,
  UINT              MinVertexIndex,
  UINT              NumVertices,
  UINT              startIndex,
  UINT              primCount
);

typedef HRESULT (STDMETHODCALLTYPE *DrawPrimitiveUP_pfn)
(
  IDirect3DDevice9 *This,
  D3DPRIMITIVETYPE  PrimitiveType,
  UINT              PrimitiveCount,
  const void       *pVertexStreamZeroData,
  UINT              VertexStreamZeroStride
);

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveUP_pfn)
(
  IDirect3DDevice9 *This,
  D3DPRIMITIVETYPE  PrimitiveType,
  UINT              MinVertexIndex,
  UINT              NumVertices,
  UINT              PrimitiveCount,
  const void       *pIndexData,
  D3DFORMAT         IndexDataFormat,
  const void       *pVertexStreamZeroData,
  UINT              VertexStreamZeroStride
);

typedef  HRESULT (STDMETHODCALLTYPE *CreateVertexBuffer_pfn)
( _In_  IDirect3DDevice9        *This,
  _In_  UINT                     Length,
  _In_  DWORD                    Usage,
  _In_  DWORD                    FVF,
  _In_  D3DPOOL                  Pool,
  _Out_ IDirect3DVertexBuffer9 **ppVertexBuffer,
  _In_  HANDLE                  *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *SetStreamSource_pfn)
(
  IDirect3DDevice9       *This,
  UINT                    StreamNumber,
  IDirect3DVertexBuffer9 *pStreamData,
  UINT                    OffsetInBytes,
  UINT                    Stride
);

typedef HRESULT (STDMETHODCALLTYPE *SetStreamSourceFreq_pfn)
( _In_ IDirect3DDevice9 *This,
  _In_ UINT              StreamNumber,
  _In_ UINT              FrequencyParameter
);

typedef BOOL (__stdcall *SKX_DrawExternalOSD_pfn)
( _In_ const char* szAppName,
  _In_ const char* szText
);

typedef HRESULT (STDMETHODCALLTYPE *SetRenderState_pfn)
( _In_ IDirect3DDevice9*  This,
  _In_ D3DRENDERSTATETYPE State,
  _In_ DWORD              Value
);

typedef HRESULT (STDMETHODCALLTYPE *TestCooperativeLevel_pfn)
( _In_ IDirect3DDevice9* This
);

typedef void    (STDMETHODCALLTYPE *SK_BeginBufferSwap_pfn)
(
);

typedef HRESULT (STDMETHODCALLTYPE *SK_EndBufferSwap_pfn)
( _In_    HRESULT   hr,
  _Inout_ IUnknown* device
);

typedef HRESULT (STDMETHODCALLTYPE *SetScissorRect_pfn)
( _In_       IDirect3DDevice9* This,
  _In_ const RECT*             pRect
);

typedef HRESULT (STDMETHODCALLTYPE *EndScene_pfn)
( _In_ IDirect3DDevice9* This
);

typedef HRESULT (STDMETHODCALLTYPE *SetSamplerState_pfn)
( _In_ IDirect3DDevice9*   This,
  _In_ DWORD               Sampler,
  _In_ D3DSAMPLERSTATETYPE Type,
  _In_ DWORD               Value
);

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_pfn)
( _In_     IDirect3DDevice9*       This,
  _In_opt_ IDirect3DVertexShader9* pShader
);

typedef HRESULT (STDMETHODCALLTYPE *SetPixelShader_pfn)
( _In_     IDirect3DDevice9*      This,
  _In_opt_ IDirect3DPixelShader9* pShader
);

typedef HRESULT (STDMETHODCALLTYPE *UpdateSurface_pfn)
( _In_       IDirect3DDevice9  *This,
  _In_       IDirect3DSurface9 *pSourceSurface,
  _In_ const RECT              *pSourceRect,
  _In_       IDirect3DSurface9 *pDestinationSurface,
  _In_ const POINT             *pDestinationPoint
);

typedef HRESULT (STDMETHODCALLTYPE *SetViewport_pfn)
( _In_       IDirect3DDevice9* This,
  _In_ CONST D3DVIEWPORT9*     pViewport
);

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_pfn)
( _In_       IDirect3DDevice9* This,
  _In_       UINT              StartRegister,
  _In_ CONST float*            pConstantData,
  _In_       UINT              Vector4fCount
);

typedef HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF_pfn)
( _In_       IDirect3DDevice9* This,
  _In_       UINT              StartRegister,
  _In_ CONST float*            pConstantData,
  _In_       UINT              Vector4fCount
);

typedef HRESULT (WINAPI *D3DXGetShaderConstantTable_pfn)
( _In_  const DWORD                *pFunction,
  _Out_       LPD3DXCONSTANTTABLE *ppConstantTable
);

typedef HRESULT (WINAPI *D3DXDisassembleShader_pfn)
( _In_  const DWORD         *pShader,
  _In_        BOOL            EnableColorCode,
  _In_        LPCSTR         pComments,
  _Out_       LPD3DXBUFFER *ppDisassembly
);

typedef HRESULT (__stdcall *Reset_pfn)
( _In_    IDirect3DDevice9      *This,
  _Inout_ D3DPRESENT_PARAMETERS *pPresentationParameters
);




extern SetSamplerState_pfn D3D9SetSamplerState_Original;



#endif /* __TZF__RENDER_H__ */