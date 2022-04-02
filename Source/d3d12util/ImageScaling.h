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

class GraphicsContext;
class ColorBuffer;
enum DXGI_FORMAT;

__declspec(align(16)) struct CONSTANT_BUFFER_VAR {
  DirectX::XMFLOAT4 cm_r;
  DirectX::XMFLOAT4 cm_g;
  DirectX::XMFLOAT4 cm_b;
  DirectX::XMFLOAT4 cm_c;
};

namespace ImageScaling
{

  void Initialize(DXGI_FORMAT DestFormat);
  void FreeImageScaling();
  void ColorAjust(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source0, ColorBuffer& source1, CONSTANT_BUFFER_VAR& colorconstant);
  
  void SetPipelineBilinear(GraphicsContext& Context);
  enum eScalingFilter { kBilinear, kSharpening, kBicubic, kLanczos, kFilterCount };
  void PreparePresentSDR(GraphicsContext& Context, ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect);
  void PreparePresentHDR(GraphicsContext& Context, ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect);
  void Downscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, eScalingFilter tech = kLanczos, CRect destRect = CRect());
  void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, eScalingFilter tech = kLanczos, CRect destRect = CRect());

}