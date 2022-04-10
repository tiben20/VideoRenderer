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
#include "IVideoRenderer.h"
class GraphicsContext;
class ColorBuffer;
class TypedBuffer;
class Texture;
enum DXGI_FORMAT;

__declspec(align(16)) struct CONSTANT_BUFFER_VAR {
  DirectX::XMFLOAT4 cm_r;
  DirectX::XMFLOAT4 cm_g;
  DirectX::XMFLOAT4 cm_b;
  DirectX::XMFLOAT4 cm_c;
};

__declspec(align(16)) struct CONSTANT_DOWNSCALE_BUFFER {
  DirectX::XMFLOAT2 wh;
  DirectX::XMFLOAT2 dxdy;
  DirectX::XMFLOAT2 scale;
  float support;
  float ss;
  int filter;
  int axis;
};

__declspec(align(16)) struct CONSTANT_UPSCALE_BUFFER {
  DirectX::XMFLOAT2 wh;
  DirectX::XMFLOAT2 dxdy;
  DirectX::XMFLOAT2 scale;
  int filter;
  int axis;
};

struct VERTEX_SUBPIC {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT2 TexCoord;
};

namespace ImageScaling
{

  void Initialize(DXGI_FORMAT DestFormat);
  void FreeImageScaling();
  
  void CopyPlanesSW(GraphicsContext& Context, ColorBuffer& dest, Texture& plane0, Texture& plane1, CONSTANT_BUFFER_VAR& colorconstant, CRect destRect);
  void ColorAjust(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source0, ColorBuffer& source1, CONSTANT_BUFFER_VAR& colorconstant, CRect destRect = CRect());
  HRESULT RenderToBackBuffer(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source);

  HRESULT RenderAlphaBitmap(GraphicsContext& Context, Texture& resource, RECT alpharect);
  HRESULT RenderSubPic(GraphicsContext& Context, ColorBuffer& resource, ColorBuffer& target, CRect srcRect, UINT srcW, UINT srcH);

  void Downscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect,CRect destRect);
  void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect);
  void UpscaleXbr(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect);
  void Upscalefxrcnnx(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect);

}