// pocketwatch.scad -- issue 3
// Pocket-watch style case for the Waveshare ESP32-S3-Touch-LCD-1.46
// (cover-glass version, SKU 29565) plus a 20 x 30 mm LiPo pouch cell.
//
//   front : bezel, case band, crown and bow, three screw bosses
//   back  : caseback plate carrying the board clamp posts, the battery
//           cradle and a PWR pinhole guide
//
//   Three M2.5 x 12 countersunk self-tapping screws go in from the back,
//   through the caseback, into bosses in the case band. The board's own
//   standoffs are M2 female, so the screws never touch the board: posts on
//   the caseback land on the standoff tips, with pins that drop into the
//   standoff bores, and clamp the lens against the bezel lip.
//
// Render (each part lands flat on z=0 in its print orientation):
//   openscad -o front.stl -D 'part="front"' pocketwatch.scad
//   openscad -o back.stl  -D 'part="back"'  pocketwatch.scad
//
// Coordinate system is the Waveshare STEP model's, so board positions can
// be copied straight from it: looking at the display, x right, y up (12
// o'clock, where the crown is), USB-C at 6 o'clock. z = 0 is the front of
// the cover glass and z goes NEGATIVE towards the back of the board.
//
// Tags on dimensions:
//   [S] measured from the Waveshare STEP model (ESP32-S3-Touch-LCD-1_46.stp)
//   [W] Waveshare dimension drawing
//   [M] measured on real hardware
//   [?] assumption, check before trusting

part = "assembly";      // "front" | "back" | "both" | "assembly"

$fn = 96;
eps = 0.01;

/* ---------- board ---------------------------------------------------- */

lens_d      = 44.78;    // [S][W] cover glass OD (drawing: 44.77 +/- 0.05)
lens_t      = 1.60;     // [W] cover glass thickness
va_d        = 37.56;    // [W] viewable area
pcb_back_z  = -7.40;    // [S][W] back face of the PCB
standoff_tip_z = -10.91;// [S] tips of the three M2 x 3.5 SMT standoffs
header_tip_z   = -12.95;// [S] 2x10 1.27 mm header, the deepest thing on
                        //     the board (x -17.0..-11.5, y -6.1..6.3)
standoffs = [[12.00, -14.95], [-11.54, -15.45], [0.00, 17.75]];  // [S][W]
standoff_bore_d = 1.6;  // [?] M2 thread minor diameter, through-bore assumed

// The FPC fold of the display sticks out below the glass outline: a tab
// x -10.0..10.0 reaching y -23.12, between z -1.7 and -7.4 [S]. The case
// carries a matching notch, which also keys the board's rotation so the
// USB-C always lines up with its port.
tab_half_w  = 10.0;     // [S]
tab_y       = -23.12;   // [S]
tab_z       = [-7.45, -1.65];   // [S]

// USB-C receptacle: shell x -4.47..4.47, face at y -23.00, shell z about
// -7.5..-10.75 [S]. Plug overmoulds are commonly up to 12.5 x 7.0.
usb_z       = -9.10;    // [S] shell centre
usb_face_y  = -23.00;   // [S]
plug_w      = 12.5;     // [?] overmould width
plug_h      = 7.0;      // [?] overmould height
plug_clr    = 0.3;
plug_r      = 2.0;

// PWR button (TS24CA, top actuated, faces the back) [S]. BOOT sits under
// the battery and is not reachable in this phase.
pwr_xy      = [14.33, 9.91];    // [S] body centre
pwr_top_z   = -9.20;    // [S]

/* ---------- battery -------------------------------------------------- */

bat_w       = 30.0;     // [?] LiPo pouch, long side (along x)
bat_h       = 20.0;     // [?] short side (along y)
bat_t       = 5.0;      // [?] thickness: 502030 is 5.0, 402030 is 4.0
bat_clr     = 0.25;     // per side in the cradle
bat_gap     = 0.5;      // air between the header tips and the cell
bat_slack   = 0.3;      // behind the cell, for foam tape or swelling
// Cell sits low enough that the PWR guide tube passes above it and high
// enough to clear the two lower clamp posts.
bat_cy      = -2.15;    // centre y of the cradle

/* ---------- case ----------------------------------------------------- */

case_od     = 54.0;
bezel_t     = 1.2;      // lip in front of the glass
aperture_d  = 39.2;     // opening at the glass; va_d + 0.8 per side
bore_clr    = 0.16;     // glass to bore, per side
bore_d      = lens_d + 2*bore_clr;
front_chamfer = 0.8;
back_chamfer  = 0.8;

