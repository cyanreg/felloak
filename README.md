# felloak

felloak is a modular, flexible transassembler. It's goal is to convert one
architecture's assembly into another, with minimal changes required to the
input syntax.

The goal is to be able to reuse handwritten ARM64 SIMD as WebAssembly SIMD,
as well as provide an alternative, faster development platform for writing
and testing such assembly.

|Input ↓ Output → | ARM64 | WebAssembly        |
|---------------- | ----- | -------------------|
| ARM64           | ⬜️    | :heavy_check_mark: |
| WebAssembly     | ⬜️    | ⬜️                 |

Written in portable C99 without reliance on external libraries.

#### ARM64 → WebAssembly milestones
- [x] Parsing
- [x] Converting to IR
- [x] Validating
- [x] Converting to WASM IR
- [x] WAT syntax output
- [ ] Passing WASM validation
- [ ] WASM bytecode output
- [ ] IR-level optimizations
- [ ] WASM-level optimizations

#### Syntax
| Argument             | Description                                                      |
|----------------------|------------------------------------------------------------------|
| -o `path`            | Output file path                                                 |
| `path`               | Input file path                                                  |
| -v                   | Print version                                                    |
| -V                   | Toggle debug info printing                                       |
| -h                   | Print usage (this)                                               |

Overall, the project tries to maintain command-line compatibility with the
[nasm](https://nasm.us/) CLI syntax.
