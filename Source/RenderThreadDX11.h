/*
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

#include "RenderThread.h"
#define TEX11GET(x)std::get<CComPtr<ID3D11Texture2D>>(x)

class CVideoBufferManager11 : public CVideoBufferManager
{
  friend class CRenderThread11;
  CVideoBufferManager11(ID3D11Device* pDevice) { m_pDevice = pDevice; }
  ~CVideoBufferManager11() { m_pDevice.Release(); }
  HRESULT Alloc(int size,D3D11_TEXTURE2D_DESC texdesc) override;
protected:
  CComPtr<ID3D11Device> m_pDevice;
};

class CDX11VideoProcessor;

class CRenderThread11 :
  public CRenderThread
{
public:
  CRenderThread11(CDX11VideoProcessor* pVideoProcessor);
  ~CRenderThread11()override;
  HRESULT CopySample(IMediaSample* pSample);
  HRESULT ProcessShader(CVideoBuffer& inBuf, CVideoBuffer& outBuf) override;
  HRESULT ProcessSubs(CVideoBuffer& inBuf, CVideoBuffer& outBuf) override;
  HRESULT ProcessPresents(CVideoBuffer& inBuf) override;

  HRESULT Init(int inputBufferCount, int processBufferCount, int presentBufferCount, FmtConvParams_t param) override;
  void Resize(int size) override;
  void ReleaseBuffers() override;
  void Flush()override;
protected:
  CDX11VideoProcessor* m_pVideoProcessor;
  Tex11Video_t   m_pVideoTexture;
};
