#pragma once

namespace ShapeMeshes {

// Wedge shape vertices - triangular prism (ramp shape)
// Slopes from bottom front to top back
static const float WEDGE_VERTICES[] = {
    // Bottom face (quad as 2 triangles) - counter-clockwise from below (outward facing)
    -0.5f, -0.5f, -0.5f,  // back left
     0.5f, -0.5f, -0.5f,  // back right
     0.5f, -0.5f,  0.5f,  // front right
    
    -0.5f, -0.5f, -0.5f,  // back left
     0.5f, -0.5f,  0.5f,  // front right
    -0.5f, -0.5f,  0.5f,  // front left
    
    // Left side face (triangle) - counter-clockwise from outside (outward facing)
    -0.5f, -0.5f,  0.5f,  // bottom front
    -0.5f,  0.5f, -0.5f,  // top back
    -0.5f, -0.5f, -0.5f,  // bottom back
    
    // Right side face (triangle) - counter-clockwise from outside (outward facing)
     0.5f, -0.5f, -0.5f,  // bottom back
     0.5f,  0.5f, -0.5f,  // top back
     0.5f, -0.5f,  0.5f,  // bottom front
    
    // Back face (quad as 2 triangles) - counter-clockwise from outside (outward facing)
    -0.5f, -0.5f, -0.5f,  // bottom left
    -0.5f,  0.5f, -0.5f,  // top left
     0.5f, -0.5f, -0.5f,  // bottom right
    
     0.5f, -0.5f, -0.5f,  // bottom right
    -0.5f,  0.5f, -0.5f,  // top left
     0.5f,  0.5f, -0.5f,  // top right
    
    // Sloped ramp face (quad as 2 triangles) - clockwise from outside (front face visible)
    -0.5f, -0.5f,  0.5f,  // bottom front left
     0.5f,  0.5f, -0.5f,  // top back right
    -0.5f,  0.5f, -0.5f,  // top back left
    
    -0.5f, -0.5f,  0.5f,  // bottom front left
     0.5f, -0.5f,  0.5f,  // bottom front right
     0.5f,  0.5f, -0.5f   // top back right
}; // andddddddd yeah i think i got it, now time to check my heartrate..

static const int WEDGE_VERTEX_COUNT = 24;
static const int WEDGE_TRIANGLE_COUNT = 8;

// Corner wedge shape vertices - tetrahedron-like shape
static const float CORNER_WEDGE_VERTICES[] = {
    // Base triangle - counter-clockwise from below
    -0.5f, -0.5f, -0.5f,  // 0
    -0.5f, -0.5f,  0.5f,  // 2
     0.5f, -0.5f, -0.5f,  // 1
    
    // Side face 1 - counter-clockwise from outside
    -0.5f, -0.5f, -0.5f,  // 0
    -0.5f, -0.5f,  0.5f,  // 2
    -0.5f,  0.5f, -0.5f,  // 3
    
    // Side face 2 - counter-clockwise from outside
     0.5f, -0.5f, -0.5f,  // 1
    -0.5f,  0.5f, -0.5f,  // 3
    -0.5f, -0.5f,  0.5f,  // 2
    
    // Back face - counter-clockwise from outside
    -0.5f, -0.5f, -0.5f,  // 0
    -0.5f,  0.5f, -0.5f,  // 3
     0.5f, -0.5f, -0.5f   // 1
}; // ok i'm jacked up, im so done with this one, but yeah handle it, i'm lazy now

static const int CORNER_WEDGE_VERTEX_COUNT = 12;
static const int CORNER_WEDGE_TRIANGLE_COUNT = 4;

}
