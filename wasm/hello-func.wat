(module
  (global $g (import "" "global") (mut i32))
  (global $gf (import "" "globalfloat") (mut f32))

  (func $br_if_0 (export "br_if_0") (result i32)
    i32.const 9
    i32.const 5
    i32.const 1
    br_if 0
    drop
    drop
    i32.const 3
  )

  (func (export "addi32_folded") (param i32 i32) (result i32)
    (i32.add (local.get 0) (local.get 1))
  )

  (func (export "test_enjoy") (param i32 i32) (result i32)
    (local i32 i32)
    ;;local.get 0
    i32.const 256
    block $B1 (result i32)
      local.get 0
      i32.const 256
      i32.add
    end
    i32.add
  ) 


  (func (export "test_block") (param $val i32) (result i32)
    block $EXIT
      (i32.eqz (i32.rem_s (local.get $val) (i32.const 5)))
      br_if $EXIT ;; if(val % 5 == 0) goto EXIT
      (i32.mul (i32.const 20) (local.get $val))
      local.set $val ;; val *= 20
    end ;; EXIT:
    (i32.mul (local.get $val) (i32.const 3)) ;; res *= val
  )

  (func (export "test_if") (param $val i32) (result i32)
    (i32.rem_s (local.get $val) (i32.const 5))
    if ;; if(val % 5 != 0)
      (i32.mul (i32.const 20) (local.get $val))
      local.set $val ;; val *= 20
    end
    (i32.mul (local.get $val) (i32.const 3)) ;; res *= val
  )

  (func (export "test_loop") (param $a i32) (param $b i32) (result i32)
    (local $res i32) (local $cond i32) (local $temp_a i32)
    i32.const 3
    local.set $res
    loop $L0
      (i32.mul (local.get $res) (local.get $b))
      local.set $res
      (i32.lt_s (local.get $a) (i32.const 100))
      local.set $cond
      (i32.add (local.get $a) (i32.const 1))
      local.tee $temp_a
      local.set $a
      local.get $cond
      br_if $L0
    end
    (i32.mul (local.get $res) (local.get $temp_a))
  )

  (func (export "test_while") (param $a i32) (param $b i32) (result i32)
    (local $res i32) (local $temp_a i32) (local $cond i32)
    i32.const 3
    local.set $res
    block $LOOP_END
      block $LOOP_START
        (i32.le_s (local.get $a) (i32.const 100))
        br_if $LOOP_START
        local.get $a
        local.set $temp_a
        br $LOOP_END
      end ;; LOOP_START:
      loop $LOOP
        (i32.mul (local.get $res) (local.get $b))
        local.set $res
        (i32.lt_s (local.get $a) (i32.const 100))
        local.set $cond
        (i32.add (local.get $a) (i32.const 1))
        local.tee $temp_a
        local.set $a
        local.get $cond
        br_if $LOOP
      end
    end ;; LOOP_END:
    (i32.mul (local.get $res) (local.get $temp_a)))

    (func (export "test_break") (param $a i32) (param $b i32) (result i32)
    (local $res i32) (local $a_inc i32) (local $a_temp i32)
    i32.const 3
    local.set $res
    block $END_LOOP
      (i32.gt_s (local.get $a) (i32.const 100))
      br_if $END_LOOP
      (i32.add (local.get $a) (i32.const -1))
      local.set $a_inc
      (i32.add (i32.mul (local.get $b) (local.get $a))
               (i32.const -100))
      local.set $a
      i32.const 3
      local.set $res
      block $BREAK_LOOP
        loop $LOOP
          local.get $a_inc
          local.set $a_temp
          (i32.add (local.get $a) (local.get $b))
          local.tee $a
          i32.eqz
          br_if $BREAK_LOOP
          (i32.mul (local.get $res) (local.get $b))
          local.set $res
          (i32.add (local.get $a_temp) (i32.const 1))
          (i32.lt_s (local.tee $a_inc) (i32.const 100))
          br_if $LOOP
        end
      end
      (i32.add (local.get $a_temp) (i32.const 2))
      local.set $a
    end
    i32.const 25
    memory.grow drop
    (i32.mul (local.get $a) (local.get $res)))
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
  (func (export "get_global") (result i32)
      (global.get $g))
  (func (export "inc_global")
      (global.set $g
          (i32.add (global.get $g) (i32.const 1))))
  (func (export "get_global_float") (result f32)
      (global.get $gf))
  (func (export "add_global_float") (param f32)
      (global.set $gf
          (f32.add (global.get $gf) (local.get 0))))
  (memory 1 2)
  (func $get_text (param $v i32) (result i32)
    (local $ret i32)
    (local.set $ret (i32.const 0))
    block $END_IF
      (i32.and (local.get $v) (i32.const 1))
      br_if $END_IF
      (local.set $ret (i32.const 21))
      (i32.rem_s (local.get $v) (i32.const 3))
      br_if $END_IF 
      i32.const 46
      i32.const 70
      (i32.rem_s (local.get $v) (i32.const 5))
      select
      local.set $ret
    end ;; END_IF:
    local.get $ret)
  (data $d0 (i32.const 0) "it is an even number\00")
  (data $d1 (i32.const 21) "it is divisible by three\00")
  (data $d2 (i32.const 46) "it is divisible by five\00")
  (data $d3 (i32.const 70) "it is some odd number\00")

  (type $func_t (func (param i32 i32) (result i32)))
  (table 4 funcref)
  (func $f0 (param $fPtr i32) (param $a i32) (param $b i32) (result i32)
    (call_indirect (type $func_t) (local.get $a) (local.get $b) (local.get $fPtr)
  ))
  (func $add (type $func_t) (i32.add (local.get 0) (local.get 1)))
  (func $sub (type $func_t) (i32.sub (local.get 0) (local.get 1)))
  (func $mul (type $func_t) (i32.mul (local.get 0) (local.get 1)))
  (func $div (type $func_t) (i32.div_s (local.get 0) (local.get 1)))

  (func $getfunc (param $idx i32) (result i32)
    (local $ret i32)
    i32.const 1
    local.set $ret
    block $B0
      (i32.and (local.get $idx) (i32.const 1))
      br_if $B0 ;; if(idx % 2) return [1]
      (local.set $ret (i32.const 2))
      (i32.rem_s (local.get $idx) (i32.const 3))
      br_if $B0 ;; if(idx % 3) return [2]
      i32.const 3
      i32.const 4
      (i32.rem_s (local.get $idx) (i32.const 5))
      select    ;; if(idx % 5) return [3] else return [4]
      local.set $ret
    end
    local.get $ret)
  (elem (i32.const 0) $add $sub $mul $div)


)
