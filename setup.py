from setuptools import setup, find_packages

setup(
    name="ikc",
    version="0.1.0",
    description="Python wrapper for Iterative K-Core Clustering (IKC)",
    author="",
    package_dir={"": "python"},
    packages=find_packages(where="python"),
    install_requires=[
        "pandas>=1.0.0",
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
)
