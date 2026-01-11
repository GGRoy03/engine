#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define D3D11_NO_HELPERS
#include <d3d11.h>
#include <dxgi1_3.h>

#pragma comment (lib, "d3d11")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "d3dcompiler")

#include "third_party/gui/gui.h"

#include "utilities.h"
#include "platform/platform.h"
#include "engine/rendering/renderer.h"
#include "engine/rendering/assets.h"
#include <engine/math/matrix.h>

#include "mesh_vertex_shader.h"
#include "mesh_pixel_shader.h"
#include "ui_vertex_shader.h"
#include "ui_pixel_shader.h"


#define MAX_MATERIAL_COUNT 64
#define MAX_STATIC_MESH_COUNT 128


typedef struct
{
    ID3D11Device           *Device;
    ID3D11DeviceContext    *DeviceContext;
    IDXGISwapChain1        *SwapChain;
    ID3D11RenderTargetView *RenderView;

    // Mesh Objects

    ID3D11InputLayout       *MeshInputLayout;
    ID3D11VertexShader      *MeshVertexShader;
    ID3D11PixelShader       *MeshPixelShader;
    ID3D11SamplerState      *MeshSamplerState;
    ID3D11RasterizerState   *MeshRasterizerState;
    ID3D11Buffer            *MeshTransformUniformBuffer;
    ID3D11Buffer            *MeshLightUniformBuffer;
    ID3D11DepthStencilView  *MeshDepthView;
    ID3D11DepthStencilState *MeshDepthState;

    // UI Objects

    ID3D11InputLayout     *UIInputLayout;
    ID3D11VertexShader    *UIVertexShader;
    ID3D11PixelShader     *UIPixelShader;
    ID3D11SamplerState    *UISamplerState;
    ID3D11RasterizerState *UIRasterizerState;
    ID3D11Buffer          *UIBatchUniformBuffer;
    ID3D11Buffer          *UIVertexBuffer;
} d3d11_renderer;


typedef struct
{
    mat4x4 World;
    mat4x4 View;
    mat4x4 Projection;
    vec3   CameraPosition;
    float _Pad0;
} d3d11_mesh_transform_data;


typedef struct
{
    light_source Lights[16];
    uint32_t     LightCount;
    vec3        _Pad0;
} d3d11_mesh_light_data;


typedef struct
{
    float          ScaleX, _Padding0, _Padding1, _Padding2;
    float          _Padding3, ScaleY, _Padding4, _Padding5;
    float          _Padding6, _Padding7, _Padding8, _Padding9;

    gui_dimensions ViewportSizeInPixel;
    gui_dimensions AtlasSizeInPixel;
} d3d11_ui_batch_data;


