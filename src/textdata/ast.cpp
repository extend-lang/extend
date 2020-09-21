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

namespace extend::textdata {

Field::Container
Field::typeToCpp() const {
  switch (this->type) {
#define DECLARE_FIELD_TYPE(NAME, type) case Type::NAME: return U"" #type;
#include "ast.field_types.h"
#undef DECLARE_FIELD_TYPE
    default:
      LOG(FATAL) << "Unexpected type";
      return U"";
  }
}

Document::Container
Document::exportCpp()
{
  Container exportCode;

  this->dfs([&exportCode](auto &node, Node::VisitType visit_type) {
    if (visit_type == Node::VisitType::EXIT) {
      return;
    }

    if constexpr(is_same_v<decay_t<decltype(node)>, Field>) {
      exportCode += node.typeToCpp();
      exportCode += U" ";
      exportCode += node.name;
      exportCode += U" = ";
      eastl::visit([&exportCode](const auto &value) {
        if constexpr (is_same_v<decay_t<decltype(value)>, Container>) {
          exportCode += U"u8\"";
          exportCode += value;
          exportCode += U"\"";
        } else {
          // FIXME: Remove std:: here, add ea::StdC and use sprintf here
          std::stringstream tempStream;
          tempStream << value;
          auto tempStr = tempStream.str();
          auto strView = basic_string_view<char8_t>(reinterpret_cast<const char8_t *>(tempStr.c_str()), tempStr.size());
          exportCode += Node::Encoding::decode(u8"<memory>", strView);
        }
      }, node.value);
      exportCode += U";\n";
    }
  });
  
  return exportCode;
}

Field::Field(Type _type, Span _name, VariantConst &&_value)
: type(_type), name(_name), value(_value)
{}

void
Document::appendField(Field::Type type, Span name, Field::VariantConst &&value)
{
  assert(!contains(name));
  children.emplace_back(Field(type, name, move(value)));
}

bool
Document::contains(Span name)
{
  for (Node::Variant &child : children) {
    assert(holds_alternative<Field>(child));
    Field &field = get<Field>(child);
    if (field.name == name) {
      return true;
    }
  }

  return false;
}

Field &
Document::get(Span name)
{
  for (Node::Variant &child : children) {
    assert(holds_alternative<Field>(child));
    Field &field = get<Field>(child);
    if (field.name == name) {
      return field;
    }
  }

  LOG(FATAL) << "cannot find a name `" << name << "` in document";
  assert(false);
}

}
