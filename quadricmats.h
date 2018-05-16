#include "mat4x4.h"

/*
 * These are the matrices for the implicit equations
 * of various quadrics, to be used by the quadric class in 
 * ray casting program
 */

mat4x4 ellipseQ = mat4x4(1, 0, 0, 0,
                        0, 2, 0, 0,
                        0, 0, 0.5, 0,
                        0, 0, 0, -1);

mat4x4 sphereQ = mat4x4(1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, -1);

mat4x4 cylinderQ = mat4x4(1, 0, 0, 0,
                        0, 0, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, -1);

mat4x4 coneQ = mat4x4(1, 0, 0, 0,
                        0, -1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 0);

mat4x4 paraboloidQ = mat4x4(1, 0, 0, 0,
                        0, 0, 0, -1,
                        0, 0, 1, 0,
                        0, 0, 0, 0);

mat4x4 hyperboloidQ = mat4x4(1, 0, 0, 0,
                        0, -1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, -1);

mat4x4 hypParaboloidQ = mat4x4(1, 0, 0, 0,
                              0, -1, 0, 0,
                              0, 0, -1, 0,
                              0, 0, 0, 0);

mat4x4 hypCylinderQ = mat4x4(-1, 0, 0, 0,
                              0, 0, 0, 0,
                              0, 0, -1, 0,
                              0, 0, 0, 1);

mat4x4 parallelPlanesQ = mat4x4(0, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, -1);
