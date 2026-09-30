# ode-lab

C++ core for numerical ODE tools — Euler and RK4 integrators, plus the
classic 18.03 / Strogatz nonlinear-dynamics analysis tools: t-vs-x(t),
xdot-vs-x phase lines, and direction fields/isoclines. Includes a math
expression parser so any tool can take user-typed equations (e.g.
`"r*x*(1-x/K)"`), interactive click/drag support, and a Python (FastAPI)
server layer for the web frontend.

## Requirements

- CMake >= 3.16
- A C++17 compiler (g++, clang++, or MSVC)

## Build

**Linux/macOS:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

**Windows with MinGW-w64:** CMake defaults to a Visual Studio generator on
Windows if it finds one, which conflicts with g++, so specify the generator
explicitly:
```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```
(If `"MinGW Makefiles"` errors out, try `"MSYS Makefiles"` instead — depends
on whether you're in a plain MinGW64 shell or an MSYS2 shell.) Binaries will
be `.exe` files, e.g. `build/odelab_cli.exe`.

### No CMake available

If `cmake` isn't on your PATH but `g++` works, skip CMake entirely and use
the **"g++: build all (no cmake)"** VSCode task (Ctrl+Shift+B), or run this
directly in a bash-like MinGW64/MSYS2 terminal:

```bash
mkdir -p build
for f in state euler rk4 solver phase_line expression direction_field io; do
  g++ -std=c++17 -Iinclude -Wall -Wextra -g -c src/$f.cpp -o build/$f.o
done
g++ -std=c++17 -Iinclude build/*.o apps/cli/main.cpp -o build/odelab_cli.exe
for t in test_core test_phase_line test_expression test_direction_field test_hardening; do
  g++ -std=c++17 -Iinclude build/*.o tests/$t.cpp -o build/$t.exe
done
```

To install CMake for later (so IntelliSense's `compile_commands.json` and
CTest also work): `pacman -S mingw-w64-x86_64-cmake` in an MSYS2 shell, or
`winget install Kitware.CMake` (restart your terminal after).

This produces:
- `build/odelab_cli` — demo: solves logistic growth with Euler and RK4,
  compares against the analytic solution.
- `build/test_core`, `test_phase_line`, `test_expression`, `test_direction_field`,
  `test_hardening` — the full correctness/regression suite.
- `build/logistic_phase_line`, `bistable_phase_line`, `slope_field_demo`,
  `van_der_pol`, `predator_prey`, `lorenz` (via CMake, or compile
  individually with the same pattern as above using `examples/*.cpp`) — the
  showcase examples, each writing CSVs to `output/`.

## Run

```bash
./build/odelab_cli        # Linux/macOS
./build/odelab_cli.exe    # Windows/MinGW
```

## Test

Either run the test binaries directly:

```bash
./build/test_core          # or test_core.exe on Windows
./build/test_phase_line    # or test_phase_line.exe on Windows
```

or use CTest to run everything and get a pass/fail summary:

```bash
cd build && ctest --output-on-failure
```

## VSCode

Open this folder in VSCode. Install the **C/C++** extension (and optionally
**CMake Tools**). Included tasks (Cmd/Ctrl+Shift+B for the default build):

- **CMake: configure** — runs `cmake -S . -B build`
- **CMake: build** — runs `cmake --build build` (default build task)
- **Run: odelab_cli** — builds then runs the CLI demo
- **Run: all tests** — builds then runs `ctest`

Run any of these from the Command Palette → "Tasks: Run Task".

`CMAKE_EXPORT_COMPILE_COMMANDS` is enabled, so once you've configured once,
IntelliSense (or clangd) will pick up `build/compile_commands.json`
automatically and resolve the `odelab/*.hpp` includes correctly.

## Python server layer

`python/` has pybind11 bindings + a FastAPI server exposing the C++ core
over HTTP. **See `python/README.md` before touching this** — it was
written without the ability to compile/test it (sandbox network was down),
so it walks through a staged build (smoke test first, then the real
bindings, then the server) specifically so any issue is caught early and
is easy to diagnose.

## Project layout

```
include/odelab/   — public headers: State, Integrator, Euler, RK4, Solver,
                     PhaseLine, DirectionField, Expression (parser), IO (CSV+JSON)
src/               — implementations
apps/cli/          — CLI demo entry point
tests/             — correctness/regression tests (registered with CTest)
examples/          — showcase: 1D (logistic, bistable), non-autonomous slope
                     field, 2D (Van der Pol, predator-prey), 3D (Lorenz)
python/            — pybind11 bindings + FastAPI server (see python/README.md)
```

## Interactive tools

Each tool has a corresponding way to support click/drag in a frontend,
without any extra backend work beyond what's already here:

- **t vs x(t)**: drag a start point → call `solve()` again with the new `x0`.
- **Phase line**: click → `to_ode_func(ScalarField1D)` adapts it into `solve()`
  so a "particle" flows to its fixed point.
- **2D direction field**: click → drop a trajectory, same `solve()` as above.
- **Isoclines/nullclines**: click/drag → `evaluate_component()` gets the exact
  field value at the clicked point, `isocline_through_point()` derives the
  level curve from the already-computed grid (no recomputation needed).
