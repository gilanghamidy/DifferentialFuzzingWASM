(module 
  (import "" "g_i32" (global $g_i32 (mut i32)))
  (import "" "g_i32_2" (global $g_i32_2 (mut i32)))
  (import "" "g_i32_3" (global $g_i32_3 (mut i32)))
  (import "" "g_i64" (global $g_i64 (mut i64)))
  (func $global_i32_get (export "global_i32_get") (result i32)
    global.get $g_i32
  )
  (func $global_i64_get (export "global_i64_get") (result i64)
    global.get $g_i64
  )
  (func $global_i32_set (export "global_i32_set") (param i32)
    (global.set $g_i32 (local.get 0))
  )
  (func $global_i32_operate (export "global_i32_operate")
    (global.set $g_i32 (i32.add (global.get $g_i32_2) (global.get $g_i32_3)))
  )
)