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
// Author(s):  James Stanard
//

#pragma once

#include "stdafx.h"
#include "GpuResource.h"

#define DXGI_MAX_SHADER_VIEW     4

typedef struct {
  ID3D12DescriptorHeap* heap;
  UINT                        m_descriptorSize;
} d3d12_heap_t;

class TemporaryTexture : public GpuResource
{
    //friend class CommandContext;
public:
  TemporaryTexture() {
    m_CurrentShaderHeap.heap = nullptr;
    m_ArrayIndex = -1;
  };

  int GetIndex() { return m_ArrayIndex; };
  void SetTexture(ID3D12Resource* pResource, int index);
  void ReleaseTexture();
  virtual ~TemporaryTexture() { Destroy(); }
    virtual void Destroy() override
    {
      
        //GpuResource::Destroy();
        //m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    void CreateShaderRessource();
    void InitHeap();
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t sub_resource)
    {
      union u
      {
        D3D12_GPU_DESCRIPTOR_HANDLE h;
        uintptr_t p;
      } t;
      t.h = m_CurrentShaderHeap.heap->GetGPUDescriptorHandleForHeapStart();
      t.p += sub_resource * m_CurrentShaderHeap.m_descriptorSize;
      return t.h;
    }
    ID3D12DescriptorHeap* GetHeapPointer() { return m_CurrentShaderHeap.heap; };
    
      D3D12_CPU_DESCRIPTOR_HANDLE GetContantBufferViewHeap(int subresource);

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV1() const { return m_hCpuDescriptorHandle[1]; }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetDepth() const { return m_Depth; }

protected:
    d3d12_heap_t m_CurrentShaderHeap;
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_Depth;
    int m_ArrayIndex;
    DXGI_FORMAT m_Format;
    size_t m_SrvHandle = 2;//its where the texture plane start

    uintptr_t m_CurrentHeapLocation;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle[2];
};
