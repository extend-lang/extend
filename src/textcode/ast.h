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
#include <EASTL/span.h>
#include <g3log/g3log.hpp>
#include <sstream>
#include <string>

using namespace eastl;

namespace extend::textcode {

#define DECLARE_NODE(NAME) struct NAME;
#include "ast.nodes.h"
#undef DECLARE_NODE
struct Prog;

struct Node {
  enum struct VisitType {
    ENTER,
    EXIT
  };

  using Encoding = io::EncodingTraits<text::ParserTraits<text::ParserType::TEXTCODE>::encoding>;
  using Container = Encoding::Container;
  using Span = Encoding::Span;
  using Triplet = Encoding::Triplet;

  using Variant = variant<
#define DECLARE_NODE(NAME) NAME,
#include "ast.nodes.h"
#undef DECLARE_NODE
    Prog
  >;

  Node() = default;

  template<typename Visitor>
  static void
  dfs(Visitor visitor, vector<Variant> &children);

  vector<Variant> children;
};


struct Dcl : Node {
  enum struct Type {
#define DECLARE_TYPE(NAME, type) NAME,
#include "ast.dcl.types.h"
#undef DECLARE_TYPE
  };
  Type type;
  Container name;
  vector<intmax_t> indeces;

  Dcl(Type _type, Span _name, span<const intmax_t> _indeces = {});
};

Dcl::Container
typeToCpp(Dcl::Type type);

using VarDecl = tuple<Dcl::Container, vector<intmax_t>>;
using ParmType = tuple<Dcl::Type, Dcl::Container, intmax_t>;
using ParmTypes = vector<ParmType>;


struct Prog : Node {
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
  appendDcl(Dcl &&dcl);

  bool
  haveGlobalVar(Span name);

  void
  appendFunc(Func &&func);

  Func &
  currentFunc();

private:
  Func *lastFunc = nullptr;
};

struct Func : Node {
  Func(Dcl::Type _type, Span _name, span<const ParmType> _parameters);

  Dcl::Type type;
  Container name;
  ParmTypes parameters;

  void
  appendDcl(Dcl &&dcl);

  bool
  haveVar(Span name);
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
