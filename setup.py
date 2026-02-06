from setuptools import setup, find_packages

setup(
    name="testbit",
    version="2.0",
    scripts=["bin/testbit", "bin/testbitd", "bin/testbitn"],
    packages=find_packages(),
    author="Isak Ellmer",
    url="https://github.com/spinojara/nnuebit",
    install_requires=[
        "aiohttp",
        "docker",
    ]
)
