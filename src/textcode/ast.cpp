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

#include "ast.h"
#include <sstream>

namespace extend::textcode {

Dcl::Container
typeToCpp(Dcl::Type type) {
  switch (type) {
#define DECLARE_TYPE(NAME, type) case Dcl::Type::NAME: return U"" #type;
#include "ast.dcl.types.h"
#undef DECLARE_TYPE
    default:
      LOG(FATAL) << "Unexpected type";
      return U"";
  }
}

static Prog::Container
intmax_to_str(intmax_t x)
{
   // FIXME: Remove std:: here, add ea::StdC and use sprintf here
   std::stringstream tempStream;
   tempStream << x;
   auto tempStr = tempStream.str();
   auto strView = basic_string_view<char8_t>(reinterpret_cast<const char8_t *>(tempStr.c_str()), tempStr.size());
   return Node::Encoding::decode(u8"<memory>", strView);
}

Prog::Container
Prog::exportCpp()
{
  Container exportCode;
  Container prefix = U"";

  this->dfs([&](auto &node, Node::VisitType visit_type) {

    if constexpr(is_same_v<decay_t<decltype(node)>, Dcl>) {
      if (visit_type == Node::VisitType::ENTER) {
        exportCode += prefix;
        exportCode += typeToCpp(node.type);
        exportCode += U" ";
        exportCode += node.name;
        for (intmax_t ind : node.indeces) {
          exportCode += U"[";
          exportCode += intmax_to_str(ind);
          exportCode += U"]";
        }
        exportCode += U";\n";
      }
    }
    if constexpr(is_same_v<decay_t<decltype(node)>, Func>) {
      if (visit_type == Node::VisitType::ENTER) {
        exportCode += typeToCpp(node.type);
        exportCode += U" ";
        exportCode += node.name;
        exportCode += U"(";
        if (node.parameters.empty()) {
          exportCode += U"void";
        } else {
          for (const ParmType &param : node.parameters) {
            exportCode += typeToCpp(get<0>(param));
            exportCode += U" ";
            exportCode += get<1>(param);
            for (ssize_t i = 0; i < get<2>(param); ++ i) {
              exportCode += U"[]";
            }

            if (&param + 1 != node.parameters.end()) {
              exportCode += U", ";
            }
          }
        }
        exportCode += U") {\n";
        prefix += U"  ";
      } else {
        prefix = prefix.substr(0, prefix.size() - 2);
        exportCode += prefix;
        exportCode += U"}\n";
      }
    }
  });
  
  return exportCode;
}

Dcl::Dcl(Type _type, Span _name, span<const intmax_t> _indeces)
: type(_type), name(_name), indeces(_indeces.begin(), _indeces.end())
{}

Func::Func(Dcl::Type _type, Span _name, span<const ParmType> _parameters)
: type(_type), name(_name), parameters(_parameters.begin(), _parameters.end())
{}

void
Prog::appendDcl(Dcl &&dcl)
{
  assert(!haveGlobalVar(dcl.name));
  children.emplace_back(move(dcl));
}

void
Prog::appendFunc(Func &&func)
{
  assert(!haveGlobalVar(func.name));
  children.emplace_back(move(func));
  lastFunc = get_if<Func>(children.end() - 1);
}

Func &
Prog::currentFunc()
{
  assert(lastFunc);
  return *lastFunc;
}

bool
Prog::haveGlobalVar(Span name)
{
  for (Node::Variant &child : children) {
    if(holds_alternative<Dcl>(child)) {
      Dcl &dcl = get<Dcl>(child);
      if (dcl.name == name) {
        return true;
      }
    }
    if(holds_alternative<Func>(child)) {
      Func &func = get<Func>(child);
      if (func.name == name) {
        return true;
      }
    }
  }

  return false;
}

bool
Func::haveVar(Span name)
{
  for (Node::Variant &child : children) {
    if(holds_alternative<Dcl>(child)) {
      Dcl &dcl = get<Dcl>(child);
      if (dcl.name == name) {
        return true;
      }
    }
  }

  return false;
}

void
Func::appendDcl(Dcl &&dcl)
{
  assert(!haveVar(dcl.name));
  children.emplace_back(move(dcl));
}

}
