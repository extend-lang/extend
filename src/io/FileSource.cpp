/* extend - file format and program language
 * Copyright (C) 2021 Vladimir Liutov vs@lutov.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "FileSource.h"

#include <EASTL/vector.h>
#include <emmintrin.h>
#include <g3log/g3log.hpp>

#include <cassert>
#include <fstream>
#include <g3log/loglevels.hpp>
#include <sys/types.h>

namespace extend::io {

template<typename CodePoint>
static void
replace_newlines(basic_string<CodePoint>& data)
{
  auto len = ssize_t(data.length());
  ssize_t j = 0;

  for (ssize_t i = 0; i < len; ++i, ++j) {
    CodePoint c = data[i];
    if (i + 1 < len &&
        ((c == CodePoint('\n') && data[i + 1] == CodePoint('\r')) ||
         (c == CodePoint('\r') && data[i + 1] == CodePoint('\n')))) {
      ++i;
    }

    if (c == CodePoint('\r')) {
      c = CodePoint('\n');
    }

    data[j] = c;
  }

  data.resize(j);
}

static void
input_pos(u8string_view source, ssize_t offset, ssize_t &row, ssize_t &col, u8string_view &line)
{
  const char8_t *lineStart = source.begin();
  const char8_t *targetChar = source.begin() + offset;
  row = 1;
  col = 1;
  for (const char8_t *c = source.begin(); c < targetChar; ++ c) {
    if (*c == '\r') {
      continue;
    }
    if (*c == '\n') {
      ++row;
      col = 1;
      lineStart = c + 1;
    } else {
      ++col;
    }
  }
  line = u8string_view(lineStart, source.end() - lineStart);
  for (const char8_t *c = targetChar; c < source.end(); ++ c) {
    if (*c == '\n') {
      line = u8string_view(lineStart, c - lineStart);
      break;
    }
  }
}

template<EncodingType enc>
FileSource<enc>::FileSource(u8string_view _filename,
                            basic_string_view<StorageUnit> source)
  : filename(_filename)
  , data(Traits::decode(_filename, source))
{
  replace_newlines(data);
}

template<EncodingType enc>
FileSource<enc>
FileSource<enc>::memory(basic_string_view<StorageUnit> source)
{
  return FileSource(u8"<memory>", source);
}

template<EncodingType enc>
FileSource<enc>
FileSource<enc>::memory(const StorageUnit* c_str)
{
  ssize_t length = 0;
  while (*(c_str + length) != 0) {
    ++length;
  }
  return FileSource(u8"<memory>", basic_string_view<StorageUnit>(const_cast<StorageUnit *>(c_str), length)); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

template<EncodingType enc>
FileSource<enc>
FileSource<enc>::open(u8string_view filename)
{
  u8string str(filename);
  std::ifstream ifs = std::ifstream(str.c_str(),
                                    std::ifstream::in | std::ifstream::ate |
                                      std::ifstream::binary);

  if (!ifs.good()) {
    LOG(FATAL) << "Cannot open file " << filename;
  }

  ssize_t byte_length = ifs.tellg();
  LOG(DEBUG) << "Read file " << filename << " with length " << byte_length
             << " bytes";
  ifs.seekg(0);

  assert(byte_length % sizeof(StorageUnit) == 0);
  ssize_t storage_length = byte_length / sizeof(StorageUnit);
  basic_string<StorageUnit> source(storage_length, ' ');

  ifs.read(reinterpret_cast<char*>(source.data()), byte_length);
  return FileSource(filename, source);
}

//
// Encoding US_ASCII
//

EncodingTraits<EncodingType::US_ASCII>::Container
EncodingTraits<EncodingType::US_ASCII>::decode(
  u8string_view filename,
  basic_string_view<StorageUnit> source)
{
  for (const StorageUnit& c : source) {
    if (c < 0 || c >= 128) {
      ssize_t row, col;
      u8string_view line;
      input_pos(source, &c - source.begin(), row, col, line);
      LOG(FATAL) << filename << ':' << row << ':' << col
                 << ": error: Not US ASCII symbol `" << c << '`';
    }
  }
  return basic_string<CodePoint>(source.data(), source.size());
}

u8string
EncodingTraits<EncodingType::US_ASCII>::encode(Span str)
{
  return u8string(str);
}

//
// Encoding UTF8
//

EncodingTraits<EncodingType::UTF8>::Container
EncodingTraits<EncodingType::UTF8>::decode(u8string_view filename,
                                           basic_string_view<StorageUnit> source)
{
  static_assert(sizeof(llvm::UTF8) == sizeof(StorageUnit));
  static_assert(sizeof(llvm::UTF32) == sizeof(CodePoint));

  const auto *sourcePtr = reinterpret_cast<const llvm::UTF8 *>(source.begin());
  ssize_t length = 0;
  while (sourcePtr < reinterpret_cast<const llvm::UTF8 *>(source.end())) {
    sourcePtr += llvm::getNumBytesForUTF8(*sourcePtr);
    ++length;
  }

  basic_string<CodePoint> target(length, ' ');
  sourcePtr = reinterpret_cast<const llvm::UTF8 *>(source.begin());
  auto *targetPtr = reinterpret_cast<llvm::UTF32 *>(target.begin());

  llvm::ConversionResult errorCode =
    llvm::ConvertUTF8toUTF32(&sourcePtr,
                             reinterpret_cast<const llvm::UTF8 *>(source.end()),
                             &targetPtr,
                             reinterpret_cast<llvm::UTF32 *>(target.end()),
                             llvm::ConversionFlags::strictConversion);

  if (errorCode != llvm::ConversionResult::conversionOK) {
    ssize_t row, col;
    u8string_view line;
    input_pos(source, sourcePtr - reinterpret_cast<const llvm::UTF8 *>(source.begin()), row, col, line);
    LOG(FATAL) << filename << ':' << row << ':' << col
               << ": error: Not UTF-8 symbol `" << *sourcePtr << '`';
  }

  target.resize(targetPtr - reinterpret_cast<llvm::UTF32 *>(target.begin()));

  return target;
}

u8string
EncodingTraits<EncodingType::UTF8>::encode(Span source)
{
  u8string target(4 * source.size(), ' ');
  const auto *sourcePtr = reinterpret_cast<const llvm::UTF32 *>(source.begin());
  auto *targetPtr = reinterpret_cast<llvm::UTF8 *>(target.begin());

  llvm::ConversionResult errorCode =
    llvm::ConvertUTF32toUTF8(&sourcePtr,
                             reinterpret_cast<const llvm::UTF32 *>(source.end()),
                             &targetPtr,
                             reinterpret_cast<llvm::UTF8 *>(target.end()),
                             llvm::ConversionFlags::strictConversion);

  assert(errorCode == llvm::ConversionResult::conversionOK);

  target.resize(targetPtr - reinterpret_cast<llvm::UTF8 *>(target.begin()));

  return target;
}

//
// Triplet US_ASCII
//

EncodingTraits<EncodingType::US_ASCII>::Triplet::Triplet(
  CodePoint c1, CodePoint c2, CodePoint c3)
  : value(uint32_t(c1) | (uint32_t(c2) << 8) | (uint32_t(c3) << 16))
{
  assert(c1 != 0);
  if (c3 != 0) {
    assert(c2 != 0);
  }
}

EncodingTraits<EncodingType::US_ASCII>::Triplet
EncodingTraits<EncodingType::US_ASCII>::Triplet::init(Storage val)
{
  Triplet res(1);
  res.value = val;
  return res;
}

EncodingTraits<EncodingType::US_ASCII>::CodePoint
EncodingTraits<EncodingType::US_ASCII>::Triplet::operator[](ssize_t i) const
{
  switch (i) {
    case 0: return value & 0xff;
    case 1: return (value >> 8) & 0xff;
    default:
      assert(i == 2);
      return (value >> 16) & 0xff;
  };
}

ssize_t
EncodingTraits<EncodingType::US_ASCII>::Triplet::size() const
{
  if (*this == eof) {
    return -1;
  }

  if ((*this)[1] == 0) {
    return 1;
  }

  if ((*this)[2] == 0) {
    return 2;
  }

  return 3;
}

const EncodingTraits<EncodingType::US_ASCII>::Triplet EncodingTraits<EncodingType::US_ASCII>::Triplet::eof = Triplet::init(0xffffffffU);

bool operator==(
  const EncodingTraits<EncodingType::US_ASCII>::Triplet &lhs,
  const EncodingTraits<EncodingType::US_ASCII>::Triplet &rhs
)
{
  return lhs.value == rhs.value;
}

//
// Triplet UTF8
//

EncodingTraits<EncodingType::UTF8>::Triplet::Triplet(
  CodePoint c1, CodePoint c2, CodePoint c3)
  : value(_mm_set_epi32(0, int32_t(c3), int32_t(c2), int32_t(c1)))
{
  if (c1 == 0) {
    assert(c1 != 0);
  }
  if (c3 != 0) {
    assert(c2 != 0);
  }
}

EncodingTraits<EncodingType::UTF8>::Triplet
EncodingTraits<EncodingType::UTF8>::Triplet::init(Storage val)
{
  Triplet res(-1);
  res.value = val;
  return res;
}

EncodingTraits<EncodingType::UTF8>::CodePoint
EncodingTraits<EncodingType::UTF8>::Triplet::operator[](ssize_t i) const
{
  switch (i) {
    case 0: return _mm_cvtsi128_si32(value);
    case 1: return _mm_cvtsi128_si32(_mm_shuffle_epi32(value, 1));
    default:
      assert(i == 2);
      return _mm_cvtsi128_si32(_mm_shuffle_epi32(value, 2));
  };
}

ssize_t
EncodingTraits<EncodingType::UTF8>::Triplet::size() const
{
  if (*this == eof) {
    return -1;
  }

  if ((*this)[1] == 0) {
    return 1;
  }

  if ((*this)[2] == 0) {
    return 2;
  }

  return 3;
}

const EncodingTraits<EncodingType::UTF8>::Triplet EncodingTraits<EncodingType::UTF8>::Triplet::eof = Triplet::init(_mm_set_epi32(-1, -1, -1, -1));


bool operator==(
  const EncodingTraits<EncodingType::UTF8>::Triplet &lhs,
  const EncodingTraits<EncodingType::UTF8>::Triplet &rhs
)
{
  return _mm_movemask_epi8(_mm_cmpeq_epi32(lhs.value, rhs.value)) == 0xffff;
}

// printChar

array<char, 5>
printChar(char8_t c)
{
  switch (c) {
    case '\n': return {'\\', 'n', 0};
    case '\r': return {'\\', 'r', 0};
    case '\t': return {'\\', 't', 0};
    default: ;
  }

  return {char(c), 0};
}

array<char, 5>
printChar(char32_t c)
{
  switch (c) {
    case '\n': return {'\\', 'n', 0};
    case '\r': return {'\\', 'r', 0};
    case '\t': return {'\\', 't', 0};
    default: ;
  }

  array<char, 5> target = {0, 0, 0, 0, 0};
  const auto *sourcePtr = reinterpret_cast<const llvm::UTF32 *>(&c);
  auto *targetPtr = reinterpret_cast<llvm::UTF8 *>(&target);
  llvm::ConversionResult errorCode = llvm::ConvertUTF32toUTF8(
    &sourcePtr, sourcePtr + 1,
    &targetPtr, targetPtr + 5,
    llvm::ConversionFlags::strictConversion);
  assert(errorCode == llvm::ConversionResult::conversionOK);

  return target;
}

// Explicit instatiation

template struct FileSource<EncodingType::US_ASCII>;
template struct FileSource<EncodingType::UTF8>;
// TODO: implement bits

} // namespace extend::io

size_t eastl::hash<extend::io::EncodingTraits<extend::io::EncodingType::US_ASCII>::Triplet>::operator()(const extend::io::EncodingTraits<extend::io::EncodingType::US_ASCII>::Triplet &triplet)
{
  uint32_t result = 2166136261U;   // FNV1 hash. Perhaps the best string hash. Intentionally uint32_t instead of size_t, so the behavior is the same regardless of size.
  result = (result * 16777619) ^ triplet[0];
  result = (result * 16777619) ^ triplet[1];
  result = (result * 16777619) ^ triplet[2];
  return result;
}

size_t eastl::hash<extend::io::EncodingTraits<extend::io::EncodingType::UTF8>::Triplet>::operator()(const extend::io::EncodingTraits<extend::io::EncodingType::UTF8>::Triplet &triplet)
{
  uint32_t result = 2166136261U;   // FNV1 hash. Perhaps the best string hash. Intentionally uint32_t instead of size_t, so the behavior is the same regardless of size.
  result = (result * 16777619) ^ triplet[0];
  result = (result * 16777619) ^ triplet[1];
  result = (result * 16777619) ^ triplet[2];
  return size_t(result);
}
