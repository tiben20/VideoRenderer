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

#include "stdafx.h"
#include "GpuResource.h"

class Texture : public GpuResource
{
    friend class CommandContext;

public:

    Texture() { m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
    Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_hCpuDescriptorHandle(Handle) {}

    // Create a 1-level textures
    void Create2D(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData );
    void CreateCube(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData);

    void CreateTGAFromMemory( const void* memBuffer, size_t fileSize, bool sRGB );
    bool CreateDDSFromMemory( const void* memBuffer, size_t fileSize, bool sRGB );
    void CreatePIXImageFromMemory( const void* memBuffer, size_t fileSize );

    virtual void Destroy() override
    {
        GpuResource::Destroy();
        m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_hCpuDescriptorHandle; }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetDepth() const { return m_Depth; }

protected:

    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_Depth;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};
