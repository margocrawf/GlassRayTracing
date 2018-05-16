import numpy as np

# array of control points of the form y, R**2
points = [(0,4),
          (2,9),
          (4,4),
          (6,1),
          (8,1)]
S_0 = 2.5 # dR**2/dy for the first point

derivs = np.zeros(len(points))
derivs[0] = S_0

def coefficients(Ri2, Rj2, Si, yi, yj):

    a = (Rj2 - Ri2 - Si*(yj + yi)) / (yj+yi)**2
    #a = (Rj2 - Ri2 - Si*(yi + yj)) / (2*yi*yj - yi**2 - yj**2 )

    b = Si - 2*a*yj

    c = Ri2 - a*yi**2 - b*yi

    return (a,b,c)

def get_Sj(Si, a, yi, yj):
    return Si + (2*a*(yj-yi))

for i in range(len(points)-1):
    yi, Ri2 = points[i]
    yj, Rj2 = points[i+1]
    (a,b,c) = coefficients(Ri2, Rj2, derivs[i], yi, yj)
    derivs[i+1] = get_Sj(derivs[i], a, yi, yj)

    #print(a,b,c)
    mat = "mat4x4 section_%d = mat4x4(-1, 0, 0, 0, \n 0, %f, 0, %f, \n 0, 0, -1, 0, \n 0, 0, 0, %f); \n" % (i, a, b, c)
    print(mat)
    size = float(yj - yi) / 2.0
    center = yi + size
    clipTransform = "mat4x4 clip_%d = mat4x4::scaling(vec3(1, %f,1)) * \n mat4x4::translation(vec3(0, %f, 0));\n" % (i, size, center)
    #print(clipTransform)

