Name: Xiaoting Bi

Features:

1. Draw splines using recursive subdivision instead of using brute force.

2. Texture map the sky and ground with different images. Use a box to represent the scene. 

3. The ability to ride the coaster.

4. Render rail cross-section of cylinder shape. Call gluCylinder(...) function.

5. Add lighting: add a red ambient lighting to the scene. Press "l" or "L" key to    switch to environment without lighting.

6. Determine coaster normals: Use Sloan method. 
   Initialization:
   N0 = unit(T0 x V) and B0 = unit(T0 x N0), and V is an arbitrary vector
   Then:
   N1 = unit(B0 x T1) and B1 = unit(T1 x N1)

Extra credits:

1. Draw splines using recursive subdivision instead of using brute force.

2. Add lighting: add a red ambient lighting to the scene. 