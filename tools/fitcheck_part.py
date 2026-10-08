# Fit check of one printed part against the board mesh (see fitcheck.py for
# making board.stl.npy from the Waveshare STEP).
#
#   python tools/fitcheck_part.py part.stl <z_offset> board.stl.npy
#
# z_offset moves the STL from its print position back into model
# coordinates; for the back cups that is cup_bed_z (-15.9). The part must
# not be mirrored or rotated for printing. Reports board points inside the
# part's material, grouped by location, and the smallest gaps between board
# points and the part's surface near the 6 o'clock lug.
import sys
import numpy as np

src = open(__file__.replace('fitcheck_part.py', 'fitcheck.py')).read()
exec(src.split('\nBEZEL=')[0])                       # load()
exec('def slice_pairs' + src.split('def slice_pairs')[1].split('\nfor name,m in')[0])

part = load(sys.argv[1]).copy()
part[:, :, 2] += float(sys.argv[2])
board = np.load(sys.argv[3]).astype(np.float64)
v = part.reshape(-1, 3)
print('part model-frame bbox', v.min(0).round(2), v.max(0).round(2))

pts = np.concatenate([board.reshape(-1, 3), board.mean(1)])
zmin, zmax = v[:, 2].min(), v[:, 2].max()
hits = []
for z in np.arange(zmin + 0.05, zmax, 0.1):
    sel = pts[np.abs(pts[:, 2] - z) < 0.05]
    if not len(sel):
        continue
    a, b = slice_pairs(part, z)
    ins = np.zeros(len(sel), bool)
    for s in range(0, len(sel), 4000):
        ins[s:s + 4000] = inside(sel[s:s + 4000], a, b)
    if ins.any():
        hits.append(sel[ins])
if hits:
    H = np.concatenate(hits)
    print('COLLISIONS', len(H))
    keys = {}
    for p in H:
        keys.setdefault((round(p[0] / 3) * 3, round(p[1] / 3) * 3), []).append(p)
    for k, q in sorted(keys.items(), key=lambda kv: -len(kv[1]))[:15]:
        q = np.array(q)
        print('   near', k, 'n', len(q), 'z %.2f..%.2f' % (q[:, 2].min(), q[:, 2].max()),
              'r %.2f..%.2f' % (np.hypot(q[:, 0], q[:, 1]).min(), np.hypot(q[:, 0], q[:, 1]).max()))
else:
    print('no collisions')
