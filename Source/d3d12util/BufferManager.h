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

#pragma once

#include "ColorBuffer.h"
#include "DepthBuffer.h"
//#include "ShadowBuffer.h"
#include "GpuBuffer.h"
//#include "DX12Helper.h"

namespace D3D12Engine
{
    extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
    extern ColorBuffer g_SceneNormalBuffer; // R16G16B16A16_FLOAT
    extern ColorBuffer g_PostEffectsBuffer; // R32_UINT (to support Read-Modify-Write with a UAV)
    extern ColorBuffer g_OverlayBuffer;     // R8G8B8A8_UNORM
    extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling
    void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight );

    void DestroyRenderingBuffers();

} // namespace D3D12Engine
