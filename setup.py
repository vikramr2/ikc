from setuptools import setup, Extension, find_packages
import sys
import os

class get_pybind_include:
    """Helper class to determine the pybind11 include path"""
    def __str__(self):
        import pybind11
        return pybind11.get_include()

# Configure OpenMP flags based on platform
extra_compile_args = ['-std=c++17', '-O3']
extra_link_args = []
include_dirs = [str(get_pybind_include()), 'lib']
library_dirs = []
libraries = []

if sys.platform == 'darwin':
    # macOS - use libomp from Homebrew
    omp_prefix = '/usr/local/opt/libomp'
    if os.path.exists(omp_prefix):
        extra_compile_args += ['-Xpreprocessor', '-fopenmp']
        include_dirs.append(f'{omp_prefix}/include')
        library_dirs.append(f'{omp_prefix}/lib')
        libraries.append('omp')
    else:
        print("Warning: libomp not found. Install with: brew install libomp")
else:
    # Linux and other platforms
    extra_compile_args.append('-fopenmp')
    extra_link_args.append('-fopenmp')

ext_modules = [
    Extension(
        '_ikc',
        sources=['python/bindings.cpp'],
        include_dirs=include_dirs,
        library_dirs=library_dirs,
        libraries=libraries,
        language='c++',
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    ),
]

# Read the long description from README
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="ikc",
    version="0.3.0",
    description="Fast C++ implementation of Iterative K-Core Clustering with Python bindings",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Vikram Ramavarapu",
    author_email="vikramr2@illinois.edu",
    url="https://github.com/vikramr2/ikc",
    license="MIT",
    package_dir={"": "python"},
    packages=find_packages(where="python"),
    ext_modules=ext_modules,
    install_requires=[
        "pybind11>=2.6.0",
        "tqdm>=4.0.0",
    ],
    setup_requires=[
        "pybind11>=2.6.0",
    ],
    python_requires=">=3.7",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Scientific/Engineering",
    ],
    keywords="graph clustering k-core community-detection network-analysis",
    project_urls={
        "Bug Reports": "https://github.com/vikramr2/ikc/issues",
        "Source": "https://github.com/vikramr2/ikc",
    },
    zip_safe=False,
)
