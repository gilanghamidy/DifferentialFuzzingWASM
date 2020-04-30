(module
  (func (export "addi32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add)
  (func (export "subi32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.sub)
  (func (export "muli32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.mul)
  (func (export "divi32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.div_u)
  (func (export "addi64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.add)
  (func (export "subi64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.sub)
  (func (export "muli64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.mul)
  (func (export "divi64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.div_u)
  (func (export "addf32") (param f32 f32) (result f32)
    local.get 0
    local.get 1
    f32.add)
  (func (export "subf32") (param f32 f32) (result f32)
    local.get 0
    local.get 1
    f32.sub)
  (func (export "mulf32") (param f32 f32) (result f32)
    local.get 0
    local.get 1
    f32.mul)
  (func (export "divf32") (param f32 f32) (result f32)
    local.get 0
    local.get 1
    f32.div)
  (func (export "addf64") (param f64 f64) (result f64)
    local.get 0
    local.get 1
    f64.add)
  (func (export "subf64") (param f64 f64) (result f64)
    local.get 0
    local.get 1
    f64.sub)
  (func (export "mulf64") (param f64 f64) (result f64)
    local.get 0
    local.get 1
    f64.mul)
  (func (export "divf64") (param f64 f64) (result f64)
    local.get 0
    local.get 1
    f64.div)
  (func (export "void") (param i32)
    local.get 0
    local.get 0
    i32.mul
    drop
  )
  (func (export "trap")
    i32.const 51651654
    i32.load offset=0
    drop
  )
  (memory 1 2)
)