// Bezel flutes, a pocket-watch coin edge on the front of the band
flutes      = 72;
flute_d     = 1.0;
flute_len   = 3.2;      // from the front face backwards

back_t      = 2.4;      // caseback plate
rim_z       = header_tip_z - bat_gap - bat_t - bat_slack;  // band/back split
back_z      = rim_z - back_t;                              // outer back face

/* ---------- fasteners ------------------------------------------------ */
// Same screws and seat as the xroar-waveshare-rp2350-pizero case, including
// its lessons: 2.2 pilot, 90 degree seat sunk 0.5 below the face, bosses
// tied into the wall rather than standing free.

screw_len   = 12.0;     // M2.5 x 12 countersunk, self tapping
screw_clear_d = 2.8;
screw_head_d  = 5.0;
screw_head_h  = 1.1;    // 5.0 -> 2.8 over 1.1 is 90 degrees
screw_head_recess = 0.5;
pilot_d     = 2.2;
boss_r      = 23.6;     // radius of the screw circle
boss_d      = 5.2;
boss_front_z = -2.2;    // bosses stop behind the glass edge
// Not 120 degree symmetric, so the caseback fits one way only.
// Board envelope at these angles is 20.8 max behind the glass [S]; the
// boss inner edge is at boss_r - boss_d/2 = 21.0.
boss_angles = [90, -25, -155];
pilot_bottom_z = back_z + screw_len + 0.75;

/* ---------- caseback features --------------------------------------- */

spigot_h    = 1.5;      // locating ring that enters the bore
spigot_wall = 1.4;
spigot_clr  = 0.2;
post_d      = 4.4;
post_preload = 0.10;    // posts this much proud of the standoff tips
pin_d       = 1.4;
pin_len     = 1.6;
cradle_h    = 1.5;
cradle_t    = 1.2;
pwr_tube_od = 3.2;
pwr_tube_id = 1.3;      // SIM tool / paperclip
pwr_tube_gap = 0.6;     // tube end to switch top

/* ---------- crown and bow -------------------------------------------- */
// Both print flat on the bed with the front face, so the bow's layers run
// in the plane of the ring and a pull on the chain loads them along the
// layers, not across them.

bow_t       = 4.0;      // thickness along z
crown_d     = 5.0;
crown_len   = 4.5;      // proud of the case
crown_knurl = 14;
bow_od      = 17.0;
bow_id      = 9.5;
bow_chamfer = 0.8;

/* ---------- version -------------------------------------------------- */
// Engraved inside each part. Bump only the part whose geometry changes.
front_ver = "PW-F v1";
back_ver  = "PW-B v1";
ver_size  = 2.2;
ver_d     = 0.4;

/* ---------- derived checks ------------------------------------------ */

echo(str("rim_z (band/back split) = ", rim_z, "  back face z = ", back_z));
echo(str("case thickness = ", bezel_t - back_z, " mm, OD ", case_od));
echo(str("screw thread engagement = ", screw_len - back_t, " mm"));
echo(str("post length = ", standoff_tip_z + post_preload - rim_z));
bat_y0 = bat_cy - bat_h/2 - bat_clr;
bat_y1 = bat_cy + bat_h/2 + bat_clr;
echo(str("battery top edge to PWR tube = ",
         pwr_xy[1] - pwr_tube_od/2 - bat_y1));
for (s = [standoffs[0], standoffs[1]])
    echo(str("battery bottom edge to post at ", s, " = ",
             bat_y0 - (s[1] + post_d/2)));
assert(pwr_xy[1] - pwr_tube_od/2 > bat_y1, "PWR tube hits the battery");
assert(boss_r - boss_d/2 >= 20.9, "bosses foul the board");

/* ---------- helpers -------------------------------------------------- */

module at_bosses() {
    for (a = boss_angles) rotate(a) translate([boss_r, 0, 0]) children();
}

module ring(od, id, h) {
    difference() {
        cylinder(d = od, h = h);
        translate([0, 0, -eps]) cylinder(d = id, h = h + 2*eps);
    }
}

// Rounded rectangle in the XZ plane, extruded along y (for the USB port)
module port_cut(w, h, r, y0, y1) {
    translate([0, y1, 0]) rotate([90, 0, 0])
        linear_extrude(height = y1 - y0)
            offset(r = r) square([w - 2*r, h - 2*r], center = true);
}

module engrave(txt) {
    linear_extrude(height = ver_d + eps)
        text(txt, size = ver_size, halign = "center", valign = "center");
}

