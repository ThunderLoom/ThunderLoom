import bpy
import mathutils

for obj in bpy.data.objects:
    obj.select = obj.layers[1]
bpy.ops.object.delete()

bpy.ops.render.render()

with open('/tmp/coordinate_out.txt') as f:

    def read_vector(f):
        line = f.readline()
        s = line.split()
        vec = []
        for entry in s:
            vec.append(float(entry))
        print(vec)
        return mathutils.Vector((vec[0],-vec[2],vec[1]))
    
    l = f.readline()
    if int(l) == 1:
        print(' ------ WARP ABOVE!!!! ----------')
        print('')
    else:
        print(' ------ WARP BELOW!!!! ----------')
        print('')

    p = read_vector(f)
    e1 = read_vector(f)
    e2 = read_vector(f)
    e3 = read_vector(f)


    def draw_vector(name):
        v = read_vector(f)
        bpy.ops.object.empty_add(type='SINGLE_ARROW')
        empty = bpy.context.active_object
        mat = mathutils.Matrix()
        #This is ugly...
        v.normalize()
        a = v.cross(mathutils.Vector((1.0,0.0,0.0)))
        a.normalize()
        b = v.cross(a)
        b.normalize()
        mat[0][0], mat[1][0], mat[2][0] =  a[0],  a[1],  a[2]
        mat[0][1], mat[1][1], mat[2][1] =  b[0],  b[1],  b[2]
        mat[0][2], mat[1][2], mat[2][2] =  v[0],  v[1],  v[2]
        mat[0][3], mat[1][3], mat[2][3] =  p[0],  p[1],  p[2]
        empty.name = name
        empty.matrix_world = mat
        empty.layers[1] = True


    bpy.ops.object.empty_add(type='ARROWS')
    empty = bpy.context.active_object
    mat = mathutils.Matrix()
    mat[0][0], mat[1][0], mat[2][0] = e1[0], e1[1], e1[2]
    mat[0][1], mat[1][1], mat[2][1] = e2[0], e2[1], e2[2]
    mat[0][2], mat[1][2], mat[2][2] = e3[0], e3[1], e3[2]
    mat[0][3], mat[1][3], mat[2][3] =  p[0],  p[1],  p[2]
    empty.name = 'shading space'
    empty.matrix_world = mat
    empty.layers[1] = True

    draw_vector('wi')
    draw_vector('wo')
    draw_vector('H')
