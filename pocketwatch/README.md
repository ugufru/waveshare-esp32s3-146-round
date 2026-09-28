# Pocket-watch case (issue 3)

A two-part case for the Waveshare ESP32-S3-Touch-LCD-1.46 (cover-glass
version, SKU 29565) and a 20 x 30 mm LiPo pouch cell, with a crown and bow at
12 o'clock for a chain or tether. Source is `pocketwatch.scad`; the STLs are
rendered from it.

54.0 mm across the band, 22.35 mm thick, 47.3 mm from centre to the top of
the bow. Three M2.5 x 12 countersunk self-tapping screws, the same ones used
on the xroar-waveshare-rp2350-pizero case. No supports.

| Part | File | Print orientation | Height |
| --- | --- | --- | --- |
| Front (band, bezel, crown, bow) | `front.stl` | front face down | 19.95 |
| Back (caseback, posts, cradle) | `back.stl` | outer face down | 11.94 |

## How it goes together

1. Stick the cell into the cradle on the caseback (foam tape), lead towards
   12 o'clock, and plug it into the board's MX1.25 BAT socket.
2. Drop the board into the front, glass first. The display's FPC tab at
   6 o'clock drops into a notch, which fixes the rotation and lines USB-C up
   with its port.
3. Fit the caseback. It only goes on one way (the screws are not on a 120
   degree pattern and the spigot is relieved around the bosses). The three
   posts land on the board's M2 standoffs, with pins in the standoff bores,
   and clamp the glass against the bezel lip with 0.1 mm of preload.
4. Three screws in from the back.

The board's standoffs are M2 female, so the M2.5 screws cannot go into the
board. They go into bosses in the case band instead, and the posts do the
clamping.

PWR is reached with a SIM tool or paperclip through the pinhole in the back.

## Stack (z from the front of the glass, negative towards the back)

| z | What |
| --- | --- |
| +1.2 to 0 | bezel lip, 39.2 opening chamfered out to 41.6 |
| 0 to -1.6 | cover glass, 44.78 in a 45.10 bore |
| -7.40 | back of the PCB |
| -10.91 | standoff tips, where the posts land |
| -12.95 | tips of the 2x10 header, the deepest part of the board |
| -13.45 to -18.45 | cell (5.0 thick) |
| -18.75 | band/back split |
| -21.15 | outer face of the caseback |

## Checked

- Both parts render as single manifold solids in OpenSCAD 2021.01.
- The rendered STLs were checked against a mesh of Waveshare's STEP model
  of the board (`tools/fitcheck.py`). The front has no contact. The back
  touches only at the three standoff tips, which is the intended 0.1 mm
  preload. That run also shows the checker detects contact.
- The battery clears the PWR tube by 0.21 and the lower posts by 0.35 and
  0.85 (echoed at render time).

Not checked yet: anything on a real print. Values tagged `[?]` in the source
are assumptions: the cell's size (especially thickness), the standoff bore,
and USB-C plug overmould size.

## Rendering

```sh
make stl        # from the repo root
```

To re-run the fit check you need the board mesh, which is too big to commit.
See `tools/fitcheck.py` for how to generate it from the Waveshare STEP.
