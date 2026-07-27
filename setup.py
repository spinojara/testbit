from setuptools import setup, find_packages

setup(
    name="testbit",
    version="2.0",
    scripts=["bin/testbit", "bin/testbitb"],
    packages=find_packages(),
    author="Isak Ellmer",
    url="https://github.com/spinojara/testbit",
    install_requires=[
        "requests",
        "urllib3",
    ],
    python_requires=">=3.14",
)
