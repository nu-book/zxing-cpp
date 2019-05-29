#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <string>
#include "BarcodeFormat.h"
#include "DecodeHints.h"
#include "MultiFormatReader.h"
#include "GenericLuminanceSource.h"
#include "HybridBinarizer.h"
#include "Result.h"
#include "TextUtfEncoding.h"

using namespace ZXing;

#if 0
using Binarizer = GlobalHistogramBinarizer;
#else
using Binarizer = HybridBinarizer;
#endif

namespace py = pybind11;


// Note: Image is a Numpy array in BGR format (the default opencv format)
using Image = py::array_t<uint8_t, py::array::c_style>;

Result decode(const Image& image, bool fastMode, bool tryRotate, BarcodeFormat format) {
	DecodeHints hints;
	hints.setShouldTryHarder(!fastMode);
	hints.setShouldTryRotate(tryRotate);
	if (format != BarcodeFormat::FORMAT_COUNT) {
		hints.setPossibleFormats({ format });
	}
	MultiFormatReader reader(hints);
	const auto height = image.shape(0);
	const auto width = image.shape(1);
	const auto channels = image.shape(2);
	const auto bytes = image.data();
	GenericLuminanceSource source(width, height, bytes, width*channels, channels, 2, 1, 0);
	Binarizer binImage(std::shared_ptr<LuminanceSource>(&source, [](void*) {}));
	return reader.read(binImage);
}

PYBIND11_MODULE(zxingcpp, m) {
	m.doc() = "python bindings for zxingcpp";
	py::enum_<BarcodeFormat>(m, "BarcodeFormat")
		.value("AZTEC", BarcodeFormat::AZTEC)
		.value("CODABAR", BarcodeFormat::CODABAR)
		.value("CODE_39", BarcodeFormat::CODE_39)
		.value("CODE_93", BarcodeFormat::CODE_93)
		.value("CODE_128", BarcodeFormat::CODE_128)
		.value("DATA_MATRIX", BarcodeFormat::DATA_MATRIX)
		.value("EAN_8", BarcodeFormat::EAN_8)
		.value("EAN_13", BarcodeFormat::EAN_13)
		.value("ITF", BarcodeFormat::ITF)
		.value("MAXICODE", BarcodeFormat::MAXICODE)
		.value("PDF_417", BarcodeFormat::PDF_417)
		.value("QR_CODE", BarcodeFormat::QR_CODE)
		.value("RSS_14", BarcodeFormat::RSS_14)
		.value("RSS_EXPANDED", BarcodeFormat::RSS_EXPANDED)
		.value("UPC_A", BarcodeFormat::UPC_A)
		.value("UPC_E", BarcodeFormat::UPC_E)
		.value("UPC_EAN_EXTENSION", BarcodeFormat::UPC_EAN_EXTENSION)
		.value("FORMAT_COUNT", BarcodeFormat::FORMAT_COUNT)
		.export_values();
	py::class_<Result>(m, "Result")
		.def_property_readonly("valid", &Result::isValid)
		.def_property_readonly("text", &Result::text)
		.def_property_readonly("format", &Result::format);
	m.def("decode", &decode, "Decode a barcode from a numpy BGR image array",
		py::arg("image"),
		py::arg("fastMode")=false,
		py::arg("tryRoate")=true,
		py::arg("format")=BarcodeFormat::FORMAT_COUNT
	);
}
