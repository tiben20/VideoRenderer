/*
 * Original developed by Minigraph author James Stanard
 *
 * (C) 2022 Ti-BEN
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    extern ColorBuffer g_OverlayBuffer;     // R8G8B8A8_UNORM
    extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling
    void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight );

    void DestroyRenderingBuffers();

} // namespace D3D12Engine
