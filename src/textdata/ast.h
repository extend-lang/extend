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
#include "io/FileSource.h"
#include <EASTL/string_view.h>
#include <io/prelude.h>
#include "text/ParserTraits.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>
#include <EASTL/variant.h>
#include <g3log/g3log.hpp>
#include <sstream>
#include <string>

using namespace eastl;

namespace extend::textdata {

struct Document;
struct Field;

struct Node {
  enum struct VisitType {
    ENTER,
    EXIT
  };

  using Encoding = io::EncodingTraits<text::ParserTraits<text::ParserType::TEXTDATA>::encoding>;
  using Container = Encoding::Container;
  using Span = Encoding::Span;

  using Variant = variant<Document, Field>;

  Node() = default;

  template<typename Visitor>
  static void
  dfs(Visitor visitor, vector<Variant> &children);

protected:
  vector<Variant> children;
};


struct Field : Node {
  enum struct Type {
#define DECLARE_FIELD_TYPE(NAME, type) NAME,
#include "ast.field_types.h"
#undef DECLARE_FIELD_TYPE
  };
  Type type;
  const Container name;
  using VariantConst = variant<uintmax_t, intmax_t, Node::Container>;
  const VariantConst value;

  Field(Type _type, Span _name, VariantConst &&_value);

  [[nodiscard]] Container
  typeToCpp() const;
};


struct Document : Node {
  template<typename Visitor>
  void
  dfs(Visitor visitor)
  {
    visitor(*this, VisitType::ENTER);
    Node::dfs(visitor, children);
    visitor(*this, VisitType::EXIT);
  }

  Container
  exportCpp();

  void
  appendField(Field::Type type, Span name, Field::VariantConst &&value);

  bool
  contains(Span name);

  Field &
  get(Span name);
};


template<typename Visitor>
void Node::dfs(Visitor visitor, vector<Variant> &children)
{
  for (Variant &child : children) {
    eastl::visit([&visitor](auto &childWithActualClass) {
      visitor(childWithActualClass, Node::VisitType::ENTER);
      Node::dfs(visitor, childWithActualClass.children);
      visitor(childWithActualClass, Node::VisitType::EXIT);
    }, child);
  }
}

}
