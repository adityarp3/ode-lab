import { makeTransform } from "../js/coords.js";
import { clear, drawAxes, drawCurve } from "../js/canvas-utils.js";
import { solveOde } from "../js/api.js";

const canvasXZ = document.getElementById("canvas-xz");
const ctxXZ = canvasXZ.getContext("2d");
const canvasXY = document.getElementById("canvas-xy");
const ctxXY = canvasXY.getContext("2d");
const errorBanner = document.getElementById("error-banner");

function showError(msg) {
  errorBanner.textContent = msg;
  errorBanner.classList.add("visible");
}
function clearError() {
  errorBanner.classList.remove("visible");
}

function drawProjection(ctx, canvas, points, ranges) {
  clear(ctx, canvas.width, canvas.height);
  const transform = makeTransform({ ...ranges, width: canvas.width, height: canvas.height, padding: 20 });
  drawAxes(ctx, transform, ranges);
  drawCurve(ctx, transform, points, { color: "#7c3aed", lineWidth: 1 });
}

async function plot() {
  clearError();
  const sigma = parseFloat(document.getElementById("sigma").value);
  const rho = parseFloat(document.getElementById("rho").value);
  const beta = parseFloat(document.getElementById("beta").value);
  const x0 = parseFloat(document.getElementById("x0").value);
  const y0 = parseFloat(document.getElementById("y0").value);
  const z0 = parseFloat(document.getElementById("z0").value);

  try {
    const traj = await solveOde({
      expressions: ["sigma*(y - x)", "x*(rho - z) - y", "x*y - beta*z"],
      params: { sigma, rho, beta },
      x0: [x0, y0, z0],
      t0: 0, t1: 40, dt: 0.01,
      method: "rk4",
    });

    if (traj.diverged) {
      showError(`Solution diverged: ${traj.divergence_reason}`);
    }

    const xz = traj.x.map(([x, , z]) => [x, z]);
    const xy = traj.x.map(([x, y]) => [x, y]);

    drawProjection(ctxXZ, canvasXZ, xz, { xMin: -25, xMax: 25, yMin: 0, yMax: 50 });
    drawProjection(ctxXY, canvasXY, xy, { xMin: -25, xMax: 25, yMin: -30, yMax: 30 });
  } catch (e) {
    showError(e.message);
  }
}

document.getElementById("plot-btn").addEventListener("click", plot);

plot();
