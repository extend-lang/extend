# Textdata format

Used for store any sort of data

## Text format

```pug
template.InnerTemplate # template declaration compiles to struct
  key1 = "default value"
  key2 = ""
  key3 = f32[4] 1, 2, 3, 4

template.OuterTemplate # nested templates
  inner.innerTemplate

root.OuterTemplate # data
  inner.innerTemplate
    key2 = "some value"
```

## C++

Compiles to:

```cpp
struct InnerTemplate {
  eastl::string key1 = "default value";
  eastl::string key2 = "";
  eastl::array<_Float32, 4> key3;
};

struct OuterTemplate {
  InnerTemplate inner;
};

OuterTemplate root = OuterTemplate {
  InnerTemplate {
    "default value",
    "some value",
    eastl::array<_Float32, 4>{1, 2, 3, 4}
  }
};
```

Template format is the same, but data type is required
for any key. Also accepted key without value: 
`key = type`. Template root must be tagname template.


Types:
- str
- Integers: `(i|u)(size)`, where size is 8, 16, 32, 64
- Floats: `f(size)`, where size is 16, 32, 64
- Vectors: `(i|u|f)(size)[(count)]`, count is 2, 3, 4
- Matrices: `(i|u|f)(size)[(count)][(count)]`
- Arrays: `(i|u|f)(size)[]`
