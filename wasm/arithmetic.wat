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
  (func (export "div_ui32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.div_u)
  (func (export "div_si32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.div_s)
  (func (export "rem_ui32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.rem_u)
  (func (export "rem_si32") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.rem_s)
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
  (func (export "div_ui64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.div_u)
  (func (export "div_si64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.div_s)
  (func (export "rem_ui64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.rem_u)
  (func (export "rem_si64") (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.rem_s)
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
    f64.div))