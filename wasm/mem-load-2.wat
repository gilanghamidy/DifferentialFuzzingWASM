(module
  (import "thesis" "memory" (memory $mem 16 32))
  (import "thesis" "global1" (global $global1 (mut i32)))
  (import "thesis" "table" (table 1 funcref))
  (import "thesis" "ext_mul" (func $ext_mul (type $op)))

  (type $op (func (param i32 i32)(result i32)))
  (func $add (type $op) (i32.add (local.get 0)(local.get 1)))
  (func $sub (type $op) (i32.sub (local.get 0)(local.get 1)))
  

  (func $compute (param i32) (result i32)
    local.get 0
    i32.load offset=0
    global.get $global1
    i32.const 0
    call_indirect (type $op))
  (export "compute" (func $compute))
  (export "add" (func $add))
  (export "sub" (func $sub))
  (export "mul" (func $ext_mul)))
