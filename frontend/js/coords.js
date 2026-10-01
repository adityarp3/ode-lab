// Pure coordinate-transform math between "math space" (the x/y or t/x range
// a tool is plotting) and "canvas space" (pixel coordinates, y flipped since
// canvas pixel-y grows downward while math-y grows upward). No DOM access —
// fully unit-testable in plain Node, see tests/coords.test.mjs.

export function makeTransform({ xMin, xMax, yMin, yMax, width, height, padding = 0 }) {
  const innerW = width - 2 * padding;
  const innerH = height - 2 * padding;

  function toCanvas(x, y) {
    const px = padding + ((x - xMin) / (xMax - xMin)) * innerW;
    const py = padding + innerH - ((y - yMin) / (yMax - yMin)) * innerH;
    return [px, py];
  }

  function toMath(px, py) {
    const x = xMin + ((px - padding) / innerW) * (xMax - xMin);
    const y = yMin + ((innerH - (py - padding)) / innerH) * (yMax - yMin);
    return [x, y];
  }

  // Scale-only conversion (no translation) — useful for direction/vector
  // arrows, where you want a length in math-units turned into a pixel
  // length without also applying the origin offset.
  function scaleToCanvas(dx, dy) {
    const px = (dx / (xMax - xMin)) * innerW;
    const py = -(dy / (yMax - yMin)) * innerH; // flipped, same as toCanvas
    return [px, py];
  }

  return { toCanvas, toMath, scaleToCanvas };
}
