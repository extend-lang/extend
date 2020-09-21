/* extend - expansible programming language
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

#pragma once
#include <io/prelude.h>

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <emmintrin.h>
#include <llvm/Support/ConvertUTF.h>

using namespace eastl;

namespace extend::io {

enum struct EncodingType
{
#define DECLARE_ENCODING(name) name,
#include "encoding_types.h"
#undef DECLARE_ENCODING
};

template<EncodingType enc>
struct EncodingTraits;

template<>
struct EncodingTraits<EncodingType::US_ASCII>
{
  static constexpr EncodingType encoding = EncodingType::US_ASCII;
  using StorageUnit = char8_t;
  using CodePoint = char8_t;

  using Container = basic_string<CodePoint>;
  using Span = basic_string_view<CodePoint>;
  static constexpr Span zeroString = Span(nullptr, 0);
  struct Triplet final {
    using Storage = uint32_t;
  private:
    Storage value;
    static Triplet init(Storage val);

  public:
    explicit Triplet(CodePoint c1, CodePoint c2 = 0, CodePoint c3 = 0);

    Triplet(const Triplet &) noexcept = default;
    Triplet(Triplet &&) noexcept = default;
    Triplet &operator=(const Triplet &) noexcept = default;
    Triplet &operator=(Triplet &&) noexcept = default;
    ~Triplet() = default;

    static const Triplet eof;

    [[nodiscard]] ssize_t size() const;
    [[nodiscard]] CodePoint operator[](ssize_t i) const;

    friend bool operator==(
      const Triplet &lhs,
      const Triplet &rhs);
  };

  static Container decode(u8string_view filename,
                          basic_string_view<StorageUnit> source);
  static u8string encode(Span source);
};

template<>
struct EncodingTraits<EncodingType::UTF8>
{
  static constexpr EncodingType encoding = EncodingType::UTF8;
  using StorageUnit = char8_t;
  using CodePoint = char32_t;

  using Container = basic_string<CodePoint>;
  using Span = basic_string_view<CodePoint>;
  static constexpr Span zeroString = {nullptr, 0};
  struct Triplet final {
    using Storage = __m128i;
  private:
    Storage value;
    static Triplet init(Storage val);

  public:
    explicit Triplet(CodePoint c1, CodePoint c2 = 0, CodePoint c3 = 0);

    Triplet(const Triplet &) noexcept = default;
    Triplet(Triplet &&) noexcept = default;
    Triplet &operator=(const Triplet &) noexcept = default;
    Triplet &operator=(Triplet &&) noexcept = default;
    ~Triplet() = default;

    static const Triplet eof;

    [[nodiscard]] ssize_t size() const;
    [[nodiscard]] CodePoint operator[](ssize_t i) const;

    friend bool operator==(
      const Triplet &lhs,
      const Triplet &rhs);
  };

  static Container decode(u8string_view filename,
                          basic_string_view<StorageUnit> source);
  static u8string encode(Span source);
};

/**
 * Open file, read data and save source
 */
template<EncodingType enc>
struct FileSource final
{
  static constexpr EncodingType encoding = enc;
  using ThisType = FileSource<encoding>;
  using Traits = EncodingTraits<encoding>;
  using CodePoint = typename Traits::CodePoint;
  using Container = typename Traits::Container;
  using StorageUnit = typename Traits::StorageUnit;

private:
  FileSource(u8string_view _filename,
             basic_string_view<StorageUnit> source);

public:
  u8string_view filename;
  Container data;

  ~FileSource() = default;

  FileSource(FileSource&&) noexcept = default;
  FileSource(const FileSource&) = delete;
  FileSource& operator=(FileSource&&) noexcept = default;
  FileSource& operator=(const FileSource&) = delete;

  static FileSource memory(basic_string_view<StorageUnit> source);
  static FileSource memory(const StorageUnit* c_str);
  static FileSource open(u8string_view filename);
};

array<char, 5>
printChar(char8_t c);

array<char, 5>
printChar(char32_t c);

template<class Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const extend::io::EncodingTraits<extend::io::EncodingType::US_ASCII>::Triplet &str)
{
  output << "t'";
  for (ssize_t i = 0; i < str.size(); ++ i) {
    output << printChar(str[i]).begin();
  }
  output << '\'';

  return output;
}

template<class Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const extend::io::EncodingTraits<extend::io::EncodingType::UTF8>::Triplet &str)
{
  output << "t'";
  for (ssize_t i = 0; i < str.size(); ++ i) {
    output << printChar(str[i]).begin();
  }
  output << '\'';

  return output;
}
}

template<>
struct eastl::hash<extend::io::EncodingTraits<extend::io::EncodingType::US_ASCII>::Triplet>
{
  size_t operator()(const extend::io::EncodingTraits<extend::io::EncodingType::US_ASCII>::Triplet &);
};

template<>
struct eastl::hash<extend::io::EncodingTraits<extend::io::EncodingType::UTF8>::Triplet>
{
  size_t operator()(const extend::io::EncodingTraits<extend::io::EncodingType::UTF8>::Triplet &);
};
