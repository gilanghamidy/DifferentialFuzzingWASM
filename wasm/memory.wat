(module
  (memory 1 10)
  (func $zero_offset_i32 (export "zero_offset_i32") (param $adrr i32) (result i32)
    (i32.load offset=0 (local.get 0))
  )
  (func $zero_offset_i64 (export "zero_offset_i64") (param $adrr i32) (result i64)
    (i64.load offset=0 (local.get 0))
  )
  (func $zero_offset_f32 (export "zero_offset_f32") (param $adrr i32) (result f32)
    (f32.load offset=0 (local.get 0))
  )
  (func $zero_offset_f64 (export "zero_offset_f64") (param $adrr i32) (result f64)
    (f64.load offset=0 (local.get 0))
  )
  (func $with_offset_i32 (export "with_offset_i32") (param $adrr i32) (result i32)
    (i32.load offset=1024 (local.get 0))
  )
  (func $with_offset_i64 (export "with_offset_i64") (param $adrr i32) (result i64)
    (i64.load offset=1024 (local.get 0))
  )
  (func $store_zero_offset_i64 (export "store_zero_offset_i64") (param $adrr i32) (param $val i64)
    (i64.store offset=0 (local.get 0) (local.get 1))
  )

  (func $load_store_zero_offset_i64 (export "load_store_zero_offset_i64") (param $adrr i32) (param $val i64)
    (i64.store offset=0 (local.get 0) (i64.add (i64.load offset=0 (local.get 0)) (local.get 1)))
  )


  (func $memory_grow_0 (export "memory_grow_0") (result i32)
    i32.const 0
    memory.grow
  )

  (func $memory_grow_s (export "memory_grow_s") (param i32) (result i32)
    local.get 0
    memory.grow
  )
)