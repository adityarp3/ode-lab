// Thin fetch wrappers around the FastAPI server. Every function here
// builds the exact request body server.py expects (field names match its
// Pydantic models one-for-one) and returns the parsed JSON response.
// No DOM access — testable in plain Node with a mocked global.fetch,
// see tests/api.test.mjs.

export const DEFAULT_BASE_URL = "http://localhost:8000";

export async function postJSON(path, body, baseUrl = DEFAULT_BASE_URL) {
  const res = await fetch(`${baseUrl}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) {
    let detail = res.statusText;
    try {
      const err = await res.json();
      detail = err.detail || detail;
    } catch {
      /* body wasn't JSON; fall back to statusText */
    }
    throw new Error(detail);
  }
  return res.json();
}

export function solveOde(
  { expressions, params = {}, x0, t0 = 0, t1, dt = 0.01, method = "rk4" },
  baseUrl
) {
  return postJSON("/api/solve", { expressions, params, x0, t0, t1, dt, method }, baseUrl);
}

export function phaseLine({ expression, params = {}, xMin, xMax, nSamples = 500 }, baseUrl) {
  return postJSON(
    "/api/phase-line",
    { expression, params, x_min: xMin, x_max: xMax, n_samples: nSamples },
    baseUrl
  );
}

export function phaseLineParticle(
  { expression, params = {}, clickX, tMax = 20, dt = 0.01 },
  baseUrl
) {
  return postJSON(
    "/api/phase-line/particle",
    { expression, params, click_x: clickX, t_max: tMax, dt },
    baseUrl
  );
}

export function slopeField(
  { expression, params = {}, tMin, tMax, nT, xMin, xMax, nX },
  baseUrl
) {
  return postJSON(
    "/api/slope-field",
    { expression, params, t_min: tMin, t_max: tMax, n_t: nT, x_min: xMin, x_max: xMax, n_x: nX },
    baseUrl
  );
}

export function slopeFieldIsocline(
  { expression, params = {}, tMin, tMax, nT, xMin, xMax, nX, clickT, clickX },
  baseUrl
) {
  return postJSON(
    "/api/slope-field/isocline",
    {
      expression, params,
      t_min: tMin, t_max: tMax, n_t: nT,
      x_min: xMin, x_max: xMax, n_x: nX,
      click_t: clickT, click_x: clickX,
    },
    baseUrl
  );
}

export function directionField2D(
  { expressions, params = {}, xMin, xMax, nX, yMin, yMax, nY },
  baseUrl
) {
  return postJSON(
    "/api/direction-field",
    { expressions, params, x_min: xMin, x_max: xMax, n_x: nX, y_min: yMin, y_max: yMax, n_y: nY },
    baseUrl
  );
}

export function directionFieldIsocline(
  { expressions, params = {}, xMin, xMax, nX, yMin, yMax, nY, vectorComponent, clickAxis1, clickAxis2 },
  baseUrl
) {
  return postJSON(
    "/api/direction-field/isocline",
    {
      expressions, params,
      x_min: xMin, x_max: xMax, n_x: nX,
      y_min: yMin, y_max: yMax, n_y: nY,
      vector_component: vectorComponent,
      click_axis1: clickAxis1, click_axis2: clickAxis2,
    },
    baseUrl
  );
}

export function directionFieldTrajectory(
  { expressions, params = {}, clickX, clickY, tMax = 20, dt = 0.01 },
  baseUrl
) {
  return postJSON(
    "/api/direction-field/trajectory",
    { expressions, params, click_x: clickX, click_y: clickY, t_max: tMax, dt },
    baseUrl
  );
}

export function directionFieldSlice(
  { expressions, params = {}, axis1Min, axis1Max, n1, axis2Min, axis2Max, n2, fixedState },
  baseUrl
) {
  return postJSON(
    "/api/direction-field/slice",
    {
      expressions, params,
      axis1_min: axis1Min, axis1_max: axis1Max, n1,
      axis2_min: axis2Min, axis2_max: axis2Max, n2,
      fixed_state: fixedState,
    },
    baseUrl
  );
}
