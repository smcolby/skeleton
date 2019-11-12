import subprocess
import os
import sys

path = {}
path['off'] = sys.argv[1]
path['skel'] = os.path.splitext(path['off'])[0] + '.skel'
path['corr'] = os.path.splitext(path['off'])[0] + '.corr'
path['ply'] = os.path.splitext(path['off'])[0] + '.ply'

subprocess.call(['skeleton', path['off'], path['skel'], path['corr']])

header = """ply
format ascii 1.0
element vertex %i
property float x
property float y
property float z
element edge %i
property int vertex1
property int vertex2
end_header
"""

f = open(path['skel'])
ar = f.read().split('\n')
f.close()
x = ''
ve = []
ed = []
for i in range(0, len(ar)):
    v = ar[i].split(' ')
    if(v[0]):
        n = int(v[0])
        a = v[1] + ' ' + v[2] + ' ' + v[3] + ' '
        b = v[(n - 1) * 3 + 1] + ' ' + v[(n - 1) * 3 + 2] + ' ' + v[(n - 1) * 3 + 3]
        try:
            ia = ve.index(a)
        except:
            ve.append(a)
            ia = ve.index(a)
        try:
            ib = ve.index(b)
        except:
            ve.append(b)
            ib = ve.index(b)
        ed.append(str(ia) + ' ' + str(ib))

with open(path['ply'], 'w') as f:
    f.write(header % (len(ve), len(ed)))
    f.write('\n'.join(ve))
    f.write('\n')
    f.write('\n'.join(ed))
