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

class DescriptorCache;

class RootParameter
{
    friend class RootSignature;
public:

    RootParameter() 
    {
        m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }

    ~RootParameter()
    {
        Clear();
    }

    void Clear()
    {
        if (m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            delete [] m_RootParam.DescriptorTable.pDescriptorRanges;

        m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }

    void InitAsConstants( UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Constants.Num32BitValues = NumDwords;
        m_RootParam.Constants.ShaderRegister = Register;
        m_RootParam.Constants.RegisterSpace = Space;
    }

    void InitAsConstantBuffer( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsBufferSRV( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsBufferUAV( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsDescriptorRange( D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        InitAsDescriptorTable(1, Visibility);
        SetTableRange(0, Type, Register, Count, Space);
    }

    void InitAsDescriptorTable( UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
        m_RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
    }

    void SetTableRange( UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0 )
    {
        D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
        range->RangeType = Type;
        range->NumDescriptors = Count;
        range->BaseShaderRegister = Register;
        range->RegisterSpace = Space;
        range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    const D3D12_ROOT_PARAMETER& operator() ( void ) const { return m_RootParam; }
        

protected:

    D3D12_ROOT_PARAMETER m_RootParam;
};

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)
class RootSignature
{
    friend class DynamicDescriptorHeap;

public:

    RootSignature( UINT NumRootParams = 0, UINT NumStaticSamplers = 0 ) : m_Finalized(FALSE), m_NumParameters(NumRootParams)
    {
        Reset(NumRootParams, NumStaticSamplers);
    }

    ~RootSignature() { }

    void Free()
    {
      m_NumParameters = 0;
      m_Finalized = FALSE;
      m_NumParameters = 0;
      m_NumSamplers = 0;
      m_NumInitializedStaticSamplers = 0;
      m_DescriptorTableBitMap = 0;
      m_SamplerTableBitMap = 0;
      int i = 0;
      for (i = 0; i < 16; i++) 
        m_DescriptorTableSize[i] = 0;
      m_ParamArray.release();
      m_SamplerArray.release();
      m_Signature->Release();
      m_Signature = nullptr;
      //m_ParamArray.std::unique_ptr<RootParameter[]> m_ParamArray;
      //std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_SamplerArray;
      //ID3D12RootSignature* m_Signature;
    }

    static void DestroyAll(void);

    void Reset( UINT NumRootParams, UINT NumStaticSamplers = 0 )
    {
        if (NumRootParams > 0)
            m_ParamArray.reset(new RootParameter[NumRootParams]);
        else
            m_ParamArray = nullptr;
        m_NumParameters = NumRootParams;

        if (NumStaticSamplers > 0)
            m_SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
        else
            m_SamplerArray = nullptr;
        m_NumSamplers = NumStaticSamplers;
        m_NumInitializedStaticSamplers = 0;
    }

    RootParameter& operator[] ( size_t EntryIndex )
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    const RootParameter& operator[] ( size_t EntryIndex ) const
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    void InitStaticSampler( UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
        D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL );

    void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ID3D12RootSignature* GetSignature() const { return m_Signature; }

protected:

    BOOL m_Finalized;
    UINT m_NumParameters;
    UINT m_NumSamplers;
    UINT m_NumInitializedStaticSamplers;
    uint32_t m_DescriptorTableBitMap;		// One bit is set for root parameters that are non-sampler descriptor tables
    uint32_t m_SamplerTableBitMap;			// One bit is set for root parameters that are sampler descriptor tables
    uint32_t m_DescriptorTableSize[16];		// Non-sampler descriptor tables need to know their descriptor count
    std::unique_ptr<RootParameter[]> m_ParamArray;
    std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_SamplerArray;
    ID3D12RootSignature* m_Signature;
};
