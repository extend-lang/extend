# Extend

extendable programming language

Distributed under the [AGPL 3](LICENSE.md)

Data file format

Text format:
```pug
.outerTemplate # comment
  key1.innerTemplate
    key2 = str "string"
    key3 = "string"
    key4 = f32[4] 1, 2, 3, 4
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

## Build

Requirements:
- podman
- buildah

Build:
```sh
  ./build.sh
```

## TODO

- Move language-specific functional from base lexer (e.g. braces, operators, keywords).
- unicode
- musl libc
- static
- doxygen
- cmake/ninja
- buildah
