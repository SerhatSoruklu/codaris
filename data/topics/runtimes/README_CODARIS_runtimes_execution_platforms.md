# CODARIS Runtimes & Execution Platforms Research Pack

Generated: 23 September 2026

## Scope

This research pack contains **581 runtime and execution-platform entries**.

The catalogue intentionally spans several layers because developers use the word "runtime" for different things:

- application runtimes such as Node.js, Deno and Bun
- language engines such as V8, SpiderMonkey and JavaScriptCore
- managed virtual machines such as JVM, HotSpot, OpenJ9 and CLR/CoreCLR
- language implementations such as CPython, PyPy, JRuby and LuaJIT
- runtime systems linked into native applications such as Go Runtime and GHC RTS
- WebAssembly runtimes such as Wasmtime, Wasmer, WasmEdge, WAMR and wazero
- browser execution environments such as Service Workers and Web Workers
- edge/serverless execution platforms such as Cloudflare Workers and AWS Lambda
- embedded runtimes
- GPU/accelerator runtimes such as CUDA Runtime

CODARIS should show **Runtime Type**, **Runtime Family**, and **Engine / Host Relation** prominently.

## Key architecture examples

### Node.js

JavaScript / TypeScript application
→ Node.js runtime
→ V8 JavaScript engine
→ JIT/native machine execution

Node.js is **not** the same thing as V8.

### Deno

JavaScript / TypeScript
→ Deno runtime and APIs
→ V8
→ native execution

Deno adds a permission model, tooling, module system and runtime APIs around V8.

### Bun

JavaScript / TypeScript
→ Bun runtime/toolchain
→ JavaScriptCore
→ native execution

Bun does **not** use V8.

### Java / Kotlin on JVM

Java/Kotlin/etc.
→ JVM bytecode
→ a JVM implementation such as HotSpot, OpenJ9 or GraalVM
→ native execution

The JVM is a virtual-machine specification/family. HotSpot is one important implementation, not the only one.

### .NET

C#/F#/VB
→ CIL
→ CoreCLR/CLR/Mono or NativeAOT
→ native execution

NativeAOT changes the deployment/execution model by compiling IL to native code before runtime.

### WebAssembly

Rust/C/C++/Go/etc.
→ WebAssembly module/component
→ Wasmtime/Wasmer/WasmEdge/browser engine/etc.
→ host machine

Wasm is a binary instruction format. WASI defines portable system-facing interfaces. Neither is itself a single runtime implementation.

## State of JavaScript 2025 runtime usage

The JavaScript Runtimes question had **11,141 respondents** and allowed multiple selections.

- Node.js: 10,062 selections, **90.3%**
- Browser: 9,682, **86.9%**
- Bun: 2,321, **20.8%**
- Service Workers: 1,660, **14.9%**
- Cloudflare Workers: 1,294, **11.6%**
- Deno: 1,244, **11.2%**
- Hermes: 354, **3.2%**
- ChakraCore: 33, **0.3%**

These numbers describe State of JavaScript respondents, not worldwide market share.

The separate edge/serverless runtime question had **9,593 respondents**.

## Current snapshots

The workbook includes selected current-version/status facts verified for 23 September 2026, including Node.js release status, the Java SE 27 JVM specification, OpenJ9's September 2026 release, MicroPython, WASI milestones and CUDA Runtime documentation.

## Geography and demographics

No runtime-by-country or runtime-by-gender percentages are fabricated.

Overall Stack Overflow survey geography is included only as sample context. A country's respondent share does not tell us which runtime developers in that country use.

## Files

- `CODARIS_runtimes_execution_platforms_research.xlsx`
- `codaris_runtimes_execution_platforms_catalog.csv`
- `codaris_runtimes_execution_platforms_catalog.json`
- `codaris_runtime_usage_stateofjs_2025.csv`
- `codaris_edge_runtime_usage_stateofjs_2025.csv`
- `codaris_runtimes_sources.csv`

## Recommended CODARIS fields

1. Runtime / execution platform name
2. Runtime family
3. Runtime type
4. Languages / executable format
5. Execution model
6. Bytecode / IR
7. Memory-management model
8. Typical environment
9. Sandbox / isolation model
10. Provider or project
11. Engine / host relationship
12. Usage metric plus source/year where available
13. Notes
14. Source provenance

Do not create one synthetic "runtime popularity score" by averaging unrelated survey, download, website, GitHub or vendor metrics.
