# Overview
CAKE is a systemC library for simulating matrix multiplication on 3D architectures.

## Installation

```bash
git clone https://github.com/vnatesh/CAKE.git
cd CAKE
make install
export LD_LIBRARY_PATH=include/systemc-2.3.1a/lib-linux64/
export SYSTEMC_HOME=include/systemc-2.3.1a/
```

Installation automatically downloads and installs the following tool/dependency verions:

* `gcc` - 4.8.5 (with C++11)
* `systemc` - 2.3.1a
* `matchlib` 
* `matchlib connections`
* `make`
* `python` 



## Usage
See [wiki](https://github.com/vnatesh/maestro/wiki) for usage.

<p align = "center">
<img  src="https://github.com/vnatesh/maestro/blob/master/images/cake_diagram.png" width="500">
</p>
