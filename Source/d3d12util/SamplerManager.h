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


#include "Color.h"

class SamplerDesc : public D3D12_SAMPLER_DESC
{
public:
    // These defaults match the default values for HLSL-defined root
    // signature static samplers.  So not overriding them here means
    // you can safely not define them in HLSL.
    SamplerDesc()
    {
        Filter = D3D12_FILTER_ANISOTROPIC;
        AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        MipLODBias = 0.0f;
        MaxAnisotropy = 16;
        ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        BorderColor[0] = 1.0f;
        BorderColor[1] = 1.0f;
        BorderColor[2] = 1.0f;
        BorderColor[3] = 1.0f;
        MinLOD = 0.0f;
        MaxLOD = D3D12_FLOAT32_MAX;
    }

    void SetTextureAddressMode( D3D12_TEXTURE_ADDRESS_MODE AddressMode )
    {
        AddressU = AddressMode;
        AddressV = AddressMode;
        AddressW = AddressMode;
    }

    void SetBorderColor( Color Border )
    {
        BorderColor[0] = Border.R();
        BorderColor[1] = Border.G();
        BorderColor[2] = Border.B();
        BorderColor[3] = Border.A();
    }

    // Allocate new descriptor as needed; return handle to existing descriptor when possible
    D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor( void );

    // Create descriptor in place (no deduplication).  Handle must be preallocated
    void CreateDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE Handle );
};
