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

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>

class CommandAllocatorPool
{
public:
    CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
    ~CommandAllocatorPool();

    void Create(ID3D12Device* pDevice);
    void Shutdown();

    ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
    void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

    inline size_t Size() { return m_AllocatorPool.size(); }

private:
    const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

    ID3D12Device* m_Device;
    std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
    std::mutex m_AllocatorMutex;
};
