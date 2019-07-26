#pragma once
/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
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

#include "oned/ODRowReader.h"
#include "oned/rss/ODRSSPair.h"

#include <vector>

namespace ZXing {
namespace OneD {

/**
* Decodes RSS-14, including truncated and stacked variants. See ISO/IEC 24724:2006.
*/
class RSS14Reader : public RowReader
{
	struct State
	{
		std::vector<RSS::Pair> left, right;
	};

	mutable ThreadLocal<State> state;

public:
	Result decodeRow(int rowNumber, const BitArray& row) const override;

	void reset() override {
		auto& possiblePairs = state();
		possiblePairs.left.clear();
		possiblePairs.right.clear();
	}
};

} // OneD
} // ZXing