d3d11_renderer *
D3D11Initialize(HWND HWindow, int Width, int Height, memory_arena *Arena)
{
    d3d11_renderer *Result = PushStruct(Arena, d3d11_renderer);
    memset(Result, 0, sizeof(renderer));

    {
        UINT CreateFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
        CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL Levels[] = { D3D_FEATURE_LEVEL_11_0 };

        D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, CreateFlags, Levels, ARRAYSIZE(Levels), D3D11_SDK_VERSION, &Result->Device, 0, &Result->DeviceContext);
    }

    {
        IDXGIDevice *DXGIDevice = 0;
        Result->Device->lpVtbl->QueryInterface(Result->Device, &IID_IDXGIDevice, (void **)&DXGIDevice);

        if (DXGIDevice)
        {
            IDXGIAdapter *Adapter = 0;
            DXGIDevice->lpVtbl->GetAdapter(DXGIDevice, &Adapter);

            if (Adapter)
            {
                IDXGIFactory2 *Factory = 0;
                Adapter->lpVtbl->GetParent(Adapter, &IID_IDXGIFactory2, (void **)&Factory);

                if (Factory)
                {
                    DXGI_SWAP_CHAIN_DESC1 Desc = {0};
                    Desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
                    Desc.SampleDesc.Count = 1;
                    Desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                    Desc.BufferCount      = 2;
                    Desc.Scaling          = DXGI_SCALING_NONE;
                    Desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;

                    Factory->lpVtbl->CreateSwapChainForHwnd(Factory, (IUnknown *)Result->Device, HWindow, &Desc, 0, 0, &Result->SwapChain);
                    Factory->lpVtbl->MakeWindowAssociation(Factory, HWindow, DXGI_MWA_NO_ALT_ENTER);

                    Factory->lpVtbl->Release(Factory);
                }

                Adapter->lpVtbl->Release(Adapter);
            }

            DXGIDevice->lpVtbl->Release(DXGIDevice);
        }
    }

    {
        ID3D11Texture2D *BackBuffer = 0;
        Result->SwapChain->lpVtbl->GetBuffer(Result->SwapChain, 0, &IID_ID3D11Texture2D, (void **)&BackBuffer);

        if (BackBuffer)
        {
            Result->Device->lpVtbl->CreateRenderTargetView(Result->Device, (ID3D11Resource *)BackBuffer, 0, &Result->RenderView);

            BackBuffer->lpVtbl->Release(BackBuffer);
        }
    }

    {
        {
            D3D11_INPUT_ELEMENT_DESC InputLayout[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

            Result->Device->lpVtbl->CreateVertexShader(Result->Device, MeshVertexShaderBytes, sizeof(MeshVertexShaderBytes), 0, &Result->MeshVertexShader);
            Result->Device->lpVtbl->CreatePixelShader(Result->Device, MeshPixelShaderBytes, sizeof(MeshPixelShaderBytes), 0, &Result->MeshPixelShader);
            Result->Device->lpVtbl->CreateInputLayout(Result->Device, InputLayout, ARRAYSIZE(InputLayout), MeshVertexShaderBytes, sizeof(MeshVertexShaderBytes), &Result->MeshInputLayout);
        }

        {
            D3D11_INPUT_ELEMENT_DESC InputLayout[] =
            {
                {"POS" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"FONT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"COL" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"COL" , 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"COL" , 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"COL" , 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"CORR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"STY" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            };

            Result->Device->lpVtbl->CreateVertexShader(Result->Device, UIVertexShaderBytes, sizeof(UIVertexShaderBytes), 0, &Result->UIVertexShader);
            Result->Device->lpVtbl->CreatePixelShader (Result->Device, UIPixelShaderBytes , sizeof(UIPixelShaderBytes) , 0, &Result->UIPixelShader );
            Result->Device->lpVtbl->CreateInputLayout (Result->Device, InputLayout, ARRAYSIZE(InputLayout), UIVertexShaderBytes, sizeof(UIVertexShaderBytes), &Result->UIInputLayout);
        }
    }

    {
        {
            D3D11_BUFFER_DESC Desc = {0};
            Desc.ByteWidth      = sizeof(d3d11_mesh_transform_data);
            Desc.Usage          = D3D11_USAGE_DYNAMIC;
            Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            
            Result->Device->lpVtbl->CreateBuffer(Result->Device,&Desc,0, &Result->MeshTransformUniformBuffer);
        }

        {
            D3D11_BUFFER_DESC Desc =
            {
                .ByteWidth      = sizeof(d3d11_mesh_light_data),
                .Usage          = D3D11_USAGE_DYNAMIC,
                .BindFlags      = D3D11_BIND_CONSTANT_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            
            };
            
            Result->Device->lpVtbl->CreateBuffer(Result->Device,&Desc,0, &Result->MeshLightUniformBuffer);
        }

        {
            D3D11_BUFFER_DESC Desc =
            {
                .ByteWidth      = sizeof(d3d11_ui_batch_data),
                .Usage          = D3D11_USAGE_DYNAMIC,
                .BindFlags      = D3D11_BIND_CONSTANT_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            
            };
            
            Result->Device->lpVtbl->CreateBuffer(Result->Device, &Desc,0, &Result->UIBatchUniformBuffer);
        }
    }

    {
        D3D11_BUFFER_DESC Desc =
        {
            .ByteWidth      = KiB(64),
            .Usage          = D3D11_USAGE_DYNAMIC,
            .BindFlags      = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        Result->Device->lpVtbl->CreateBuffer(Result->Device, &Desc, NULL, &Result->UIVertexBuffer);
    }

    {
        {
            D3D11_SAMPLER_DESC Desc = {0};
            Desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            Desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.MaxAnisotropy  = 1;
            Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            Desc.MaxLOD         = D3D11_FLOAT32_MAX;

            Result->Device->lpVtbl->CreateSamplerState(Result->Device, &Desc, &Result->MeshSamplerState);
        }

        {
            D3D11_SAMPLER_DESC Desc = {0};
            Desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            Desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
            Desc.MaxAnisotropy  = 1;
            Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            Desc.MaxLOD         = D3D11_FLOAT32_MAX;

            Result->Device->lpVtbl->CreateSamplerState(Result->Device, &Desc, &Result->UISamplerState);
        }
    }

    {
        D3D11_TEXTURE2D_DESC DepthBufferDesc =
        {
            .Width            = Width,
            .Height           = Height,
            .MipLevels        = 1,
            .ArraySize        = 1,
            .Format           = DXGI_FORMAT_D32_FLOAT,
            .SampleDesc.Count = 1,
            .Usage            = D3D11_USAGE_DEFAULT,
            .BindFlags        = D3D11_BIND_DEPTH_STENCIL,
        };

        ID3D11Texture2D *DepthBuffer = 0;
        Result->Device->lpVtbl->CreateTexture2D(Result->Device, &DepthBufferDesc, 0, &DepthBuffer);

        D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc =
        {
            .Format        = DepthBufferDesc.Format,
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        };

        Result->Device->lpVtbl->CreateDepthStencilView(Result->Device, (ID3D11Resource *)DepthBuffer, &DSVDesc, &Result->MeshDepthView);

        DepthBuffer->lpVtbl->Release(DepthBuffer);

        D3D11_DEPTH_STENCIL_DESC DepthStencilDesc =
        {
            .DepthEnable    = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc      = D3D11_COMPARISON_LESS,
            .StencilEnable  = FALSE,
        };

        Result->Device->lpVtbl->CreateDepthStencilState(Result->Device, &DepthStencilDesc, &Result->MeshDepthState);
    }

    {
        {
            D3D11_RASTERIZER_DESC Desc = {0};
            Desc.FillMode              = D3D11_FILL_SOLID;
            Desc.CullMode              = D3D11_CULL_BACK;
            Desc.FrontCounterClockwise = TRUE;
            Desc.DepthClipEnable       = TRUE;
            Desc.ScissorEnable         = FALSE;
            Desc.MultisampleEnable     = FALSE;
            Desc.AntialiasedLineEnable = FALSE;

            Result->Device->lpVtbl->CreateRasterizerState(Result->Device, &Desc, &Result->MeshRasterizerState);
        }

        {
            D3D11_RASTERIZER_DESC Desc = {0};
            Desc.FillMode              = D3D11_FILL_SOLID;
            Desc.CullMode              = D3D11_CULL_NONE;
            Desc.FrontCounterClockwise = TRUE;
            Desc.DepthClipEnable       = TRUE;
            Desc.ScissorEnable         = FALSE;
            Desc.MultisampleEnable     = FALSE;
            Desc.AntialiasedLineEnable = FALSE;

            Result->Device->lpVtbl->CreateRasterizerState(Result->Device, &Desc, &Result->UIRasterizerState);
        }
    }

    return Result;
}


// ==============================================
// <Resources>
// ==============================================


void *
RendererCreateVertexBuffer(void *Data, uint64_t Size, renderer *Renderer)
{
    ID3D11Buffer *Result = 0;

    if (Data && Size && Renderer)
    {
        d3d11_renderer *D3D11 = (d3d11_renderer *)Renderer->Backend;
        ID3D11Device   *Device = D3D11->Device;

        D3D11_BUFFER_DESC Desc =
        {
            .ByteWidth           = Size,
            .Usage               = D3D11_USAGE_DEFAULT,
            .BindFlags           = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags      = 0,
            .MiscFlags           = 0,
            .StructureByteStride = 0,
        };

        D3D11_SUBRESOURCE_DATA InitialData =
        {
            .pSysMem          = Data,
            .SysMemPitch      = 0,
            .SysMemSlicePitch = 0,
        };

        Device->lpVtbl->CreateBuffer(Device, &Desc, &InitialData, &Result);
    }

    return Result;
}

void *
RendererCreateTexture(loaded_texture LoadedTexture, renderer *Renderer)
{
    ID3D11ShaderResourceView *Result = 0;

    // The BytesPerPixel check is artificial and should be removed.

    if (LoadedTexture.Data && LoadedTexture.Width && LoadedTexture.Height && LoadedTexture.BytesPerPixel == 4)
    {
        d3d11_renderer *D3D11 = (d3d11_renderer *)Renderer->Backend;
        ID3D11Device   *Device = D3D11->Device;

        D3D11_TEXTURE2D_DESC TextureDesc =
        {
            .Width              = LoadedTexture.Width,
            .Height             = LoadedTexture.Height,
            .MipLevels          = 1,
            .ArraySize          = 1,
            .Format             = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc.Count   = 1,
            .SampleDesc.Quality = 0,
            .Usage              = D3D11_USAGE_IMMUTABLE,
            .BindFlags          = D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags     = 0,
            .MiscFlags          = 0,
        };
        
        D3D11_SUBRESOURCE_DATA InitialData =
        {
            .pSysMem     = LoadedTexture.Data,
            .SysMemPitch = LoadedTexture.Width * LoadedTexture.BytesPerPixel,
        };
        
        ID3D11Texture2D *Texture = 0;
        Device->lpVtbl->CreateTexture2D(Device, &TextureDesc, &InitialData, &Texture);
        if (Texture)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC TextureViewDesc =
            {
                .Format                    = TextureDesc.Format,
                .ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D,
                .Texture2D.MostDetailedMip = 0,
                .Texture2D.MipLevels       = 1,
            };
        
            Device->lpVtbl->CreateShaderResourceView(Device, (ID3D11Resource *)Texture, &TextureViewDesc, &Result);

            Texture->lpVtbl->Release(Texture);
        }
    }

    return Result;
}


// ==============================================
// <Drawing>
// ==============================================


void
RendererEnterFrame(clear_color Color, renderer *Renderer)
{
    d3d11_renderer         *D3D11      =  (d3d11_renderer *)Renderer->Backend;
    ID3D11DeviceContext    *Context    = D3D11->DeviceContext;
    ID3D11RenderTargetView *RenderView = D3D11->RenderView;

    Context->lpVtbl->ClearRenderTargetView(Context, RenderView, (FLOAT *)&Color);
    Context->lpVtbl->ClearDepthStencilView(Context, D3D11->MeshDepthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


void
RendererLeaveFrame(int Width, int Height, engine_memory *EngineMemory, renderer *Renderer)
{
    d3d11_renderer      *D3D11   = (d3d11_renderer *)Renderer->Backend;
    ID3D11DeviceContext *Context = D3D11->DeviceContext;

    {
        D3D11_VIEWPORT Viewport = { 0.f, 0.f, Width, Height, 0.f, 1.f };
        Context->lpVtbl->RSSetViewports(Context, 1, &Viewport);

        Context->lpVtbl->OMSetRenderTargets(Context, 1, &D3D11->RenderView, 0);
    }

    for (render_pass_node *PassNode = Renderer->PassList.First; PassNode != 0; PassNode = PassNode->Next)
    {
        render_pass *Pass = &PassNode->Value;

        switch (Pass->Type)
        {

        case RenderPass_Mesh:
        {
            Context->lpVtbl->RSSetState(Context, D3D11->MeshRasterizerState);
            Context->lpVtbl->IASetPrimitiveTopology(Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context->lpVtbl->IASetInputLayout(Context, D3D11->MeshInputLayout);
            Context->lpVtbl->VSSetShader(Context, D3D11->MeshVertexShader, 0, 0);
            Context->lpVtbl->PSSetShader(Context, D3D11->MeshPixelShader, 0, 0);
            Context->lpVtbl->OMSetDepthStencilState(Context, D3D11->MeshDepthState, 0);
            Context->lpVtbl->PSSetSamplers(Context, 0, 1, &D3D11->MeshSamplerState);

            render_pass_params_mesh *PassParams = &Pass->Params.Mesh;

            for (mesh_group_node *GroupNode = PassParams->First; GroupNode != 0; GroupNode = GroupNode->Next)
            {
                mesh_group_params *GroupParams = &GroupNode->Params;

                d3d11_mesh_transform_data Transform =
                {
                    .World          = GroupParams->WorldMatrix,
                    .View           = GroupParams->ViewMatrix,
                    .Projection     = GroupParams->ProjectionMatrix,
                    .CameraPosition = GroupParams->CameraPosition,
                };

                d3d11_mesh_light_data Light =
                {
                    .LightCount = GroupParams->LightCount,
                };
                memcpy(Light.Lights, GroupParams->Lights, GroupParams->LightCount * sizeof(light_source));

                {
                    D3D11_MAPPED_SUBRESOURCE Mapped;
                    Context->lpVtbl->Map(Context, (ID3D11Resource *)D3D11->MeshTransformUniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
                    if (Mapped.pData)
                    {
                        memcpy(Mapped.pData, &Transform, sizeof(d3d11_mesh_transform_data));
                        Context->lpVtbl->Unmap(Context, (ID3D11Resource *)D3D11->MeshTransformUniformBuffer, 0);
                    }
                }

                {
                    D3D11_MAPPED_SUBRESOURCE Mapped;
                    Context->lpVtbl->Map(Context, (ID3D11Resource *)D3D11->MeshLightUniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
                    if (Mapped.pData)
                    {
                        memcpy(Mapped.pData, &Light, sizeof(d3d11_mesh_light_data));
                        Context->lpVtbl->Unmap(Context, (ID3D11Resource *)D3D11->MeshLightUniformBuffer, 0);
                    }
                }

                // TODO: Group these into an array so we can do a single upload.
                Context->lpVtbl->VSSetConstantBuffers(Context, 0, 1, &D3D11->MeshTransformUniformBuffer);
                Context->lpVtbl->PSSetConstantBuffers(Context, 1, 1, &D3D11->MeshLightUniformBuffer);

                // Iterate batches (tier 2: material params)
                for (render_batch_node *BatchNode = GroupNode->BatchList.First; BatchNode != 0; BatchNode = BatchNode->Next)
                {
                    render_batch      *Batch       = &BatchNode->Value;
                    mesh_batch_params *BatchParams = &BatchNode->MeshParams;

                    {
                        renderer_material *Material = AccessUnderlyingResource(BatchParams->Material, Renderer->Resources);
                        assert(Material);

                        renderer_backend_resource *ColorResource     = AccessUnderlyingResource(Material->Maps[MaterialMap_Color], Renderer->Resources);
                        ID3D11ShaderResourceView  *ColorView         = ColorResource ? (ID3D11ShaderResourceView *)ColorResource->Data : 0;
                        renderer_backend_resource *NormalResource    = AccessUnderlyingResource(Material->Maps[MaterialMap_Normal], Renderer->Resources);
                        ID3D11ShaderResourceView  *NormalView        = ColorResource ? (ID3D11ShaderResourceView *)NormalResource->Data : 0;
                        renderer_backend_resource *RoughnessResource = AccessUnderlyingResource(Material->Maps[MaterialMap_Roughness], Renderer->Resources);
                        ID3D11ShaderResourceView  *RoughnessView     = ColorResource ? (ID3D11ShaderResourceView *)RoughnessResource->Data : 0;

                        Context->lpVtbl->PSSetShaderResources(Context, 0, 1, &ColorView);
                        Context->lpVtbl->PSSetShaderResources(Context, 1, 1, &NormalView);
                        Context->lpVtbl->PSSetShaderResources(Context, 2, 1, &RoughnessView);
                    }

                    // Rewrite this.

                    uint32_t CommandCount = Batch->ByteCount / sizeof(render_command);
                    uint64_t AtByte       = 0;

                    for (uint32_t CommandIdx = 0; CommandIdx < CommandCount; ++CommandIdx)
                    {
                        render_command *Command = Batch->Memory + AtByte;

                        switch (Command->Type)
                        {

                        case RenderCommand_StaticGeometry:
                        {
                            renderer_static_mesh *StaticMesh = AccessUnderlyingResource(Command->StaticGeometry.MeshHandle, Renderer->Resources);
                            assert(StaticMesh);

                            // Bind vertex buffer
                            {
                                renderer_backend_resource *VertexBufferBD = AccessUnderlyingResource(StaticMesh->VertexBuffer, Renderer->Resources);
                                ID3D11Buffer              *VertexBuffer   = (ID3D11Buffer *)VertexBufferBD->Data;

                                UINT32 Stride = sizeof(mesh_vertex_data);
                                UINT32 Offset = 0;
                                Context->lpVtbl->IASetVertexBuffers(Context, 0, 1, &VertexBuffer, &Stride, &Offset);
                            }

                            // Draw each submesh (they all share the material from the batch)
                            for (uint32_t SubmeshIdx = 0; SubmeshIdx < StaticMesh->SubmeshCount; ++SubmeshIdx)
                            {
                                renderer_static_submesh *Submesh = &StaticMesh->Submeshes[SubmeshIdx];
                                Context->lpVtbl->Draw(Context, Submesh->VertexCount, Submesh->VertexStart);
                            }
                        } break;

                        default:
                        {
                            assert(!"INVALID ENGINE STATE");
                        } break;

                        }

                        AtByte += sizeof(render_command);
                    }
                }
            }
        } break;


        case RenderPass_UI:
        {
            Context->lpVtbl->RSSetState(Context, D3D11->UIRasterizerState);

            render_pass_params_ui *PassParams = &Pass->Params.UI;

            for (ui_group_node *GroupNode = PassParams->First; GroupNode != 0; GroupNode = GroupNode->Next)
            {
                ui_group_params  *GroupParams = &GroupNode->Params;
                render_batch_list BatchList   = GroupNode->BatchList;

                ID3D11Buffer *VertexBuffer  = D3D11->UIVertexBuffer;
                uint64_t      InstanceCount = 0;
                {
                    D3D11_MAPPED_SUBRESOURCE Resource = { 0 };
                    Context->lpVtbl->Map(Context, (ID3D11Resource *)VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Resource);

                    uint8_t *WritePointer = Resource.pData;
                    uint64_t WriteOffset = 0;

                    for (render_batch_node *BatchNode = BatchList.First; BatchNode != 0; BatchNode = BatchNode->Next)
                    {
                        render_batch Batch     = BatchNode->Value;
                        uint64_t     WriteSize = Batch.ByteCount;

                        memcpy(WritePointer + WriteOffset, Batch.Memory, WriteSize);

                        WriteOffset   += WriteSize;
                        InstanceCount += WriteSize / BatchList.BytesPerInstance;
                    }

                    Context->lpVtbl->Unmap(Context, (ID3D11Resource *)VertexBuffer, 0);
                }

                ID3D11Buffer *UniformBuffer = D3D11->UIBatchUniformBuffer;
                {
                    d3d11_ui_batch_data Uniform =
                    {
                        .ScaleX              = 1.f,
                        .ScaleY              = 1.f,
                        .ViewportSizeInPixel = (gui_dimensions){Width, Height},
                        .AtlasSizeInPixel    = (gui_dimensions){0.f, 0.f },
                    };

                    D3D11_MAPPED_SUBRESOURCE Resource = {};
                    Context->lpVtbl->Map(Context, (ID3D11Resource *)UniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Resource);
                    memcpy(Resource.pData, &Uniform, sizeof(Uniform));
                    Context->lpVtbl->Unmap(Context, (ID3D11Resource *)UniformBuffer, 0);
                }

                // Input Assembler
                {
                    uint32_t Stride = BatchList.BytesPerInstance;
                    uint32_t Offset = 0;

                    Context->lpVtbl->IASetVertexBuffers(Context, 0, 1, &VertexBuffer, &Stride, &Offset);
                    Context->lpVtbl->IASetInputLayout(Context, D3D11->UIInputLayout);
                    Context->lpVtbl->IASetPrimitiveTopology(Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                }

                // Vertex Shader
                {
                    Context->lpVtbl->VSSetShader(Context, D3D11->UIVertexShader, 0, 0);
                    Context->lpVtbl->VSSetConstantBuffers(Context, 0, 1, &UniformBuffer);
                }

                // Pixel Shader
                {
                    Context->lpVtbl->PSSetShader(Context, D3D11->UIPixelShader, 0, 0);
                }

                Context->lpVtbl->DrawInstanced(Context, 4, InstanceCount, 0, 0);
            }
        } break;

        default:
        {
            assert(!"INVALID ENGINE STATE");
        } break;
        }
    }

    D3D11->SwapChain->lpVtbl->Present(D3D11->SwapChain, 0, 0);

    Renderer->PassList.First = 0;
    Renderer->PassList.Last  = 0;
}