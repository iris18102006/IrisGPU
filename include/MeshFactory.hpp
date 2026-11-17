#pragma once

#include "Mesh.hpp"

Mesh createCubeMesh();
Mesh createSphereMesh(int stacks = 24, int slices = 48);
Mesh createPlaneMesh(float halfSize = 10.0f, float uvTiles = 10.0f);
Mesh createTorusMesh(float majorRadius = 1.0f, float minorRadius = 0.35f,
                     int segments = 48, int sides = 24);
