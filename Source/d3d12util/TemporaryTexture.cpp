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

#include "stdafx.h"
#include "TemporaryTexture.h"
#include "CommandContext.h"
#include <map>
#include <thread>

using namespace std;
using namespace D3D12Public;

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------

void TemporaryTexture::SetTexture(ID3D12Resource* pResource,int index)
{
  m_ArrayIndex = index;
  m_pResource = pResource;
  m_hCpuDescriptorHandle[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  m_hCpuDescriptorHandle[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  m_CurrentShaderHeap.m_descriptorSize = 0;
  if (m_CurrentShaderHeap.heap)
    m_CurrentShaderHeap.heap->Release();
  pResource->AddRef();
  m_UsageState - D3D12_RESOURCE_STATE_COPY_DEST;
  D3D12_RESOURCE_DESC desc = {};
  
  
  desc = pResource->GetDesc();
  m_Width = desc.Width;
  m_Height = desc.Height;
  m_Format = desc.Format;
  
  
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12_HeapCPUHandle(d3d12_heap_t* heap, size_t sub_resource)
{
  union u
  {
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    uintptr_t p;
  } t;
  t.h = heap->heap->GetCPUDescriptorHandleForHeapStart();
  t.p += sub_resource * heap->m_descriptorSize;
  return t.h;
}

D3D12_CPU_DESCRIPTOR_HANDLE TemporaryTexture::GetContantBufferViewHeap(int subresource)
{
  return D3D12_HeapCPUHandle(&m_CurrentShaderHeap, subresource);

}

void TemporaryTexture::InitHeap()
{
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = 2 + DXGI_MAX_SHADER_VIEW * 64;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  D3D12Public::g_Device->CreateDescriptorHeap(&heapDesc, IID_ID3D12DescriptorHeap, (void**)&m_CurrentShaderHeap.heap);
  m_CurrentShaderHeap.m_descriptorSize = D3D12Public::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  m_CurrentShaderHeap.heap->SetName(L"Temporary texture heap");
  m_CurrentShaderHeap.heap->AddRef();
}

//cast before rendering
void TemporaryTexture::CreateShaderRessource()
{
  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
  //todo fix plane
  

  if (m_Format == DXGI_FORMAT_NV12)
  {

    //DXGI_FORMAT_R8_UNORM
    //DXGI_FORMAT_R8G8_UNORM
    
    std::map<DXGI_FORMAT, int> nv12{ {DXGI_FORMAT_R8G8_UNORM,1} ,{ DXGI_FORMAT_R8_UNORM ,0 }};
    m_SRVHandle = D3D12Public::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (std::map<DXGI_FORMAT, int>::const_iterator it = nv12.begin(); it != nv12.end(); it++)
    { 
      desc.Format = it->first;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipLevels = 1;
      desc.Texture2D.MostDetailedMip = 0;//todo
      desc.Texture2D.ResourceMinLODClamp = 0xf;
      //desc.Texture2DArray.ArraySize = 1;//picsys->array_size;
      //if the texture is opaque the plane slice is 0
      //on yuv format like nv12 the resource plane will be 1 if the format is DXGI_FORMAT_R8G8_UNORM
      desc.Texture2D.PlaneSlice = it->second;
      //m_hCpuDescriptorHandle[it->second] = D3D12_HeapCPUHandle(&m_CurrentShaderHeap, srv_handle);
      
      D3D12Public::g_Device->CreateShaderResourceView(m_pResource, &desc, m_SRVHandle);
      
      //m_SrvHandle += 1 * m_CurrentShaderHeap.m_descriptorSize;//
      //srv_handle++;
    }
  
  }
  
  
  
}

void TemporaryTexture::ReleaseTexture()
{
  m_pResource.Release();
  
  m_CurrentShaderHeap.m_descriptorSize = 0;
  m_hCpuDescriptorHandle[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  m_hCpuDescriptorHandle[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

