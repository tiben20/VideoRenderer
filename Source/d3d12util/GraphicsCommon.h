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

#include "SamplerManager.h"

class SamplerDesc;
class CommandSignature;
class RootSignature;
class ComputePSO;
class GraphicsPSO;

namespace D3D12Engine
{
    void InitializeCommonState(void);
    void DestroyCommonState(void);

    extern SamplerDesc SamplerLinearWrapDesc;
    extern SamplerDesc SamplerAnisoWrapDesc;
    extern SamplerDesc SamplerShadowDesc;
    extern SamplerDesc SamplerLinearClampDesc;
    extern SamplerDesc SamplerLinearClampLODDesc;
    extern SamplerDesc SamplerVolumeWrapDesc;
    extern SamplerDesc SamplerPointClampDesc;
    extern SamplerDesc SamplerPointClampLODDesc;
    extern SamplerDesc SamplerPointBorderDesc;
    extern SamplerDesc SamplerLinearBorderDesc;
    extern SamplerDesc SamplerGeometryDesc;
    extern SamplerDesc SamplerfxrcnnxDesc;
    

    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointLODClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearLODClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerGeometry;
    extern D3D12_CPU_DESCRIPTOR_HANDLE Samplerfxrcnnx;


    extern D3D12_RASTERIZER_DESC RasterizerDefault;
    extern D3D12_RASTERIZER_DESC RasterizerTwoSided;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultCw;

    extern D3D12_BLEND_DESC BlendNoColorWrite;		// XXX
    extern D3D12_BLEND_DESC BlendDisable;			// 1, 0
    extern D3D12_BLEND_DESC BlendPreMultiplied;		// 1, 1-SrcA
    extern D3D12_BLEND_DESC BlendTraditional;		// SrcA, 1-SrcA
    extern D3D12_BLEND_DESC BlendAdditive;			// 1, 1
    extern D3D12_BLEND_DESC BlendTraditionalAdditive;// SrcA, 1
    extern D3D12_BLEND_DESC BlendGeometry;
    extern D3D12_BLEND_DESC BlendFont;
    extern D3D12_BLEND_DESC BlendSubPic;
    extern D3D12_BLEND_DESC Blendfxrcnnx;
    
    extern D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

    extern CommandSignature DispatchIndirectCommandSignature;
    extern CommandSignature DrawIndirectCommandSignature;

    enum eDefaultTexture
    {
        kMagenta2D,  // Useful for indicating missing textures
        kBlackOpaque2D,
        kBlackTransparent2D,
        kWhiteOpaque2D,
        kWhiteTransparent2D,
        kDefaultNormalMap,
        kBlackCubeMap,

        kNumDefaultTextures
    };
    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture( eDefaultTexture texID );

    extern RootSignature g_CommonRS;
    extern ComputePSO g_GenerateMipsLinearPSO[4];
    extern ComputePSO g_GenerateMipsGammaPSO[4];
    extern GraphicsPSO g_DownsampleDepthPSO;
}
