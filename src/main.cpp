//SPDX-License-Identifier:         MIT

#include <pybind11/pybind11.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <kisstype/c/type.h>
#include <kisstype/spectra.h>
#include <kisstype/type.h>

#include <cmath>
#include <complex>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace py = pybind11;

namespace
{

/**
 * @brief Formats a complex impedance like python formats complex numbers.
 *
 * @return The formatted impedance, i.e. "(1.000000e+00+2.000000e+00j)".
 */
std::string complexToStr(const std::complex<fvalue>& im)
{
	std::stringstream ss;
	ss<<std::scientific<<im.real()<<(std::signbit(im.imag()) ? '-' : '+')<<std::fabs(im.imag())<<'j';
	return ss.str();
}

std::string reprDataPoint(const eis::DataPoint& dp)
{
	std::stringstream ss;
	ss<<std::scientific<<"DataPoint(im="<<complexToStr(dp.im)<<", omega="<<dp.omega<<')';
	return ss.str();
}

std::string reprRange(const eis::Range& range)
{
	std::stringstream ss;
	ss<<std::scientific<<"Range(start="<<range.start<<", end="<<range.end<<", count="<<range.count
		<<", log="<<(range.log ? "True" : "False")<<')';
	return ss.str();
}

std::string reprSpectra(const eis::Spectra& spectra)
{
	std::stringstream ss;
	spectra.saveToStream(ss);
	return ss.str();
}

std::string strVersion(const VersionFixed& version)
{
	return std::to_string(version.major)+'.'+std::to_string(version.minor)+'.'+std::to_string(version.patch);
}

/**
 * @brief Bounds checked variant of eis::Range::at.
 *
 * The C++ function asserts on an out of range index which would abort the interpreter, so the
 * bounds are checked here and an IndexError raised instead. Negative indices count from the end.
 */
fvalue rangeAt(const eis::Range& range, py::ssize_t index)
{
	if(index < 0)
		index += static_cast<py::ssize_t>(range.count);
	if(index < 0 || static_cast<size_t>(index) >= range.count)
		throw py::index_error("range index out of range");
	return range.at(static_cast<size_t>(index));
}

/**
 * @brief Indexes the data of a spectra, negative indices count from the end.
 */
eis::DataPoint spectraAt(const eis::Spectra& spectra, py::ssize_t index)
{
	if(index < 0)
		index += static_cast<py::ssize_t>(spectra.data.size());
	if(index < 0 || static_cast<size_t>(index) >= spectra.data.size())
		throw py::index_error("spectra index out of range");
	return spectra.data[static_cast<size_t>(index)];
}

/**
 * @brief Writes a spectra into any python object offering a write() method, i.e. a file or io.StringIO.
 */
void spectraSaveToStream(const eis::Spectra& self, py::object stream)
{
	std::ostringstream ss;
	self.saveToStream(ss);
	stream.attr("write")(ss.str());
}

/**
 * @brief Reads a spectra from any python object offering a read() method, i.e. a file or io.StringIO.
 */
eis::Spectra spectraLoadFromStream(py::object stream, bool removeDuplicates)
{
	std::istringstream ss(stream.attr("read")().cast<std::string>());
	return eis::Spectra::loadFromStream(ss, removeDuplicates);
}

/**
 * @brief Reads just the header of a spectra from any python object offering a read() method.
 */
eis::Spectra spectraLoadHeaderFromStream(py::object stream)
{
	std::istringstream ss(stream.attr("read")().cast<std::string>());
	return eis::Spectra::loadHeaderFromStream(ss);
}

/**
 * @brief Splits data points into a (real, imaginary) pair of lists.
 *
 * The C++ function returns std::valarrays, which have no python counterpart, so plain lists are
 * handed out instead.
 */
py::tuple eisToLists(const std::vector<eis::DataPoint>& data)
{
	auto valarrays = eis::eisToValarrays(data);
	std::vector<fvalue> real(valarrays.first.size(), 0);
	std::vector<fvalue> imag(valarrays.second.size(), 0);

	for(size_t i = 0; i < real.size(); ++i)
		real[i] = valarrays.first[i];
	for(size_t i = 0; i < imag.size(); ++i)
		imag[i] = valarrays.second[i];

	return py::make_tuple(real, imag);
}

}

