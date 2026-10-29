import io
import pathlib

import pytest

import kisstype as kiss


def test_datapoint():
	point = kiss.DataPoint(complex(10, -2), 1000)
	assert point.im == complex(10, -2)
	assert point.omega == 1000
	assert (point + point).im.real == 20
	assert (point - point).im.real == 0
	assert (point * 2).im.real == 20
	assert (point / 2).im.real == 5
	assert abs(point) == pytest.approx(point.complexVectorLength())
	assert point < kiss.DataPoint(complex(0, 0), 2000)
	assert point > kiss.DataPoint(complex(0, 0), 5)
	assert kiss.DataPoint(complex(1, 2)) == kiss.DataPoint(complex(1, 2))
	assert "DataPoint(" in repr(point)


def test_range():
	rng = kiss.Range(1, 1e5, 5, True)
	assert len(rng) == 5
	assert rng[0] == 1
	assert rng[-1] == pytest.approx(1e5)
	assert list(rng) == rng.getRangeVector()
	assert rng.at(2) == rng[2]
	assert rng.getString(True) == "1~100000L:5"
	assert rng.isSane()
	assert not kiss.Range(5, 1, 3).isSane()
	assert (rng * 2).end == pytest.approx(2e5)
	assert (rng / 2).end == pytest.approx(5e4)
	with pytest.raises(IndexError):
		rng[99]


def test_range_parsing():
	assert kiss.Range.fromString("1~100000L:5").log is True
	assert kiss.Range.fromString("1~100000L:5").count == 5
	assert len(kiss.Range.rangesFromParamString("1~2,3~4", 5)) == 2
	with pytest.raises(ValueError):
		kiss.Range.fromString("gibberish~foo")


def test_spectra_labels():
	spectra = kiss.Spectra([kiss.DataPoint(complex(1, -1), 10)], "r{1e2}c{1e-6}", "header")
	spectra.headerDescription = "description"
	spectra.addLabel("temperature", 298.15)
	assert spectra.hasLabel("temperature")
	assert spectra.getLabel("temperature") == 298.15
	spectra.setLabels([1, 2, 3])
	assert spectra.getFvalueLabels() == [1, 2, 3]
	spectra.setSzLabels([4, 5])
	assert spectra.getSzLabels() == [4, 5]
	spectra.setLabel(1, 3)
	assert spectra.labels == [0, 1, 0]
	assert not spectra.isMulticlass()
	spectra.setLabels([1, 1])
	assert spectra.isMulticlass()
	with pytest.raises(KeyError):
		spectra.getLabel("does not exist")


def test_spectra_round_trip(tmp_path):
	spectra = kiss.Spectra(
		[kiss.DataPoint(complex(10, -2), 1000), kiss.DataPoint(complex(20, -8), 100)],
		"r{1e1}c{1e-6}", "header text")
	spectra.headerDescription = "description"

	path = tmp_path / "spectra.eis"
	spectra.saveToDisk(path)
	assert kiss.Spectra.loadFromDisk(path).model == spectra.model
	assert kiss.loadSpectra(str(path)).header == "header text"

	loaded = kiss.Spectra.loadFromDisk(path, removeDuplicates=False)
	assert len(loaded) == 2
	assert loaded[0].omega == 1000
	assert loaded[-1].omega == 100
	assert len(spectra) == 2

	header = kiss.Spectra.loadHeaderFromDisk(pathlib.Path(path))
	assert header.model == spectra.model
	assert len(header.data) == 0

	buffer = io.StringIO()
	spectra.saveToStream(buffer)
	buffer.seek(0)
	assert len(kiss.Spectra.loadFromStream(buffer)) == 2
	buffer.seek(0)
	assert len(kiss.Spectra.loadHeaderFromStream(buffer)) == 0

	assert kiss.Spectra.FILE_MAGIC == "EISF"
	assert spectra.model in repr(spectra)


def test_errors(tmp_path):
	with pytest.raises(kiss.FileError):
		kiss.Spectra.loadFromDisk(tmp_path / "does-not-exist.eis")


def test_valarrays():
	data = [kiss.DataPoint(complex(10, -2), 1000), kiss.DataPoint(complex(20, -8), 100)]
	real, imag = kiss.eisToValarrays(data)
	assert real == [10, 20]
	assert imag == [-2, -8]


def test_version():
	version = kiss.getVersion()
	assert version.major >= 1
	assert str(version).count('.') == 2
	assert kiss.__version__


def test_importable_with_eisgenerator():
	"""
	Both packages wrap the same C++ types, importing them in one interpreter must not abort on a
	duplicate pybind11 type registration.
	"""
	eis = pytest.importorskip("eisgenerator")

	model = eis.Model("r{1e2}c{1e-6}")
	sweep = model.executeSweep(eis.Range(1, 1e5, 10, True))
	assert len(sweep) == 10

	points = [kiss.DataPoint(point.im, point.omega) for point in sweep]
	spectra = kiss.Spectra(points, model.getModelStr(), "generated with eisgenerator")
	assert len(spectra) == 10

	# Both sets of types still behave for their own module.
	assert isinstance(sweep[0], eis.DataPoint)
	assert isinstance(points[0], kiss.DataPoint)
	assert model.execute(1000).omega == 1000
