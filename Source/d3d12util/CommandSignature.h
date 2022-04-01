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

class RootSignature;

class IndirectParameter
{
    friend class CommandSignature;
public:

    IndirectParameter() 
    {
        m_IndirectParam.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
    }

    void Draw(void)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
    }

    void DrawIndexed(void)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    }

    void Dispatch(void)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    }

    void VertexBufferView(UINT Slot)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
        m_IndirectParam.VertexBuffer.Slot = Slot;
    }

    void IndexBufferView(void)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
    }

    void Constant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        m_IndirectParam.Constant.RootParameterIndex = RootParameterIndex;
        m_IndirectParam.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
        m_IndirectParam.Constant.Num32BitValuesToSet = Num32BitValuesToSet;
    }

    void ConstantBufferView(UINT RootParameterIndex)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
        m_IndirectParam.ConstantBufferView.RootParameterIndex = RootParameterIndex;
    }

    void ShaderResourceView(UINT RootParameterIndex)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
        m_IndirectParam.ShaderResourceView.RootParameterIndex = RootParameterIndex;
    }

    void UnorderedAccessView(UINT RootParameterIndex)
    {
        m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
        m_IndirectParam.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
    }

    const D3D12_INDIRECT_ARGUMENT_DESC& GetDesc( void ) const { return m_IndirectParam; }

protected:

    D3D12_INDIRECT_ARGUMENT_DESC m_IndirectParam;
};

class CommandSignature
{
public:

    CommandSignature( UINT NumParams = 0 ) : m_Finalized(FALSE), m_NumParameters(NumParams)
    {
        Reset(NumParams);
    }

    void Destroy( void )
    {
        m_Signature = nullptr;
        m_ParamArray = nullptr;
    }

    void Reset( UINT NumParams )
    {
        if (NumParams > 0)
            m_ParamArray.reset(new IndirectParameter[NumParams]);
        else
            m_ParamArray = nullptr;

        m_NumParameters = NumParams;
    }

    IndirectParameter& operator[] ( size_t EntryIndex )
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    const IndirectParameter& operator[] ( size_t EntryIndex ) const
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    void Finalize( const RootSignature* RootSignature = nullptr );

    ID3D12CommandSignature* GetSignature() const { return m_Signature; }

protected:

    BOOL m_Finalized;
    UINT m_NumParameters;
    std::unique_ptr<IndirectParameter[]> m_ParamArray;
    CComPtr<ID3D12CommandSignature> m_Signature;
};