/**
 * @brief Bindings for libkisstype.
 *
 * Every type is registered with py::module_local() and every exception translator with
 * py::register_local_exception(). The very same C++ types are also exported by eisgenerator._core,
 * which registers them globally. Without the module local registration importing both extension
 * modules in one interpreter aborts with "generic_type: type \"DataPoint\" is already registered!".
 * The price is that the two packages expose independent python types for the same C++ type.
 */
PYBIND11_MODULE(_core, m)
{
	m.attr("__version__") = KISSTYPEPY_VERSION;

	py::class_<eis::DataPoint>(m, "DataPoint", py::module_local())
		.def(py::init<std::complex<fvalue>, fvalue>(),
			py::arg("im") = std::complex<fvalue>(0, 0), py::arg("omega") = 0)
		.def_readwrite("im", &eis::DataPoint::im)
		.def_readwrite("omega", &eis::DataPoint::omega)
		.def("complexVectorLength", &eis::DataPoint::complexVectorLength)
		.def("__abs__", &eis::DataPoint::complexVectorLength)
		.def("__lt__", &eis::DataPoint::operator<)
		.def("__gt__", &eis::DataPoint::operator>)
		.def("__eq__", &eis::DataPoint::operator==)
		.def("__add__", &eis::DataPoint::operator+)
		.def("__sub__", &eis::DataPoint::operator-)
		.def("__mul__", static_cast<eis::DataPoint (eis::DataPoint::*)(const eis::DataPoint&) const>(&eis::DataPoint::operator*))
		.def("__mul__", static_cast<eis::DataPoint (eis::DataPoint::*)(fvalue) const>(&eis::DataPoint::operator*))
		.def("__truediv__", static_cast<eis::DataPoint (eis::DataPoint::*)(const eis::DataPoint&) const>(&eis::DataPoint::operator/))
		.def("__truediv__", static_cast<eis::DataPoint (eis::DataPoint::*)(fvalue) const>(&eis::DataPoint::operator/))
		.def("__itruediv__", &eis::DataPoint::operator/=)
		.def("__repr__", &reprDataPoint);

	py::class_<eis::Range>(m, "Range", py::module_local())
		.def(py::init<>())
		.def(py::init<fvalue, fvalue, size_t, bool>(),
			py::arg("start"), py::arg("end"), py::arg("count"), py::arg("log") = false)
		.def_readwrite("start", &eis::Range::start)
		.def_readwrite("end", &eis::Range::end)
		.def_readwrite("count", &eis::Range::count)
		.def_readwrite("step", &eis::Range::step)
		.def_readwrite("log", &eis::Range::log)
		.def("stepSize", &eis::Range::stepSize)
		.def("stepValue", &eis::Range::stepValue)
		.def("center", &eis::Range::center)
		.def("at", &rangeAt, py::arg("index"))
		.def("print", &eis::Range::print)
		.def("getString", &eis::Range::getString, py::arg("withCount") = false)
		.def("isSane", &eis::Range::isSane)
		.def("getRangeVector", &eis::Range::getRangeVector)
		.def_static("fromString", &eis::Range::fromString, py::arg("str"), py::arg("count") = 1)
		.def_static("rangesFromParamString", &eis::Range::rangesFromParamString,
			py::arg("paramStr"), py::arg("count"))
		.def("__len__", [](const eis::Range& self) {return self.count;})
		.def("__getitem__", &rangeAt, py::arg("index"))
		.def("__iter__", [](const eis::Range& self) {return py::iter(py::cast(self.getRangeVector()));})
		.def("__mul__", static_cast<eis::Range (eis::Range::*)(fvalue) const>(&eis::Range::operator*))
		.def("__truediv__", static_cast<eis::Range (eis::Range::*)(fvalue) const>(&eis::Range::operator/))
		.def("__repr__", &reprRange);

	py::class_<eis::Spectra>(m, "Spectra", py::module_local())
		.def(py::init<>())
		.def(py::init<const std::vector<eis::DataPoint>&, const std::string&, const std::string&,
			const std::vector<double>&, const std::vector<std::string>&>(),
			py::arg("data"), py::arg("model") = std::string(), py::arg("header") = std::string(),
			py::arg("labels") = std::vector<double>(), py::arg("labelNames") = std::vector<std::string>())
		.def_readwrite("data", &eis::Spectra::data)
		.def_readwrite("model", &eis::Spectra::model)
		.def_readwrite("headerDescription", &eis::Spectra::headerDescription)
		.def_readwrite("header", &eis::Spectra::header)
		.def_readwrite("labels", &eis::Spectra::labels)
		.def_readwrite("labelNames", &eis::Spectra::labelNames)
		.def_static("loadFromDisk", &eis::Spectra::loadFromDisk, py::arg("path"),
			py::arg("removeDuplicates") = true)
		.def_static("loadHeaderFromDisk", &eis::Spectra::loadHeaderFromDisk, py::arg("path"))
		.def_static("loadFromStream", &spectraLoadFromStream, py::arg("stream"),
			py::arg("removeDuplicates") = true)
		.def_static("loadHeaderFromStream", &spectraLoadHeaderFromStream, py::arg("stream"))
		.def("saveToDisk", &eis::Spectra::saveToDisk, py::arg("path"))
		.def("saveToStream", &spectraSaveToStream, py::arg("stream"))
		.def("hasLabel", &eis::Spectra::hasLabel, py::arg("key"))
		.def("getLabel", &eis::Spectra::getLabel, py::arg("key"))
		.def("addLabel", &eis::Spectra::addLabel, py::arg("key"), py::arg("value"))
		.def("setLabel", &eis::Spectra::setLabel, py::arg("label"), py::arg("maxLabel"))
		.def("setLabels", static_cast<void (eis::Spectra::*)(const std::vector<double>&)>(&eis::Spectra::setLabels),
			py::arg("labels"))
		.def("setLabels", static_cast<void (eis::Spectra::*)(const std::vector<float>&)>(&eis::Spectra::setLabels),
			py::arg("labels"))
		.def("setSzLabels", &eis::Spectra::setSzLabels, py::arg("labels"))
		.def("getSzLabels", &eis::Spectra::getSzLabels)
		.def("getFvalueLabels", &eis::Spectra::getFvalueLabels)
		.def("isMulticlass", &eis::Spectra::isMulticlass)
		.def_property_readonly_static("FILE_MAGIC", [](py::object) {return std::string(eis::Spectra::F_MAGIC);})
		.def_property_readonly_static("FILE_VERSION_MAJOR", [](py::object) {return eis::Spectra::F_VERSION_MAJOR;})
		.def_property_readonly_static("FILE_VERSION_MINOR", [](py::object) {return eis::Spectra::F_VERSION_MINOR;})
		.def_property_readonly_static("FILE_VERSION_PATCH", [](py::object) {return eis::Spectra::F_VERSION_PATCH;})
		.def("__len__", [](const eis::Spectra& self) {return self.data.size();})
		.def("__getitem__", &spectraAt, py::arg("index"))
		.def("__repr__", &reprSpectra);

	py::class_<VersionFixed>(m, "Version", py::module_local())
		.def_readwrite("major", &VersionFixed::major)
		.def_readwrite("minor", &VersionFixed::minor)
		.def_readwrite("patch", &VersionFixed::patch)
		.def("__str__", &strVersion)
		.def("__repr__", [](const VersionFixed& self) {return "Version(" + strVersion(self) + ')';});

	py::register_local_exception<eis::parse_errror>(m, "ParseError");
	py::register_local_exception<eis::file_error>(m, "FileError");
	// eis::key_error carries no message, python's builtin KeyError is the natural counterpart of a
	// failed label lookup, so it is used instead of a dedicated kisstype exception type.
	py::register_local_exception_translator([](std::exception_ptr p)
	{
		try
		{
			if(p)
				std::rethrow_exception(p);
		}
		catch(const eis::key_error&)
		{
			PyErr_SetString(PyExc_KeyError, "this spectra has no label of that name");
		}
	});

	m.def("eisToValarrays", &eisToLists, py::arg("data"),
		"Split a list of DataPoints into a (real, imaginary) pair of lists.");
	// kisstype/type.h declares eis::getVersion(), but eistype.cpp defines the function in the global
	// namespace, so the C entry point is the only version getter that has a definition to link
	// against.
	m.def("getVersion", &eis_get_version, "Returns the version of the linked libkisstype.");

	m.def("loadSpectra", &eis::Spectra::loadFromDisk, py::arg("path"), py::arg("removeDuplicates") = true,
		"Convenience alias of Spectra.loadFromDisk.");
}
