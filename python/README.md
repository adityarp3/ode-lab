# odelab Python server

pybind11 bindings + FastAPI wrapper around the C++ core. Every endpoint
returns the exact JSON string the C++ side's `to_json()` builds.

> **Heads up:** I could not build or test any of this myself — my sandbox's
> network access was down when I wrote it (couldn't fetch pybind11 to even
> attempt a compile). Everything here is written carefully against the
> actual C++ header signatures, but it is **unverified**, unlike the rest of
> this project. Go through the steps below **in order** and report back
> what happens at each one — that's exactly the approach that made the
> earlier MinGW linker issue fast to fix instead of slow.

## Step 0: use a consistent toolchain

Use MSYS2 UCRT64's own Python, not a separately-installed python.org one —
this avoids the ABI-mismatch class of problems (MSVC-built CPython vs.
MinGW-built extension) entirely, since everything comes from the same
compiler family.

In your **MSYS2 UCRT64** terminal:
```bash
pacman -S mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-pip
python --version   # confirm this is the UCRT64 python, not a Windows one
```

## Step 1: smoke test — build a trivial pybind11 module FIRST

Don't skip this. If the toolchain has any issue, you want to find out with
a 15-line file, not the real 200-line bindings.

```bash
cd /d/dev/ode-lab/python   # adjust to wherever you synced the project
pip install pybind11 setuptools
python setup_smoke_test.py build_ext --inplace
python -c "import smoke_test; print(smoke_test.add(2, 3))"
```

Expected output: `5`. **Stop here and report back if this fails** — don't
move on to Step 2 until this works, the error will be much clearer here.

## Step 2: build the real bindings

```bash
python setup.py build_ext --inplace
python test_bindings.py
```

`test_bindings.py` exercises every binding function, including the
hardening behavior (malformed expressions, domain errors, divergence) —
expected output ends with `All binding sanity checks passed.`

If this fails, the error message plus which `check(...)` line it died on
will tell us a lot — paste both.

## Step 3: run the server

```bash
pip install fastapi "uvicorn[standard]"
uvicorn server:app --reload --port 8000
```

Then in another terminal:
```bash
curl http://localhost:8000/api/health
# {"status":"ok"}

curl -X POST http://localhost:8000/api/phase-line \
  -H "Content-Type: application/json" \
  -d '{"expression": "r*x*(1-x/K)", "params": {"r": 1.0, "K": 10.0}, "x_min": -2, "x_max": 12, "n_samples": 500}'
```

Or open `http://localhost:8000/docs` for FastAPI's interactive Swagger UI —
you can try every endpoint from the browser without writing curl commands.

## Endpoints

| Endpoint | Tool |
|---|---|
| `POST /api/solve` | t vs x(t) — N-dim, any integrator |
| `POST /api/phase-line` | xdot vs x, with fixed points |
| `POST /api/phase-line/particle` | click-to-drop-a-particle on a phase line |
| `POST /api/slope-field` | t-x slope field (non-autonomous 1D) |
| `POST /api/slope-field/isocline` | click/drag isocline on a slope field |
| `POST /api/direction-field` | 2D phase-plane direction field |
| `POST /api/direction-field/isocline` | click/drag nullcline on a 2D field |
| `POST /api/direction-field/trajectory` | click-to-drop-a-trajectory on a 2D field |
| `POST /api/direction-field/slice` | N-dim slice (e.g. Lorenz x-y at fixed z) |

All bad user expressions (division by zero, sqrt of a negative number,
malformed syntax) come back as **HTTP 400** with the C++ error message in
`detail`, not a 500 — see `call_cpp()` in `server.py`.
