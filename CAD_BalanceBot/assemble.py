"""
BalanceBot Assembly Script — FreeCAD 1.1
Run with:  cat assemble.py | /snap/bin/freecad --console

Placements last synced from BalanceBotAssembly.FCStd (manually adjusted in GUI).

NOTE — two parts use procedural geometry (replace when native files are converted):
  Wheel_R / Wheel_L  : built from Pololu 90mm wheel published specs (dia=90, w=10mm)
                       To use the real DWG: install libredwg-tools (sudo apt install
                       libredwg-tools), then run  dwg2dxf "Pololu 90mm Wheel.dwg"
                       and import the resulting DXF into FreeCAD → export to STEP.
  Samsung_INR18650   : loaded from Samsung INR18650-25R.stp (dia≈19.98mm, L=65.85mm)
"""

import FreeCAD as App
import Part

BASE = "/home/phlix/claude-spaces/NewBalanceBot/CAD_BalanceBot"
OUT  = BASE + "/BalanceBotAssembly.FCStd"

Vec = App.Vector
Rot = App.Rotation
Plc = App.Placement

def load_body(path):
    """Return a copy of the final Body shape from an FCStd file."""
    d = App.openDocument(path)
    s = d.getObject("Body").Shape.copy()
    App.closeDocument(d.Name)
    return s

def load_step(path):
    s = Part.Shape()
    s.read(path)
    return s

def load_wheel(path):
    """Return (hub_shape, tire_shape) from Pololu_Wheel_90x10.FCStd."""
    d = App.openDocument(path)
    hub  = d.getObject("Wheel_Hub").Shape.copy()
    tire = d.getObject("Wheel_Tire").Shape.copy()
    App.closeDocument(d.Name)
    return hub, tire


def add_part(doc, name, shape, color, pos=(0,0,0), rot=None):
    obj = doc.addObject("Part::Feature", name)
    obj.Shape = shape
    try:
        obj.ViewObject.ShapeColor = color
    except Exception:
        pass  # ViewObject not available in headless mode
    if rot is None:
        obj.Placement = Plc(Vec(*pos), Rot(0, 0, 0, 1))
    else:
        obj.Placement = Plc(Vec(*pos), rot)
    return obj

doc = App.newDocument("BalanceBotAssembly")

# ── 1. Carrier V4 ─────────────────────────────────────────────────────────────
# Body centered at world origin. Top face Z=+2, bottom Z=-30. Unchanged.
add_part(doc, "carrierV4",
         load_body(BASE + "/carrierV4.FCStd"),
         color=(0.80, 0.80, 0.80),
         pos=(0.0, 0.0, 0.0))

# ── 2. NANO Motor Carrier PCB ─────────────────────────────────────────────────
# Exact hole alignment (< 0.25 mm residual). Unchanged.
# PCB bottom face (local Z=-1.63) on carrier top (world Z=+2.0) → dz=3.63
add_part(doc, "NANOMotorCarrier",
         load_step(BASE + "/STEP_[No Variations] for Public_NANOMotorCarrier.PcbDoc.step"),
         color=(0.10, 0.50, 0.10),
         pos=(-24.965, -39.885, 3.63))

# ── 3. Arduino Nano 33 IoT ────────────────────────────────────────────────────
# 180° around axis (0, 1, 1)/√2  — manually positioned in GUI.
# pos Z raised to 12.53 (was 4.53); rotation updated to match GUI result.
add_part(doc, "Arduino_Nano33IoT",
         load_step(BASE + "/Arduino Nano 33 IoT.stp"),
         color=(0.00, 0.30, 0.80),
         pos=(0.0, 0.0, 12.53),
         rot=Rot(0.000000, 0.707107, 0.707107, 0.000000))

# ── 4. 18650 Battery ─────────────────────────────────────────────────────────
# 180° around axis (1, 1, 0)/√2  — manually positioned in GUI.
# Z raised from -26.2 → -4.2 (sits higher in pocket).
add_part(doc, "Battery_18650",
         load_step(BASE + "/User Library-18650 Full.STEP"),
         color=(0.70, 0.70, 0.10),
         pos=(0.0, -27.8, -4.2),
         rot=Rot(0.707107, 0.707107, 0.000000, 0.000000))

# ── 5. Motor Holder RIGHT ─────────────────────────────────────────────────────
# 180° around X axis — manually adjusted in GUI.
# Translated to (-10, 0, -30).
add_part(doc, "motorhalter_R",
         load_body(BASE + "/motorhalter_lose.FCStd"),
         color=(0.90, 0.50, 0.10),
         pos=(-10.0, 0.0, -30.0),
         rot=Rot(1.000000, 0.000000, 0.000000, 0.000000))

