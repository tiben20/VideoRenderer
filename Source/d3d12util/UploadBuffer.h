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

// Description:  An upload buffer is visible to both the CPU and the GPU, but
// because the memory is write combined, you should avoid reading data with the CPU.
// An upload buffer is intended for moving data to a default GPU buffer.  You can
// read from a file directly into an upload buffer, rather than reading into regular
// cached memory, copying that to an upload buffer, and copying that to the GPU.

#pragma once

#include "GpuResource.h"

class UploadBuffer : public GpuResource
{
public:
    virtual ~UploadBuffer() { Destroy(); }

    void Create( const std::wstring& name, size_t BufferSize );

    void* Map(void);
    void Unmap(size_t begin = 0, size_t end = -1);

    size_t GetBufferSize() const { return m_BufferSize; }

protected:

    size_t m_BufferSize;
};
