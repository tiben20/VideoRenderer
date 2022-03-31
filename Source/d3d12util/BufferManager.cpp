//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "stdafx.h"
#include "BufferManager.h"
//#include "Display.h"
#include "CommandContext.h"
#include "EsramAllocator.h"
//#include "TemporalEffects.h"

namespace D3D12Engine
{
    ColorBuffer g_SceneColorBuffer;
    ColorBuffer g_SceneNormalBuffer;
    ColorBuffer g_PostEffectsBuffer;
    ColorBuffer g_VelocityBuffer;
    ColorBuffer g_OverlayBuffer;
    ColorBuffer g_HorizontalBuffer;

    ColorBuffer g_SSAOFullScreen(Color(1.0f, 1.0f, 1.0f));
    ColorBuffer g_LinearDepth[2];
    ColorBuffer g_MinMaxDepth8;
    ColorBuffer g_MinMaxDepth16;
    ColorBuffer g_MinMaxDepth32;
    ColorBuffer g_DepthDownsize1;
    ColorBuffer g_DepthDownsize2;
    ColorBuffer g_DepthDownsize3;
    ColorBuffer g_DepthDownsize4;
    ColorBuffer g_DepthTiled1;
    ColorBuffer g_DepthTiled2;
    ColorBuffer g_DepthTiled3;
    ColorBuffer g_DepthTiled4;
    ColorBuffer g_AOMerged1;
    ColorBuffer g_AOMerged2;
    ColorBuffer g_AOMerged3;
    ColorBuffer g_AOMerged4;
    ColorBuffer g_AOSmooth1;
    ColorBuffer g_AOSmooth2;
    ColorBuffer g_AOSmooth3;
    ColorBuffer g_AOHighQuality1;
    ColorBuffer g_AOHighQuality2;
    ColorBuffer g_AOHighQuality3;
    ColorBuffer g_AOHighQuality4;

    ColorBuffer g_DoFTileClass[2];
    ColorBuffer g_DoFPresortBuffer;
    ColorBuffer g_DoFPrefilter;
    ColorBuffer g_DoFBlurColor[2];
    ColorBuffer g_DoFBlurAlpha[2];

    ColorBuffer g_MotionPrepBuffer;
    ColorBuffer g_LumaBuffer;
    ColorBuffer g_TemporalColor[2];
    ColorBuffer g_TemporalMinBound;
    ColorBuffer g_TemporalMaxBound;
    ColorBuffer g_LumaLR;

    ByteAddressBuffer g_GeometryBuffer;

    DXGI_FORMAT DefaultHdrColorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    //DXGI_FORMAT DefaultHdrColorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;//DXGI_FORMAT_R11G11B10_FLOAT;
}

#define T2X_COLOR_FORMAT DXGI_FORMAT_R10G10B10A2_UNORM
#define HDR_MOTION_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define DSV_FORMAT DXGI_FORMAT_D32_FLOAT

