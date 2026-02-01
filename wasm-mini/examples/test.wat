;; test.wat - Minimal valid WebAssembly module for testing wasm-mini
;;
;; This module exports a simple 'add' function that adds two i32 integers.
;; Convert to binary with: wat2wasm test.wat -o test.wasm
;;
;; Usage:
;;   ./wasm-mini parse test.wasm
;;   ./wasm-mini validate test.wasm
;;   ./wasm-mini instantiate test.wasm

(module
  ;; Simple add function: takes two i32 parameters, returns their sum
  (func (export "add") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add
  )

  ;; Simple subtract function: takes two i32 parameters, returns their difference
  (func (export "sub") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.sub
  )

  ;; Constant function: returns 42
  (func (export "answer") (result i32)
    i32.const 42
  )
)
