(module
  (func $simple_goto (export "simple_goto") (param $val i32) (result i32)
    block $EXIT
      (i32.eqz (i32.rem_s (local.get $val) (i32.const 5)))
      br_if $EXIT ;; if(val % 5 == 0) goto EXIT
      (i32.mul (i32.const 20) (local.get $val))
      local.set $val ;; val *= 20
    end ;; EXIT:
    (i32.mul (local.get $val) (i32.const 3)) ;; res *= val
  )
  (func $simple_goto_64 (export "simple_goto_64") (param $val i64) (result i64)
    block $EXIT
      (i64.eqz (i64.rem_s (local.get $val) (i64.const 5)))
      br_if $EXIT ;; if(val % 5 == 0) goto EXIT
      (i64.mul (i64.const 20) (local.get $val))
      local.set $val ;; val *= 20
    end ;; EXIT:
    (i64.mul (local.get $val) (i64.const 3)) ;; res *= val
  )
  (func $simple_conditional (export "simple_conditional") (param $val i32) (result i32)
    (i32.rem_s (local.get $val) (i32.const 5))
    if ;; if(val % 5 != 0)
      (i32.mul (i32.const 20) (local.get $val))
      local.set $val ;; val *= 20
    end
    (i32.mul (local.get $val) (i32.const 3)) ;; res *= val
  )
  (func $do_while (export "do_while") (param $a i32) (param $b i32) (result i32)
    (local $res i32) (local $cond i32) (local $temp_a i32)
    i32.const 3
    local.set $res    ;; res = 3
    loop $L0
      (i32.mul (local.get $res) (local.get $b))
      local.set $res  ;; res *= b
      (i32.lt_s (local.get $a) (i32.const 100))
      local.set $cond ;; a < 100
      (i32.add (local.get $a) (i32.const 1))
      local.tee $temp_a 
      local.set $a    ;; a++
      local.get $cond
      br_if $L0       ;; while(a <= 100)
    end ;; POSTCONDITION: 101 <= temp_a <= (a + 1)
    (i32.mul (local.get $res) (local.get $temp_a)) ;; res * a
  )
  (func $break_loop (export "break_loop") (param $a i32) (param $b i32) (result i32)
        (local $res i32) (local $a_inc i32) (local $a_temp i32)
    i32.const 3
    local.set $res
    block $END_LOOP
      (i32.gt_s (local.get $a) (i32.const 100))
      br_if $END_LOOP       ;; if a > 100 skip the loop
      (i32.add (local.get $a) (i32.const -1))
      local.set $a_inc
      (i32.add (i32.mul (local.get $b) (local.get $a))
               (i32.const -100))
      local.set $a          ;; a to store a * b value
      block $BREAK_LOOP
        loop $LOOP
          (local.set $a_temp (local.get $a_inc))
          (i32.add (local.get $a) (local.get $b))
          local.tee $a      ;; store a * b result
          i32.eqz
          br_if $BREAK_LOOP ;; if(a * b == 100) break
          (i32.mul (local.get $res) (local.get $b))
          local.set $res    ;; res *= b
          (i32.add (local.get $a_temp) (i32.const 1))
          (i32.lt_s (local.tee $a_inc) (i32.const 100))
          br_if $LOOP       ;; while(a <= 100)
        end
      end ;; BREAK_LOOP:
      (i32.add (local.get $a_temp) (i32.const 2))
      local.set $a          ;; reset a value to original
    end ;; END_LOOP:
    (i32.mul (local.get $a) (local.get $res)) ;; res * a
  ))