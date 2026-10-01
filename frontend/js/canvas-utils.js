// Canvas drawing helpers. These take a 2D context + a coords.js transform
// and draw plain shapes — no framework, no state beyond what's passed in.
// Unlike coords.js/api.js, this can't be unit-tested without a browser
// (no canvas implementation available in this project's dev environment),
// so it's kept deliberately simple: basic strokes and fills, nothing that
// depends on subtle canvas behavior.

export function clear(ctx, width, height) {
  ctx.clearRect(0, 0, width, height);
}

export function drawAxes(ctx, transform, { xMin, xMax, yMin, yMax }, color = "#94a3b8") {
  ctx.strokeStyle = color;
  ctx.lineWidth = 1;

  if (xMin <= 0 && 0 <= xMax) {
    const [x0, y0] = transform.toCanvas(0, yMin);
    const [x1, y1] = transform.toCanvas(0, yMax);
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();
  }
  if (yMin <= 0 && 0 <= yMax) {
    const [x0, y0] = transform.toCanvas(xMin, 0);
    const [x1, y1] = transform.toCanvas(xMax, 0);
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();
  }
}

// Draws a small arrow at each field sample, oriented along (vx, vy),
// scaled to a fixed pixel length so the field reads as directions, not
// magnitudes (magnitude differences across a field can span orders of
// magnitude and would make a literal-length plot unreadable).
export function drawVectorField(ctx, transform, samples, { arrowLength = 12, color = "#cbd5e1" } = {}) {
  ctx.strokeStyle = color;
  ctx.lineWidth = 1;

  for (const s of samples) {
    const [ax, ay] = s.position;
    const v = s.vector;
    if (!v || v.length < 1) continue;

    // 1D field (slope field): vector is [dx/dt]; direction is (1, dx/dt).
    // 2D field: vector is [dx/dt, dy/dt]; direction is (dx/dt, dy/dt).
    const [vx, vy] = v.length === 1 ? [1, v[0]] : [v[0], v[1]];
    if (!Number.isFinite(vx) || !Number.isFinite(vy)) continue; // skip invalid samples

    const mag = Math.hypot(vx, vy) || 1;
    const ux = vx / mag;
    const uy = vy / mag;

    const [cx, cy] = transform.toCanvas(ax, ay);
    // Build the arrow directly in pixel space (constant on-screen length)
    // rather than transforming a math-space vector, so it stays legible
    // regardless of the axis ranges.
    const dx = ux * arrowLength;
    const dy = -uy * arrowLength; // canvas y is flipped vs. math y

    const x0 = cx - dx / 2;
    const y0 = cy - dy / 2;
    const x1 = cx + dx / 2;
    const y1 = cy + dy / 2;

    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();

    // Small arrowhead.
    const headLen = 3;
    const angle = Math.atan2(dy, dx);
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x1 - headLen * Math.cos(angle - Math.PI / 6), y1 - headLen * Math.sin(angle - Math.PI / 6));
    ctx.moveTo(x1, y1);
    ctx.lineTo(x1 - headLen * Math.cos(angle + Math.PI / 6), y1 - headLen * Math.sin(angle + Math.PI / 6));
    ctx.stroke();
  }
}

// Draws a polyline through math-space points [[x,y], ...] or [x,y] pairs.
export function drawCurve(ctx, transform, points, { color = "#2563eb", lineWidth = 2 } = {}) {
  if (!points || points.length === 0) return;
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.beginPath();
  points.forEach(([x, y], i) => {
    const [cx, cy] = transform.toCanvas(x, y);
    if (i === 0) ctx.moveTo(cx, cy);
    else ctx.lineTo(cx, cy);
  });
  ctx.stroke();
}

// Draws small dots at each math-space point — used for isocline/nullcline
// point clouds and for a scattered (non-connected) curve.
export function drawDots(ctx, transform, points, { color = "#16a34a", radius = 1.5 } = {}) {
  ctx.fillStyle = color;
  for (const [x, y] of points) {
    const [cx, cy] = transform.toCanvas(x, y);
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
    ctx.fill();
  }
}

// Fixed points on a phase line, color-coded by stability.
export function drawFixedPoints(ctx, transform, fixedPoints, y = 0) {
  const colors = { stable: "#16a34a", unstable: "#dc2626", "semi-stable": "#f59e0b" };
  for (const fp of fixedPoints) {
    const [cx, cy] = transform.toCanvas(fp.x, y);
    ctx.fillStyle = colors[fp.stability] || "#64748b";
    ctx.beginPath();
    ctx.arc(cx, cy, 6, 0, 2 * Math.PI);
    ctx.fill();
  }
}

export function drawPoint(ctx, transform, x, y, { color = "#111827", radius = 5 } = {}) {
  const [cx, cy] = transform.toCanvas(x, y);
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
  ctx.fill();
}
