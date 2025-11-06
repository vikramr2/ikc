from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
import sys
import os

class get_pybind_include:
    """Helper class to determine the pybind11 include path"""
    def __str__(self):
        import pybind11
        return pybind11.get_include()

ext_modules = [
    Extension(
        '_ikc',
        sources=['python/bindings.cpp'],
        include_dirs=[
            str(get_pybind_include()),
            'lib',
        ],
        language='c++',
        extra_compile_args=['-std=c++17', '-O3', '-fopenmp'],
        extra_link_args=['-fopenmp'],
    ),
]

setup(
    name="ikc",
    version="0.1.0",
    description="Python bindings for Iterative K-Core Clustering (IKC)",
    author="",
    package_dir={"": "python"},
    packages=find_packages(where="python"),
    ext_modules=ext_modules,
    install_requires=[
        "pybind11>=2.6.0",
    ],
    setup_requires=[
        "pybind11>=2.6.0",
    ],
    python_requires=">=3.7",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
    zip_safe=False,
)
