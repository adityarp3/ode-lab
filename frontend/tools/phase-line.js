import { makeTransform } from "../js/coords.js";
import { clear, drawAxes, drawCurve, drawFixedPoints, drawPoint } from "../js/canvas-utils.js";
import { phaseLine, phaseLineParticle } from "../js/api.js";

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const errorBanner = document.getElementById("error-banner");

let currentResult = null;
let transform = null;
let currentXMin = -2;
let currentXMax = 12;
let particleX = null; // current animated particle position, or null if none
let animationToken = 0; // incremented on each new click, invalidates in-flight animations

function showError(msg) {
  errorBanner.textContent = msg;
  errorBanner.classList.add("visible");
}
function clearError() {
  errorBanner.classList.remove("visible");
}

function parseParams(text) {
  const params = {};
  const trimmed = text.trim();
  if (!trimmed) return params;
  for (const pair of trimmed.split(",")) {
    const [k, v] = pair.split("=").map((s) => s.trim());
    if (k) params[k] = parseFloat(v);
  }
  return params;
}

function getFormValues() {
  return {
    expression: document.getElementById("expression").value,
    xMin: parseFloat(document.getElementById("xMin").value),
    xMax: parseFloat(document.getElementById("xMax").value),
    params: parseParams(document.getElementById("params").value),
  };
}

function render() {
  clear(ctx, canvas.width, canvas.height);
  if (!currentResult) return;

  const finiteXdot = currentResult.xdot.filter((v) => v !== null && Number.isFinite(v));
  const maxAbs = finiteXdot.length ? Math.max(...finiteXdot.map(Math.abs)) : 1;
  const yMax = Math.max(1e-6, maxAbs) * 1.15;
  const yMin = -yMax;

  transform = makeTransform({
    xMin: currentXMin, xMax: currentXMax, yMin, yMax,
    width: canvas.width, height: canvas.height, padding: 30,
  });

  drawAxes(ctx, transform, { xMin: currentXMin, xMax: currentXMax, yMin, yMax });

  const points = currentResult.x
    .map((x, i) => [x, currentResult.xdot[i]])
    .filter(([, y]) => y !== null && Number.isFinite(y));
  drawCurve(ctx, transform, points, { color: "#2563eb" });

  drawFixedPoints(ctx, transform, currentResult.fixed_points, 0);

  if (particleX !== null) {
    drawPoint(ctx, transform, particleX, 0, { color: "#111827", radius: 6 });
  }

  if (currentResult.has_invalid_samples) {
    ctx.fillStyle = "#f59e0b";
    ctx.font = "12px sans-serif";
    ctx.fillText("⚠ some samples were undefined (gaps in the curve)", 10, 16);
  }
}

async function plot() {
  clearError();
  const { expression, xMin, xMax, params } = getFormValues();
  currentXMin = xMin;
  currentXMax = xMax;
  particleX = null;
  try {
    currentResult = await phaseLine({ expression, params, xMin, xMax, nSamples: 500 });
    render();
  } catch (e) {
    showError(e.message);
  }
}

function animateParticle(traj, token) {
  const n = traj.x.length;
  if (n === 0) return;
  let i = 0;
  // Always finish the animation in roughly the same number of frames
  // regardless of how many integration steps it took.
  const stepsPerFrame = Math.max(1, Math.floor(n / 240));

  function frame() {
    if (token !== animationToken) return; // a newer click superseded this animation
    if (i >= n) {
      particleX = traj.x[n - 1][0];
      render();
      return;
    }
    particleX = traj.x[i][0];
    render();
    i += stepsPerFrame;
    requestAnimationFrame(frame);
  }
  frame();
}

canvas.addEventListener("click", async (evt) => {
  if (!transform) return;
  const rect = canvas.getBoundingClientRect();
  const px = evt.clientX - rect.left;
  const py = evt.clientY - rect.top;
  const [mathX] = transform.toMath(px, py);

  const { expression, params } = getFormValues();
  const token = ++animationToken;
  try {
    clearError();
    const traj = await phaseLineParticle({ expression, params, clickX: mathX, tMax: 20, dt: 0.02 });
    if (traj.diverged) {
      showError(`Particle diverged: ${traj.divergence_reason}`);
    }
    animateParticle(traj, token);
  } catch (e) {
    showError(e.message);
  }
});

document.getElementById("plot-btn").addEventListener("click", plot);

plot();
