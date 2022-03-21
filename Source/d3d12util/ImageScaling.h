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
  void ColorAjust(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source0, ColorBuffer& source1, CONSTANT_BUFFER_VAR& colorconstant);
  void SetPipelineBilinear(GraphicsContext& Context);
  enum eScalingFilter { kBilinear, kSharpening, kBicubic, kLanczos, kFilterCount };
  void PreparePresentSDR(GraphicsContext& Context, ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect);
  void PreparePresentHDR(GraphicsContext& Context, ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect);
  void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, eScalingFilter tech = kLanczos, CRect destRect = CRect());

}