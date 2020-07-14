(module
  (memory 1 2)
  (func (export "parami1") (param i32) (result i32)
      local.get 0
      local.get 0
      i32.add)

  (func (export "parami2") (param i32 i32) (result i32)
      local.get 0
      local.get 1
      i32.add)

  (func (export "parami3") (param i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      i32.add
      i32.add)

  (func (export "parami4") (param i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      i32.add
      i32.add
      i32.add)
  
  (func (export "parami5") (param i32 i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      i32.add
      i32.add
      i32.add
      i32.add)

  (func (export "parami6") (param i32 i32 i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add)

  (func (export "parami7") (param i32 i32 i32 i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add)

  (func (export "parami8") (param i32 i32 i32 i32 i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      local.get 7
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add)

  (func (export "parami8_64") (param i64 i64 i64 i64 i64 i64 i64 i64) (result i64)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      local.get 7
      i64.add
      i64.add
      i64.add
      i64.add
      i64.add
      i64.add
      i64.add)

  
  (func (export "parami64_7") (param i64 i64 i64 i64 i64 i64 i64) (result i64)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      i64.add
      i64.add
      i64.add
      i64.add
      i64.add
      i64.add)    

  (func (export "parami1_mem") (param i32) (result i32)
      local.get 0
      i32.load offset=0
      local.get 0
      i32.add)

  (func (export "parami7_mem") (param i32 i32 i32 i32 i32 i32 i32) (result i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      i32.load offset=0
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add
      i32.add)
)