/* ---------- front: band, bezel, crown, bow --------------------------- */

module band_solid() {
    h = bezel_t - rim_z;
    translate([0, 0, rim_z]) hull() {
        translate([0, 0, back_chamfer])
            cylinder(d = case_od, h = h - back_chamfer - front_chamfer);
        cylinder(d = case_od - 2*back_chamfer, h = h);
    }
}

module crown_and_bow() {
    r_case = case_od/2;
    z_bed  = bezel_t;                    // front face, on the bed
    // Crown: horizontal knurled cylinder along y, flattened on the bed
    intersection() {
        translate([0, r_case - 1.5, z_bed - crown_d/2 + 0.6])
            rotate([-90, 0, 0]) difference() {
                cylinder(d = crown_d, h = crown_len + 1.5, $fn = 48);
                for (i = [0 : crown_knurl - 1])
                    rotate(i * 360 / crown_knurl)
                        translate([crown_d/2 + 0.15, 0, 1.5 + 0.8])
                            cylinder(d = 0.9, h = crown_len, $fn = 12);
            }
        translate([-10, 0, z_bed - 20]) cube([20, 80, 20]);
    }
    // Bow: a flat ring in the plane of the face, chamfered where it meets
    // the bed so its first layers do not overhang
    bow_cy = r_case + crown_len + bow_od/2 - 1.2;
    wall = (bow_od - bow_id) / 2;
    translate([0, bow_cy, z_bed - bow_t])
        rotate_extrude($fn = 96)
            translate([bow_id/2, 0])
                polygon([[0, bow_t - bow_chamfer], [bow_chamfer, bow_t],
                         [wall - bow_chamfer, bow_t], [wall, bow_t - bow_chamfer],
                         [wall, 0.6], [wall - 0.6, 0], [0.6, 0], [0, 0.6]]);
}

module front() {
    difference() {
        union() {
            difference() {
                band_solid();
                // bore for glass and board, open at the back
                translate([0, 0, rim_z - eps])
                    cylinder(d = bore_d, h = -rim_z + eps);
            }
            // screw bosses, merged into the wall
            intersection() {
                at_bosses() translate([0, 0, rim_z])
                    cylinder(d = boss_d, h = boss_front_z - rim_z, $fn = 48);
                cylinder(d = case_od - 1, h = 100, center = true);
            }
            crown_and_bow();
        }
        // viewing aperture, chamfered outwards at 45 degrees
        translate([0, 0, -eps])
            cylinder(d1 = aperture_d, d2 = aperture_d + 2*bezel_t + 2*eps,
                     h = bezel_t + 2*eps);
        // notch for the display FPC tab (keys rotation)
        translate([-tab_half_w - 0.4, tab_y - 0.45, tab_z[0] - 0.3])
            cube([2*tab_half_w + 0.8, 5, tab_z[1] - tab_z[0] + 0.3 + 0.05]);
        // USB-C plug pocket, straight through the band down to the
        // receptacle face so a plug seats fully
        translate([0, 0, usb_z])
            port_cut(plug_w + 2*plug_clr, plug_h + 2*plug_clr, plug_r,
                     -case_od/2 - 1, usb_face_y + 1.2);
        // pilots
        at_bosses() translate([0, 0, rim_z - eps])
            cylinder(d = pilot_d, h = pilot_bottom_z - rim_z + eps, $fn = 24);
        // bezel flutes
        for (i = [0 : flutes - 1])
            rotate(i * 360 / flutes + 180 / flutes)
                translate([case_od/2 + flute_d * 0.2, 0, bezel_t - flute_len])
                    cylinder(d = flute_d, h = flute_len + 1, $fn = 12);
        // version on the rim face (top of the print), at 7 o'clock
        rotate(-120) translate([(bore_d/2 + case_od/2)/2, 0, rim_z - eps])
            rotate(90) mirror([1, 0, 0]) engrave(front_ver);
    }
}

/* ---------- back: caseback, posts, cradle, PWR guide ----------------- */

