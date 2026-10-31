# kisstypepy

Python3 bindings for [kisstype](https://github.com/EIS-KISS/libkisstype), the EIS data type library of kiss.

## Requirements

* git
* c++20 capable compiler (GCC, CLANG)
* cmake 3.23 or higher
* [pybind11](https://github.com/pybind/pybind11)
* pip

## Compile/Install Procedure

* `git clone --recursive https://uvos.xyz/git/kiss/kisstypepy.git`
* `cd kisstypepy`
* `pip install .`

## Usage

The supported classes are

* `kisstype.Spectra`
	* all members are supported, `loadFromDisk`/`saveToDisk` accept `str` and `pathlib.Path`,
	  `loadFromStream`/`saveToStream` accept any object with a `read()`/`write()` method, such as an
	  open file or an `io.StringIO`
	* `len(spectra)` and `spectra[index]` index the data points, negative indices count from the end
	* `FILE_MAGIC`, `FILE_VERSION_MAJOR`, `FILE_VERSION_MINOR` and `FILE_VERSION_PATCH` expose the
	  file format version
* `kisstype.DataPoint`
	* `im` and `omega` are readable and writable, the arithmetic and comparison operators are mapped
	  to their python counterparts, `abs(point)` is `complexVectorLength()`
* `kisstype.Range`
	* iterable, `len(range)` is `count`, out of range indices raise `IndexError` instead of asserting
* `kisstype.Version`
	* corrisponds to `VersionFixed`

Module functions:

* `kisstype.getVersion()`
	* returns the version of the linked kisstype library as a `kisstype.Version`
	* bound to `eis_get_version()`, since `eis::getVersion()` is declared in `kisstype/type.h` but
	  defined in the global namespace by `eistype.cpp` and therefore has no definition to link
* `kisstype.eisToValarrays(data)`
	* splits a list of `DataPoint` into a `(real, imaginary)` pair of lists
* `kisstype.loadSpectra(path)`
	* convenience alias of `Spectra.loadFromDisk`

Exceptions:

* `kisstype.ParseError`, `kisstype.FileError`
	* corrisponds to `eis::parse_errror` and `eis::file_error`
* a failed label lookup (`Spectra.getLabel`) raises the builtin `KeyError`, `eis::key_error` carries
  no message of its own
* `Range.fromString` raises the builtin `ValueError` for malformed strings

```python
import kisstype as kiss

spectra = kiss.Spectra.loadFromDisk("measurement.eis")
print(spectra.model, len(spectra))
for point in spectra:
	print(point.omega, point.im)

print(kiss.Range(1, 1e5, 25, True).getRangeVector())
```
