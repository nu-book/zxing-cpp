/*
* Copyright 2016 Nu-book Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ReadBarcode.h"
#include "TextUtfEncoding.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <clocale>
#include <cstring>
#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace ZXing;
using namespace TextUtfEncoding;

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath << " [-fast] [-norotate] [-format <FORMAT[,...]>] [-ispure] [-1] <png image path>...\n"
			  << "    -fast      Skip some lines/pixels during detection (faster)\n"
			  << "    -norotate  Don't try rotated image during detection (faster)\n"
			  << "    -format    Only detect given format(s) (faster)\n"
			  << "    -ispure    Assume the image contains only a 'pure'/perfect code (faster)\n"
			  << "    -1         Print only file name, text and status on one line per file\n"
			  << "    -escape    Escape non-graphical characters in angle brackets (ignored for -1 option, which always escapes)\n"
			  << "\n"
			  << "Supported formats are:\n";
	for (auto f : BarcodeFormats::all()) {
		std::cout << "    " << ToString(f) << "\n";
	}
	std::cout << "Formats can be lowercase, with or without '-', separated by ',' and/or '|'\n";
}

static bool ParseOptions(int argc, char* argv[], DecodeHints* hints, bool& oneLine, bool& angleEscape, std::list<std::string>& filePaths)
{
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-fast") == 0) {
			hints->setTryHarder(false);
		}
		else if (strcmp(argv[i], "-norotate") == 0) {
			hints->setTryRotate(false);
		}
		else if (strcmp(argv[i], "-ispure") == 0) {
			hints->setIsPure(true);
			hints->setBinarizer(Binarizer::FixedThreshold);
		}
		else if (strcmp(argv[i], "-format") == 0) {
			if (++i == argc)
				return false;
			try {
				hints->setFormats(BarcodeFormatsFromString(argv[i]));
			} catch (const std::exception& e) {
				std::cerr << e.what() << "\n";
				return false;
			}
		}
		else if (strcmp(argv[i], "-1") == 0) {
			oneLine = true;
		}
		else if (strcmp(argv[i], "-escape") == 0) {
			angleEscape = true;
		}
		else {
			filePaths.push_back(argv[i]);
		}
	}

	return !filePaths.empty();
}

std::ostream& operator<<(std::ostream& os, const Position& points) {
	for (const auto& p : points)
		os << p.x << "x" << p.y << " ";
	return os;
}

int main(int argc, char* argv[])
{
	DecodeHints hints;
	std::list<std::string> filePaths;
	bool oneLine = false;
	bool angleEscape = false;
	int fileCount = 0;
	int ret = 0;

	if (!ParseOptions(argc, argv, &hints, oneLine, angleEscape, filePaths)) {
		PrintUsage(argv[0]);
		return -1;
	}
	if (oneLine) {
		angleEscape = true;
	}

	if (angleEscape) {
		std::setlocale(LC_CTYPE, "en_US.UTF-8"); // Needed for `std::iswgraph()` in `ToUtf8(angleEscape)`
	}

	for (std::string filePath : filePaths) {
		int width, height, channels;
		std::unique_ptr<stbi_uc, void(*)(void*)> buffer(stbi_load(filePath.c_str(), &width, &height, &channels, 4), stbi_image_free);
		if (buffer == nullptr) {
			std::cerr << "Failed to read image: " << filePath << "\n";
			return -1;
		}
		fileCount++;

		const auto& result = ReadBarcode({buffer.get(), width, height, ImageFormat::RGBX}, hints);
		const auto& meta = result.metadata();

		ret |= static_cast<int>(result.status());

		if (oneLine) {
			if (!meta.getString(ResultMetadata::UPC_EAN_EXTENSION).empty()) {
				std::cout << filePath << " \"" << ToUtf8(result.text(), angleEscape)
						  << " " << ToUtf8(meta.getString(ResultMetadata::UPC_EAN_EXTENSION)) << "\" "
						  << ToString(result.status()) << "\n";
			}
			else {
				std::cout << filePath << " \"" << ToUtf8(result.text(), angleEscape) << "\" " << ToString(result.status()) << "\n";
			}
			continue;
		}

		if (filePaths.size() > 1) {
			if (fileCount > 1) {
				std::cout << "\n";
			}
			std::cout << "File:     " << filePath << "\n";
		}
		std::cout << "Text:     \"" << ToUtf8(result.text(), angleEscape) << "\"\n"
				  << "Format:   " << ToString(result.format()) << "\n"
				  << "Position: " << result.position() << "\n"
				  << "Rotation: " << result.orientation() << " deg\n"
				  << "Error:    " << ToString(result.status()) << "\n";

		std::map<ResultMetadata::Key, const char*> keys = {{ResultMetadata::ERROR_CORRECTION_LEVEL, "EC Level: "},
														   {ResultMetadata::POSSIBLE_COUNTRY, "Country:  "}};

		for (auto key : keys) {
			auto value = ToUtf8(meta.getString(key.first));
			if (value.size())
				std::cout << key.second << value << "\n";
		}

		if (!meta.getString(ResultMetadata::UPC_EAN_EXTENSION).empty()) {
			std::cout << "Add-on:   \"" << ToUtf8(meta.getString(ResultMetadata::UPC_EAN_EXTENSION)) << "\"";
			if (!meta.getString(ResultMetadata::ISSUE_NUMBER).empty()) {
				std::cout << " (Issue #" << ToUtf8(meta.getString(ResultMetadata::ISSUE_NUMBER)) << ")";
			}
			else if (!meta.getString(ResultMetadata::SUGGESTED_PRICE).empty()) {
				std::cout << " (Price " << ToUtf8(meta.getString(ResultMetadata::SUGGESTED_PRICE)) << ")";
			}
			std::cout << "\n";
		}
	}

	return ret;
}
