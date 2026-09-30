"""
Build the odelab_py pybind11 extension.

Usage:
    cd python
    pip install pybind11 setuptools
    python setup.py build_ext --inplace

IMPORTANT: build and import smoke_test.cpp (via setup_smoke_test.py) FIRST.
If that doesn't work, fix that before touching this file — see the note at
the top of bindings.cpp for why.
"""
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import pathlib

# Project root is one level up from this file (python/setup.py -> ode-lab/).
ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

sources = [
    str(pathlib.Path(__file__).parent / "bindings.cpp"),
    str(SRC / "state.cpp"),
    str(SRC / "euler.cpp"),
    str(SRC / "rk4.cpp"),
    str(SRC / "solver.cpp"),
    str(SRC / "phase_line.cpp"),
    str(SRC / "expression.cpp"),
    str(SRC / "direction_field.cpp"),
    str(SRC / "io.cpp"),
]

ext_modules = [
    Pybind11Extension(
        "odelab_py",
        sources,
        include_dirs=[str(ROOT / "include")],
        cxx_std=17,
    ),
]

setup(
    name="odelab_py",
    version="0.1.0",
    description="Python bindings for the odelab C++ ODE toolkit",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
