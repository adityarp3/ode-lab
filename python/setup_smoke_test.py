from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

setup(
    name="smoke_test",
    version="0.0.1",
    ext_modules=[Pybind11Extension("smoke_test", ["smoke_test.cpp"], cxx_std=17)],
    cmdclass={"build_ext": build_ext},
)
