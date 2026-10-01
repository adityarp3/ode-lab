import assert from "node:assert/strict";
import * as api from "../api.js";

// Mock global.fetch: records the last call's url/body, and returns a
// canned JSON response. This tests that each wrapper builds the exact
// request shape server.py expects, without needing a live server.
let lastCall = null;
let mockResponse = { ok: true, body: {} };

global.fetch = async (url, options) => {
  lastCall = { url, method: options.method, body: JSON.parse(options.body) };
  return {
    ok: mockResponse.ok,
    statusText: "Error",
    json: async () => mockResponse.body,
  };
};

function resetMock(body = {}) {
  lastCall = null;
  mockResponse = { ok: true, body };
}

async function run() {
  // --- solveOde ---
  resetMock({ t: [0, 1], x: [[1], [0.5]], diverged: false });
  await api.solveOde({ expressions: ["-x"], params: {}, x0: [1.0], t0: 0, t1: 1, dt: 0.5, method: "rk4" });
  assert.equal(lastCall.url, "http://localhost:8000/api/solve");
  assert.deepEqual(lastCall.body, {
    expressions: ["-x"], params: {}, x0: [1.0], t0: 0, t1: 1, dt: 0.5, method: "rk4",
  });
  console.log("[OK] solveOde builds correct request body");

  // --- phaseLine ---
  resetMock({ x: [], xdot: [], fixed_points: [] });
  await api.phaseLine({ expression: "r*x*(1-x/K)", params: { r: 1, K: 10 }, xMin: -2, xMax: 12, nSamples: 500 });
  assert.equal(lastCall.url, "http://localhost:8000/api/phase-line");
  assert.deepEqual(lastCall.body, {
    expression: "r*x*(1-x/K)", params: { r: 1, K: 10 }, x_min: -2, x_max: 12, n_samples: 500,
  });
  console.log("[OK] phaseLine builds correct request body (camelCase -> snake_case)");

  // --- phaseLineParticle ---
  resetMock({ t: [], x: [] });
  await api.phaseLineParticle({ expression: "x - x^3", clickX: 0.6, tMax: 20, dt: 0.01 });
  assert.deepEqual(lastCall.body, {
    expression: "x - x^3", params: {}, click_x: 0.6, t_max: 20, dt: 0.01,
  });
  console.log("[OK] phaseLineParticle builds correct request body (default params={})");

  // --- slopeField ---
  resetMock({ samples: [] });
  await api.slopeField({ expression: "t - x", tMin: 0, tMax: 5, nT: 26, xMin: -3, xMax: 6, nX: 46 });
  assert.deepEqual(lastCall.body, {
    expression: "t - x", params: {}, t_min: 0, t_max: 5, n_t: 26, x_min: -3, x_max: 6, n_x: 46,
  });
  console.log("[OK] slopeField builds correct request body");

  // --- slopeFieldIsocline ---
  resetMock([]);
  await api.slopeFieldIsocline({
    expression: "x", tMin: 0, tMax: 5, nT: 6, xMin: -2, xMax: 2, nX: 9, clickT: 2.0, clickX: 1.0,
  });
  assert.deepEqual(lastCall.body, {
    expression: "x", params: {}, t_min: 0, t_max: 5, n_t: 6, x_min: -2, x_max: 2, n_x: 9,
    click_t: 2.0, click_x: 1.0,
  });
  console.log("[OK] slopeFieldIsocline builds correct request body");

  // --- directionField2D ---
  resetMock({ samples: [] });
  await api.directionField2D({
    expressions: ["y", "mu*(1-x^2)*y - x"], params: { mu: 1.5 },
    xMin: -4, xMax: 4, nX: 41, yMin: -4, yMax: 4, nY: 41,
  });
  assert.deepEqual(lastCall.body, {
    expressions: ["y", "mu*(1-x^2)*y - x"], params: { mu: 1.5 },
    x_min: -4, x_max: 4, n_x: 41, y_min: -4, y_max: 4, n_y: 41,
  });
  console.log("[OK] directionField2D builds correct request body");

  // --- directionFieldIsocline ---
  resetMock([]);
  await api.directionFieldIsocline({
    expressions: ["-y", "x"], xMin: -2, xMax: 2, nX: 21, yMin: -2, yMax: 2, nY: 21,
    vectorComponent: 0, clickAxis1: 1.0, clickAxis2: 0.0,
  });
  assert.deepEqual(lastCall.body, {
    expressions: ["-y", "x"], params: {}, x_min: -2, x_max: 2, n_x: 21, y_min: -2, y_max: 2, n_y: 21,
    vector_component: 0, click_axis1: 1.0, click_axis2: 0.0,
  });
  console.log("[OK] directionFieldIsocline builds correct request body");

  // --- directionFieldTrajectory ---
  resetMock({ t: [], x: [] });
  await api.directionFieldTrajectory({
    expressions: ["y", "1.5*(1-x^2)*y - x"], clickX: 0.1, clickY: 0.1, tMax: 30, dt: 0.01,
  });
  assert.deepEqual(lastCall.body, {
    expressions: ["y", "1.5*(1-x^2)*y - x"], params: {}, click_x: 0.1, click_y: 0.1, t_max: 30, dt: 0.01,
  });
  console.log("[OK] directionFieldTrajectory builds correct request body");

  // --- directionFieldSlice ---
  resetMock({ samples: [] });
  await api.directionFieldSlice({
    expressions: ["sigma*(y-x)", "x*(rho-z)-y", "x*y-beta*z"],
    params: { sigma: 10, rho: 28, beta: 8 / 3 },
    axis1Min: -25, axis1Max: 25, n1: 31, axis2Min: -30, axis2Max: 30, n2: 31,
    fixedState: [25.0],
  });
  assert.deepEqual(lastCall.body, {
    expressions: ["sigma*(y-x)", "x*(rho-z)-y", "x*y-beta*z"],
    params: { sigma: 10, rho: 28, beta: 8 / 3 },
    axis1_min: -25, axis1_max: 25, n1: 31, axis2_min: -30, axis2_max: 30, n2: 31,
    fixed_state: [25.0],
  });
  console.log("[OK] directionFieldSlice builds correct request body");

  // --- Error handling: non-ok response with a FastAPI-style {detail: ...} ---
  resetMock({ detail: "Division by zero in expression" });
  mockResponse.ok = false;
  let threw = false;
  try {
    await api.phaseLine({ expression: "1/x", xMin: -1, xMax: 1, nSamples: 10 });
  } catch (e) {
    threw = true;
    assert.equal(e.message, "Division by zero in expression");
  }
  assert.ok(threw, "expected postJSON to throw on a non-ok response");
  console.log("[OK] postJSON surfaces the FastAPI error detail message on failure");

  console.log("\nAll api.js tests passed.");
}

run();