module back() {
    bw = bat_w + 2*bat_clr;
    bh = bat_h + 2*bat_clr;
    difference() {
        union() {
            // plate, chamfered on the outer (bed) edge
            translate([0, 0, back_z]) hull() {
                translate([0, 0, back_chamfer])
                    cylinder(d = case_od, h = back_t - back_chamfer);
                cylinder(d = case_od - 2*back_chamfer, h = back_t);
            }
            // spigot ring into the bore, relieved around the bosses
            difference() {
                translate([0, 0, rim_z])
                    ring(bore_d - 2*spigot_clr,
                         bore_d - 2*spigot_clr - 2*spigot_wall, spigot_h);
                at_bosses() translate([0, 0, rim_z - eps])
                    cylinder(d = boss_d + 1.0, h = spigot_h + 1, $fn = 48);
            }
            // clamp posts with locating pins
            for (s = standoffs) translate([s[0], s[1], rim_z]) {
                cylinder(d = post_d, h = standoff_tip_z + post_preload - rim_z,
                         $fn = 48);
                cylinder(d = pin_d, h = standoff_tip_z + post_preload - rim_z
                                        + pin_len, $fn = 24);
            }
            // battery cradle, open at the top for the lead
            translate([0, bat_cy, rim_z]) difference() {
                translate([-bw/2 - cradle_t, -bh/2 - cradle_t, 0])
                    cube([bw + 2*cradle_t, bh + 2*cradle_t, cradle_h]);
                translate([-bw/2, -bh/2, -eps])
                    cube([bw, bh, cradle_h + 1]);
                translate([-bw/2 + 4, 0, -eps])
                    cube([bw - 8, bh, cradle_h + 1]);
            }
            // PWR guide tube
            translate([pwr_xy[0], pwr_xy[1], rim_z])
                cylinder(d = pwr_tube_od,
                         h = pwr_top_z - pwr_tube_gap - rim_z, $fn = 32);
        }
        // PWR pinhole, through plate and tube
        translate([pwr_xy[0], pwr_xy[1], back_z - eps])
            cylinder(d = pwr_tube_id, h = 40, $fn = 16);
        // countersunk screw holes
        at_bosses() {
            translate([0, 0, back_z - eps])
                cylinder(d = screw_clear_d, h = back_t + spigot_h + 1, $fn = 24);
            translate([0, 0, back_z - eps])
                cylinder(d = screw_head_d, h = screw_head_recess + eps, $fn = 48);
            translate([0, 0, back_z + screw_head_recess])
                cylinder(d1 = screw_head_d, d2 = screw_clear_d,
                         h = screw_head_h, $fn = 48);
        }
        // caseback decoration: two concentric grooves on the outer face
        for (r = [20.3, 15.5])
            translate([0, 0, back_z - eps]) ring(2*r + 0.8, 2*r - 0.8, 0.4);
        // version, inside face, reads from the front
        translate([0, bat_cy, rim_z - ver_d]) engrave(back_ver);
    }
}

/* ---------- board envelope (assembly view only) ---------------------- */

module board_envelope() {
    color("black") translate([0, 0, -lens_t]) cylinder(d = lens_d, h = lens_t);
    color("dimgray") translate([0, 0, -4.7]) cylinder(d = 40.8, h = 4.7 - lens_t);
    color("dimgray") translate([-tab_half_w, tab_y, tab_z[0]])
        cube([2*tab_half_w, 3, tab_z[1] - tab_z[0]]);
    color("darkgreen") translate([0, 0, pcb_back_z]) cylinder(d = 39.18, h = 1.6);
    color("gold") for (s = standoffs) translate([s[0], s[1], standoff_tip_z])
        cylinder(d = 3.5, h = pcb_back_z - standoff_tip_z);
    color("silver") translate([-4.47, usb_face_y, -10.75]) cube([8.94, 7.6, 3.25]);
    color("black") translate([-17.02, -6.1, header_tip_z])
        cube([5.5, 12.4, pcb_back_z - header_tip_z]);
    color("silver") translate([pwr_xy[0] - 2, pwr_xy[1] - 2, pwr_top_z])
        cube([4, 4, pcb_back_z - pwr_top_z]);
    color("skyblue") translate([-bat_w/2, bat_cy - bat_h/2,
                                header_tip_z - bat_gap - bat_t])
        cube([bat_w, bat_h, bat_t]);
}

/* ---------- output --------------------------------------------------- */

if (part == "front") {
    // front face down: rotate about y (never mirror), then lift
    translate([0, 0, bezel_t]) rotate([0, 180, 0]) front();
} else if (part == "back") {
    translate([0, 0, -back_z]) back();
} else if (part == "both") {
    translate([-case_od/2 - 6, 0, bezel_t]) rotate([0, 180, 0]) front();
    translate([ case_od/2 + 6, 0, -back_z]) back();
} else {
    color("silver", 0.55) front();
    color("lightsteelblue") back();
    board_envelope();
}
