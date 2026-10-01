import assert from "node:assert/strict";
import { makeTransform } from "../coords.js";

function approx(a, b, tol = 1e-9) {
  return Math.abs(a - b) < tol;
}

// --- Basic mapping: 0..10 both axes onto a 100x100 canvas, no padding ---
{
  const t = makeTransform({ xMin: 0, xMax: 10, yMin: 0, yMax: 10, width: 100, height: 100 });

  const [px0, py0] = t.toCanvas(0, 0);
  assert.ok(approx(px0, 0) && approx(py0, 100), "math (0,0) -> canvas bottom-left (0,100)");

  const [px1, py1] = t.toCanvas(10, 10);
  assert.ok(approx(px1, 100) && approx(py1, 0), "math (10,10) -> canvas top-right (100,0)");

  const [pxm, pym] = t.toCanvas(5, 5);
  assert.ok(approx(pxm, 50) && approx(pym, 50), "math (5,5) -> canvas center (50,50)");

  console.log("[OK] basic toCanvas mapping (0..10 -> 0..100, y-flip)");
}

// --- Round-trip: toMath(toCanvas(x,y)) should recover (x,y) ---
{
  const t = makeTransform({ xMin: -5, xMax: 15, yMin: -3, yMax: 7, width: 640, height: 360, padding: 20 });
  const testPoints = [
    [-5, -3], [15, 7], [0, 0], [3.14159, 2.71828], [-4.999, 6.999],
  ];
  for (const [x, y] of testPoints) {
    const [px, py] = t.toCanvas(x, y);
    const [x2, y2] = t.toMath(px, py);
    assert.ok(approx(x, x2, 1e-6) && approx(y, y2, 1e-6), `round-trip failed for (${x}, ${y})`);
  }
  console.log("[OK] round-trip toCanvas -> toMath recovers original point (with padding)");
}

// --- scaleToCanvas: pure direction/length, no origin offset ---
{
  const t = makeTransform({ xMin: 0, xMax: 10, yMin: 0, yMax: 10, width: 100, height: 100 });
  const [dpx, dpy] = t.scaleToCanvas(1, 0); // 1 math-unit in x -> 10 px, no y change
  assert.ok(approx(dpx, 10) && approx(dpy, 0), "scaleToCanvas(1,0) -> (10,0)");

  const [dpx2, dpy2] = t.scaleToCanvas(0, 1); // 1 math-unit in y -> -10 px (flipped)
  assert.ok(approx(dpx2, 0) && approx(dpy2, -10), "scaleToCanvas(0,1) -> (0,-10) (y flipped)");
  console.log("[OK] scaleToCanvas gives correct direction/length without translation");
}

console.log("\nAll coords.js tests passed.");
