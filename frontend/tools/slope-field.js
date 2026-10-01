import { makeTransform } from "../js/coords.js";
import { clear, drawAxes, drawVectorField, drawDots } from "../js/canvas-utils.js";
import { slopeField, slopeFieldIsocline } from "../js/api.js";

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const errorBanner = document.getElementById("error-banner");

let currentField = null;
let transform = null;
let ranges = null; // { tMin, tMax, xMin, xMax } used to build the field
let isoclinePoints = [];
let dragging = false;
let requestToken = 0; // invalidate stale in-flight isocline requests during a fast drag

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
    params: parseParams(document.getElementById("params").value),
    tMin: parseFloat(document.getElementById("tMin").value),
    tMax: parseFloat(document.getElementById("tMax").value),
    xMin: parseFloat(document.getElementById("xMin").value),
    xMax: parseFloat(document.getElementById("xMax").value),
  };
}

function render() {
  clear(ctx, canvas.width, canvas.height);
  if (!currentField || !ranges) return;

  transform = makeTransform({
    xMin: ranges.tMin, xMax: ranges.tMax, yMin: ranges.xMin, yMax: ranges.xMax,
    width: canvas.width, height: canvas.height, padding: 30,
  });

  drawAxes(ctx, transform, { xMin: ranges.tMin, xMax: ranges.tMax, yMin: ranges.xMin, yMax: ranges.xMax });
  drawVectorField(ctx, transform, currentField.samples, { arrowLength: 10 });

  if (isoclinePoints.length) {
    const pts = isoclinePoints.map((p) => [p.axis1, p.axis2]);
    drawDots(ctx, transform, pts, { color: "#dc2626", radius: 2 });
  }

  if (currentField.has_invalid_samples) {
    ctx.fillStyle = "#f59e0b";
    ctx.font = "12px sans-serif";
    ctx.fillText("⚠ some grid points were undefined", 10, 16);
  }
}

async function plot() {
  clearError();
  const { expression, params, tMin, tMax, xMin, xMax } = getFormValues();
  ranges = { tMin, tMax, xMin, xMax };
  isoclinePoints = [];
  try {
    currentField = await slopeField({ expression, params, tMin, tMax, nT: 26, xMin, xMax, nX: 46 });
    render();
  } catch (e) {
    showError(e.message);
  }
}

async function placeIsoclineAt(clickT, clickX) {
  const { expression, params, tMin, tMax, xMin, xMax } = getFormValues();
  const token = ++requestToken;
  try {
    const points = await slopeFieldIsocline({
      expression, params, tMin, tMax, nT: 26, xMin, xMax, nX: 46, clickT, clickX,
    });
    if (token !== requestToken) return; // a newer drag position superseded this
    isoclinePoints = points;
    clearError();
    render();
  } catch (e) {
    if (token === requestToken) showError(e.message);
  }
}

function handlePointerEvent(evt) {
  if (!transform) return;
  const rect = canvas.getBoundingClientRect();
  const px = evt.clientX - rect.left;
  const py = evt.clientY - rect.top;
  const [t, x] = transform.toMath(px, py);
  placeIsoclineAt(t, x);
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
