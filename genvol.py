import sys
from array import array

if len(sys.argv) < 2:
    z = 16
else:
    z = int(sys.argv[1])

dim = { 'x': 4, 'y': 4, 'z': z }
print(dim)
volume = [[] for _ in range(dim['z'])]

# for i in range(dim['z']):
#     volume[i] = [1 - (i % 2) for j in range(dim['x'] * dim['y'])]

for i in range(dim['z']):
    volume[i] = [1 if i % 4 == j % 4 else 0 for j in range(dim['x'] * dim['y'])]

# for i in range(dim['z']):
#     volume[i] = [1 for j in range(dim['x'] * dim['y'])]


# volume[-1] = [0 for i in range(dim['x'] * dim['y'])]

filename = 'o%d-%d-%d.vol' % (dim['x'], dim['y'], dim['z'])

def pack(slice_contents):
    """
    Pack a array of 1|0 into an array of bytes
    e.g. [1, 0, 0, 0, 0, 0, 0, 1] -> [129]
    """
    assert(len(slice_contents) % 8 == 0)

    output = [0 for _ in range(len(slice_contents) // 8)]
    for i, e in enumerate(slice_contents):
        assert(e == 0 or e == 1)

        byte_no = i // 8
        byte_index = i % 8
        output[byte_no] = output[byte_no] | (e << byte_index)

    return output


with open(filename, 'wb') as f:
    f.write(array('B', [ ord(e) for e in 'VOL' ]).tobytes())
    f.write(array('I', [ dim[e] for e in 'xyz' ]).tobytes())
    f.write(array('B', [ ord(e) for e in 'LOV' ]).tobytes())

    for _slice in volume:
#        print(_slice)
        f.write(array('B', pack(_slice)).tobytes())