void D3D12Engine::InitializeRenderingBuffers( uint32_t bufferWidth, uint32_t bufferHeight )
{
    GraphicsContext& InitContext = GraphicsContext::Begin();

    const uint32_t bufferWidth1 = (bufferWidth + 1) / 2;
    const uint32_t bufferWidth2 = (bufferWidth + 3) / 4;
    const uint32_t bufferWidth3 = (bufferWidth + 7) / 8;
    const uint32_t bufferWidth4 = (bufferWidth + 15) / 16;
    const uint32_t bufferWidth5 = (bufferWidth + 31) / 32;
    const uint32_t bufferWidth6 = (bufferWidth + 63) / 64;
    const uint32_t bufferHeight1 = (bufferHeight + 1) / 2;
    const uint32_t bufferHeight2 = (bufferHeight + 3) / 4;
    const uint32_t bufferHeight3 = (bufferHeight + 7) / 8;
    const uint32_t bufferHeight4 = (bufferHeight + 15) / 16;
    const uint32_t bufferHeight5 = (bufferHeight + 31) / 32;
    const uint32_t bufferHeight6 = (bufferHeight + 63) / 64;

    EsramAllocator esram;

    esram.PushStack();

        g_SceneColorBuffer.Create( L"Main Color Buffer", bufferWidth, bufferHeight, 1, DefaultHdrColorFormat, esram );
        g_SceneNormalBuffer.Create( L"Normals Buffer", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, esram );
        g_VelocityBuffer.Create( L"Motion Vectors", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R32_UINT );

        esram.PushStack();	// Render HDR image

            g_LinearDepth[0].Create( L"Linear Depth 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM );
            g_LinearDepth[1].Create( L"Linear Depth 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16_UNORM );
            g_MinMaxDepth8.Create(L"MinMaxDepth 8x8", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R32_UINT, esram );
            g_MinMaxDepth16.Create(L"MinMaxDepth 16x16", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R32_UINT, esram );
            g_MinMaxDepth32.Create(L"MinMaxDepth 32x32", bufferWidth5, bufferHeight5, 1, DXGI_FORMAT_R32_UINT, esram );

            

            esram.PushStack(); // Begin opaque geometry

                esram.PushStack();    // Begin Shading

                    g_SSAOFullScreen.Create( L"SSAO Full Res", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM );

                    esram.PushStack();    // Begin generating SSAO
                        g_DepthDownsize1.Create( L"Depth Down-Sized 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R32_FLOAT, esram );
                        g_DepthDownsize2.Create( L"Depth Down-Sized 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R32_FLOAT, esram );
                        g_DepthDownsize3.Create( L"Depth Down-Sized 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R32_FLOAT, esram );
                        g_DepthDownsize4.Create( L"Depth Down-Sized 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R32_FLOAT, esram );
                        g_DepthTiled1.CreateArray( L"Depth De-Interleaved 1", bufferWidth3, bufferHeight3, 16, DXGI_FORMAT_R16_FLOAT, esram );
                        g_DepthTiled2.CreateArray( L"Depth De-Interleaved 2", bufferWidth4, bufferHeight4, 16, DXGI_FORMAT_R16_FLOAT, esram );
                        g_DepthTiled3.CreateArray( L"Depth De-Interleaved 3", bufferWidth5, bufferHeight5, 16, DXGI_FORMAT_R16_FLOAT, esram );
                        g_DepthTiled4.CreateArray( L"Depth De-Interleaved 4", bufferWidth6, bufferHeight6, 16, DXGI_FORMAT_R16_FLOAT, esram );
                        g_AOMerged1.Create( L"AO Re-Interleaved 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOMerged2.Create( L"AO Re-Interleaved 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOMerged3.Create( L"AO Re-Interleaved 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOMerged4.Create( L"AO Re-Interleaved 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOSmooth1.Create( L"AO Smoothed 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOSmooth2.Create( L"AO Smoothed 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOSmooth3.Create( L"AO Smoothed 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOHighQuality1.Create( L"AO High Quality 1", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOHighQuality2.Create( L"AO High Quality 2", bufferWidth2, bufferHeight2, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOHighQuality3.Create( L"AO High Quality 3", bufferWidth3, bufferHeight3, 1, DXGI_FORMAT_R8_UNORM, esram );
                        g_AOHighQuality4.Create( L"AO High Quality 4", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R8_UNORM, esram );
                    esram.PopStack();	// End generating SSAO

                esram.PopStack();	// End Shading

                esram.PushStack();	// Begin depth of field
                    g_DoFTileClass[0].Create(L"DoF Tile Classification Buffer 0", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R11G11B10_FLOAT);
                    g_DoFTileClass[1].Create(L"DoF Tile Classification Buffer 1", bufferWidth4, bufferHeight4, 1, DXGI_FORMAT_R11G11B10_FLOAT);

                    g_DoFPresortBuffer.Create(L"DoF Presort Buffer", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram );
                    g_DoFPrefilter.Create(L"DoF PreFilter Buffer", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram );
                    g_DoFBlurColor[0].Create(L"DoF Blur Color", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram );
                    g_DoFBlurColor[1].Create(L"DoF Blur Color", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R11G11B10_FLOAT, esram );
                    g_DoFBlurAlpha[0].Create(L"DoF FG Alpha", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram );
                    g_DoFBlurAlpha[1].Create(L"DoF FG Alpha", bufferWidth1, bufferHeight1, 1, DXGI_FORMAT_R8_UNORM, esram );
                esram.PopStack();	// End depth of field

                g_TemporalColor[0].Create( L"Temporal Color 0", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
                g_TemporalColor[1].Create( L"Temporal Color 1", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
                //TemporalEffects::ClearHistory(InitContext);

                g_TemporalMinBound.Create( L"Temporal Min Color", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R11G11B10_FLOAT);
                g_TemporalMaxBound.Create( L"Temporal Max Color", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R11G11B10_FLOAT);

                esram.PushStack();	// Begin motion blur
                    g_MotionPrepBuffer.Create( L"Motion Blur Prep", bufferWidth1, bufferHeight1, 1, HDR_MOTION_FORMAT, esram );
                esram.PopStack();	// End motion blur

            esram.PopStack();	// End opaque geometry

        esram.PopStack();	// End HDR image

        esram.PushStack();	// Begin post processing

            // This is useful for storing per-pixel weights such as motion strength or pixel luminance
            g_LumaBuffer.Create( L"Luminance", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8_UNORM, esram );

            
        esram.PopStack();	// End post processing

        g_OverlayBuffer.Create( L"UI Overlay", bufferWidth, bufferHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, esram );
        g_HorizontalBuffer.Create( L"Bicubic Intermediate", bufferWidth, bufferHeight, 1, DefaultHdrColorFormat, esram );

    esram.PopStack(); // End final image
    
    InitContext.Finish();
}

void D3D12Engine::DestroyRenderingBuffers()
{
    
    g_SceneColorBuffer.DestroyBuffer();
    g_SceneNormalBuffer.DestroyBuffer();
    g_VelocityBuffer.DestroyBuffer();
    g_OverlayBuffer.DestroyBuffer();
    g_HorizontalBuffer.DestroyBuffer();

    g_SSAOFullScreen.DestroyBuffer();
    g_LinearDepth[0].DestroyBuffer();
    g_LinearDepth[1].DestroyBuffer();
    g_MinMaxDepth8.DestroyBuffer();
    g_MinMaxDepth16.DestroyBuffer();
    g_MinMaxDepth32.DestroyBuffer();
    g_DepthDownsize1.DestroyBuffer();
    g_DepthDownsize2.DestroyBuffer();
    g_DepthDownsize3.DestroyBuffer();
    g_DepthDownsize4.DestroyBuffer();
    g_DepthTiled1.DestroyBuffer();
    g_DepthTiled2.DestroyBuffer();
    g_DepthTiled3.DestroyBuffer();
    g_DepthTiled4.DestroyBuffer();
    g_AOMerged1.DestroyBuffer();
    g_AOMerged2.DestroyBuffer();
    g_AOMerged3.DestroyBuffer();
    g_AOMerged4.DestroyBuffer();
    g_AOSmooth1.DestroyBuffer();
    g_AOSmooth2.DestroyBuffer();
    g_AOSmooth3.DestroyBuffer();
    g_AOHighQuality1.DestroyBuffer();
    g_AOHighQuality2.DestroyBuffer();
    g_AOHighQuality3.DestroyBuffer();
    g_AOHighQuality4.DestroyBuffer();

    g_DoFTileClass[0].DestroyBuffer();
    g_DoFTileClass[1].DestroyBuffer();
    g_DoFPresortBuffer.DestroyBuffer();
    g_DoFPrefilter.DestroyBuffer();
    g_DoFBlurColor[0].DestroyBuffer();
    g_DoFBlurColor[1].DestroyBuffer();
    g_DoFBlurAlpha[0].DestroyBuffer();
    g_DoFBlurAlpha[1].DestroyBuffer();

    g_MotionPrepBuffer.DestroyBuffer();
    g_LumaBuffer.DestroyBuffer();
    g_TemporalColor[0].DestroyBuffer();
    g_TemporalColor[1].DestroyBuffer();
    g_TemporalMinBound.DestroyBuffer();
    g_TemporalMaxBound.DestroyBuffer();
    g_LumaLR.DestroyBuffer();
}
