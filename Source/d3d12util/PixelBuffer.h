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

#include "GpuResource.h"

class EsramAllocator;

class PixelBuffer : public GpuResource
{
public:
    PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) {}

    uint32_t GetWidth(void) const { return m_Width; }
    uint32_t GetHeight(void) const { return m_Height; }
    uint32_t GetDepth(void) const { return m_ArraySize; }
    const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

    // Has no effect on Desktop
    void SetBankRotation( uint32_t RotationAmount )
    {
        (RotationAmount);
    }

    // Write the raw pixel buffer contents to a file
    // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
    void ExportToFile( const std::wstring& FilePath );

protected:

    D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

    void AssociateWithResource( ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState );

    void CreateTextureResource( ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
        D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );

    void CreateTextureResource( ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
        D3D12_CLEAR_VALUE ClearValue, EsramAllocator& Allocator );

    static DXGI_FORMAT GetBaseFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT GetUAVFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT GetDSVFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT GetDepthFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT GetStencilFormat( DXGI_FORMAT Format );
    static size_t BytesPerPixel( DXGI_FORMAT Format );

    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_ArraySize;
    DXGI_FORMAT m_Format;
    uint32_t m_BankRotation;
};
