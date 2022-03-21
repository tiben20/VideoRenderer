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
// Author:  James Stanard 
//

#include "stdafx.h"
#include "CommandSignature.h"
#include "RootSignature.h"
#include "DX12Helper.h"

using namespace D3D12Engine;

void CommandSignature::Finalize( const RootSignature* RootSignature )
{
    if (m_Finalized)
        return;

    UINT ByteStride = 0;
    bool RequiresRootSignature = false;

    for (UINT i = 0; i < m_NumParameters; ++i)
    {
        switch (m_ParamArray[i].GetDesc().Type)
        {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
                ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
                ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
                ByteStride += m_ParamArray[i].GetDesc().Constant.Num32BitValuesToSet * 4;
                RequiresRootSignature = true;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
                ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
                ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
                ByteStride += 8;
                RequiresRootSignature = true;
                break;
        }
    }

    D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc;
    CommandSignatureDesc.ByteStride = ByteStride;
    CommandSignatureDesc.NumArgumentDescs = m_NumParameters;
    CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)m_ParamArray.get();
    CommandSignatureDesc.NodeMask = 1;

    CComPtr<ID3DBlob> pOutBlob, pErrorBlob;

    ID3D12RootSignature* pRootSig = RootSignature ? RootSignature->GetSignature() : nullptr;
    if (RequiresRootSignature)
    {
        ASSERT(pRootSig != nullptr);
    }
    else
    {
        pRootSig = nullptr;
    }

    EXECUTE_ASSERT(S_OK ==  g_Device->CreateCommandSignature(&CommandSignatureDesc, pRootSig,
        MY_IID_PPV_ARGS(&m_Signature)) );

    m_Signature->SetName(L"CommandSignature");

    m_Finalized = TRUE;
}
