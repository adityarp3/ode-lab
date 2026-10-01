import { makeTransform } from "../js/coords.js";
import { clear, drawAxes, drawVectorField, drawDots, drawCurve, drawPoint } from "../js/canvas-utils.js";
import { directionField2D, directionFieldIsocline, directionFieldTrajectory } from "../js/api.js";

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const errorBanner = document.getElementById("error-banner");

let currentField = null;
let transform = null;
let ranges = null; // { xMin, xMax, yMin, yMax }
let xNullcline = [];
let yNullcline = [];
let trajectoryPoints = []; // [[x,y], ...] progressively revealed during animation
let dragging = false;
let requestToken = 0;
let animationToken = 0;

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
    expressions: [document.getElementById("exprX").value, document.getElementById("exprY").value],
    params: parseParams(document.getElementById("params").value),
    xMin: parseFloat(document.getElementById("xMin").value),
    xMax: parseFloat(document.getElementById("xMax").value),
    yMin: parseFloat(document.getElementById("yMin").value),
    yMax: parseFloat(document.getElementById("yMax").value),
  };
}

function getMode() {
  return document.querySelector('input[name="mode"]:checked').value;
}

function render() {
  clear(ctx, canvas.width, canvas.height);
  if (!currentField || !ranges) return;

  transform = makeTransform({
    xMin: ranges.xMin, xMax: ranges.xMax, yMin: ranges.yMin, yMax: ranges.yMax,
    width: canvas.width, height: canvas.height, padding: 30,
  });

  drawAxes(ctx, transform, ranges);
  drawVectorField(ctx, transform, currentField.samples, { arrowLength: 10 });

  if (xNullcline.length) drawDots(ctx, transform, xNullcline.map((p) => [p.axis1, p.axis2]), { color: "#16a34a", radius: 2 });
  if (yNullcline.length) drawDots(ctx, transform, yNullcline.map((p) => [p.axis1, p.axis2]), { color: "#dc2626", radius: 2 });
  if (trajectoryPoints.length) drawCurve(ctx, transform, trajectoryPoints, { color: "#2563eb", lineWidth: 2 });

  if (currentField.has_invalid_samples) {
    ctx.fillStyle = "#f59e0b";
    ctx.font = "12px sans-serif";
    ctx.fillText("⚠ some grid points were undefined", 10, 16);
  }
}

async function plot() {
  clearError();
  const { expressions, params, xMin, xMax, yMin, yMax } = getFormValues();
  ranges = { xMin, xMax, yMin, yMax };
  xNullcline = [];
  yNullcline = [];
  trajectoryPoints = [];
  try {
    currentField = await directionField2D({ expressions, params, xMin, xMax, nX: 41, yMin, yMax, nY: 41 });
    render();
  } catch (e) {
    showError(e.message);
  }
}

async function placeNullclineAt(component, clickAxis1, clickAxis2) {
  const { expressions, params, xMin, xMax, yMin, yMax } = getFormValues();
  const token = ++requestToken;
  try {
    const points = await directionFieldIsocline({
      expressions, params, xMin, xMax, nX: 41, yMin, yMax, nY: 41,
      vectorComponent: component, clickAxis1, clickAxis2,
    });
    if (token !== requestToken) return;
    if (component === 0) xNullcline = points;
    else yNullcline = points;
    clearError();
    render();
  } catch (e) {
    if (token === requestToken) showError(e.message);
  }
}

function animateTrajectory(traj, token) {
  const n = traj.x.length;
  if (n === 0) return;
  let i = 0;
  const stepsPerFrame = Math.max(1, Math.floor(n / 400));

  function frame() {
    if (token !== animationToken) return;
    trajectoryPoints = traj.x.slice(0, i + 1);
    render();
    if (i >= n - 1) return;
    i = Math.min(n - 1, i + stepsPerFrame);
    requestAnimationFrame(frame);
  }
  frame();
}

async function dropTrajectoryAt(clickX, clickY) {
  const { expressions, params } = getFormValues();
  const token = ++animationToken;
  try {
    clearError();
    const traj = await directionFieldTrajectory({ expressions, params, clickX, clickY, tMax: 30, dt: 0.01 });
    if (traj.diverged) showError(`Trajectory diverged: ${traj.divergence_reason}`);
    animateTrajectory(traj, token);
  } catch (e) {
    showError(e.message);
  }
}

function handlePointerEvent(evt) {
  if (!transform) return;
  const rect = canvas.getBoundingClientRect();
  const px = evt.clientX - rect.left;
  const py = evt.clientY - rect.top;
  const [x, y] = transform.toMath(px, py);

  const mode = getMode();
  if (mode === "trajectory") dropTrajectoryAt(x, y);
  else if (mode === "x-null") placeNullclineAt(0, x, y);
  else if (mode === "y-null") placeNullclineAt(1, x, y);
}

canvas.addEventListener("mousedown", (evt) => {
  dragging = true;
  handlePointerEvent(evt);
});
canvas.addEventListener("mousemove", (evt) => {
  if (dragging) handlePointerEvent(evt);
});
window.addEventListener("mouseup", () => {
  dragging = false;
});

document.getElementById("plot-btn").addEventListener("click", plot);

plot();
