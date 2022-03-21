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

#pragma once

#include "Matrix3.h"
//#include "BoundingSphere.h"

namespace Math
{
    // Orthonormal basis (just rotation via quaternion) and translation
    class OrthogonalTransform;

    // A 3x4 matrix that allows for asymmetric skew and scale
    class AffineTransform;

    // Uniform scale and translation that can be compactly represented in a vec4
    class ScaleAndTranslation;

    // Uniform scale, rotation (quaternion), and translation that fits in two vec4s
    class UniformTransform;


}
