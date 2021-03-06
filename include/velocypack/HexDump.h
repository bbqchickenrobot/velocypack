////////////////////////////////////////////////////////////////////////////////
/// @brief Library to build up VPack documents.
///
/// DISCLAIMER
///
/// Copyright 2015 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Jan Steemann
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef VELOCYPACK_HEXDUMP_H
#define VELOCYPACK_HEXDUMP_H 1

#include <iosfwd>

#include "velocypack/velocypack-common.h"
#include "velocypack/Slice.h"
#include "velocypack/Value.h"
#include "velocypack/ValueType.h"

namespace arangodb {
namespace velocypack {

struct HexDump {
  HexDump() = delete;

  HexDump(Slice const& slice, int valuesPerLine = 16,
          std::string const& separator = " ")
      : slice(slice), valuesPerLine(valuesPerLine), separator(separator) {}

  HexDump(Slice const* slice, int valuesPerLine = 16,
          std::string const& separator = " ")
      : HexDump(*slice, valuesPerLine, separator) {}

  static std::string toHex(uint8_t value);

  Slice const slice;
  int valuesPerLine;
  std::string separator;
};

}  // namespace arangodb::velocypack
}  // namespace arangodb

std::ostream& operator<<(std::ostream&, arangodb::velocypack::HexDump const*);

std::ostream& operator<<(std::ostream&, arangodb::velocypack::HexDump const&);

#endif
