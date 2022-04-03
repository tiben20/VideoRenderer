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
#include <map>

struct VideoCapabilities
{
  //DXGI_FORMAT Format;
  bool        Texture2D;
  bool        Input;
  bool        Output;
  bool        Encoder;
};



namespace D3D12Engine
{
  static std::map<DXGI_FORMAT, VideoCapabilities> g_D3D12Capabilities;
  void FillD3D12Capabilities();
};