# ── 6. Motor Holder LEFT ──────────────────────────────────────────────────────
# 180° around Z axis — manually adjusted in GUI.
# Translated to (10, 0, 0).
add_part(doc, "motorhalter_L",
         load_body(BASE + "/motorhalter_lose.FCStd"),
         color=(0.90, 0.50, 0.10),
         pos=(10.0, 0.0, 0.0),
         rot=Rot(0.000000, 0.000000, 1.000000, 0.000000))

# ── 7. N20 Motor RIGHT (shaft → +X, no rotation) ─────────────────────────────
# No rotation. Position manually refined in GUI.
add_part(doc, "N20_motor_R",
         load_step(BASE + "/N20 Mini Micro Metall Gear Motor DC 3-6-12V v1.step"),
         color=(0.25, 0.25, 0.25),
         pos=(36.9, -0.6, -14.7))

# ── 8. N20 Motor LEFT (shaft → -X, 180° around Z) ────────────────────────────
# 180° around Z. Position manually refined in GUI.
add_part(doc, "N20_motor_L",
         load_step(BASE + "/N20 Mini Micro Metall Gear Motor DC 3-6-12V v1.step"),
         color=(0.25, 0.25, 0.25),
         pos=(-37.9, -0.4, -14.7),
         rot=Rot(0.000000, 0.000000, 1.000000, 0.000000))

# ── 9. Wheels (Pololu 90×10mm, product #1439) ────────────────────────────────
# Loaded from Pololu_Wheel_90x10.FCStd (run create_wheel.py to regenerate).
# Wheel geometry: axis along Z, inner face at Z=0, outer face at Z=10,
#                 hub boss Z=-1 to Z=11 (1 mm proud each side).
#
# Shaft tips (from GUI-adjusted motor positions):
#   Right motor: shaft tip world (55.9, -0.6, -11.0) — shaft → +X
#   Left  motor: shaft tip world (-56.9, -0.4, -11.0) — shaft → -X
#
# Ry(+90°): local Z → world +X  →  inner face (Z=0) at Px, outer face at Px+10
# Ry(-90°): local Z → world -X  →  inner face (Z=0) at Px, outer face at Px-10
#
# Right: Px=55.9  (wheel spans X [55.9, 65.9], extending outward from robot)
# Left:  Px=-56.9 (wheel spans X [-66.9, -56.9], extending outward)
WHEEL_FCStd = BASE + "/Pololu_Wheel_90x10.FCStd"
wheel_hub_shape, wheel_tire_shape = load_wheel(WHEEL_FCStd)

rot_ry_pos90 = Rot(Vec(0, 1, 0),  90)   # Z → +X
rot_ry_neg90 = Rot(Vec(0, 1, 0), -90)   # Z → -X

# Right wheel
add_part(doc, "Wheel_R_Hub",  wheel_hub_shape,  (0.95, 0.95, 0.95),
         pos=(55.9, -0.6, -11.0), rot=rot_ry_pos90)
add_part(doc, "Wheel_R_Tire", wheel_tire_shape, (0.10, 0.10, 0.10),
         pos=(55.9, -0.6, -11.0), rot=rot_ry_pos90)

# Left wheel
add_part(doc, "Wheel_L_Hub",  wheel_hub_shape,  (0.95, 0.95, 0.95),
         pos=(-56.9, -0.4, -11.0), rot=rot_ry_neg90)
add_part(doc, "Wheel_L_Tire", wheel_tire_shape, (0.10, 0.10, 0.10),
         pos=(-56.9, -0.4, -11.0), rot=rot_ry_neg90)

# ── 10. Samsung INR18650-25R battery cell ─────────────────────────────────────
# Loaded from Samsung INR18650-25R.stp
# Native bbox: long axis = Y (65.85mm), diameter = 19.98mm, centred at X=0, Z=0
#
# Placed INSIDE the Battery_18650 holder.
# Holder rotation (180° around (1,1,0)/√2) maps native-X→world-Y, so holder long
# axis is world-Y. Battery native Y is also world-Y → no rotation needed.
#
# Holder world centre: X=0, Y=0, Z=-13.15
# Battery native centre: X=0, Y=32.925, Z=0
#   → Px = 0,  Py = 0 - 32.925 = -32.925,  Pz = -13.15
#
# Clearances inside holder: X ±0.41mm, Y ±5.88mm (end-stop), Z ±2.76mm  ✓
add_part(doc, "Samsung_INR18650",
         load_step(BASE + "/Samsung INR18650-25R.stp"),
         color=(0.20, 0.20, 0.60),              # dark blue cell wrap
         pos=(0.0, -32.925, -13.15))

# ── Save ──────────────────────────────────────────────────────────────────────
doc.recompute()
doc.saveAs(OUT)
print("Assembly saved: " + OUT)
