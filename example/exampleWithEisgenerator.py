#!/bin/python

"""
kisstype and eisgenerator wrap the same C++ types, but each extension module registers them for
itself, so both packages can be imported in the same interpreter.
"""

import eisgenerator as eis
import kisstype as kiss

print(f"eisgenerator {eis.__version__} and kisstype {kiss.__version__} loaded side by side")

model = eis.Model("r{1e2}c{1e-6}")
sweep = model.executeSweep(eis.Range(1, 1e5, 10, True))
print(f"eisgenerator produced {len(sweep)} {type(sweep[0]).__name__} points")

# The two packages have their own python types for the same C++ types, so points are handed over
# explicitly.
points = [kiss.DataPoint(point.im, point.omega) for point in sweep]
spectra = kiss.Spectra(points, model.getModelStr(), "generated with eisgenerator")
spectra.saveToDisk("/tmp/both.eis")

reloaded = kiss.Spectra.loadFromDisk("/tmp/both.eis")
print(f"kisstype read the file back: {len(reloaded)} points, model {reloaded.model}")

# kisstype's Spectra is what the kiss tooling reads and writes, eisgenerator's Model is what
# generates data, so a typical workflow uses both at the same time.
print(f"omega of the highest frequency point: {reloaded[-1].omega}")
