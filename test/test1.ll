define i32 @test(i32 %a, i32 %b, i32 %c) {
entry:
  %x1 = add i32 %a, %b
  %x2 = mul i32 %x1, %c
  br label %cond

cond:
  %x3 = sub i32 %a, %c
  %x4 = add i32 %a, %b           ; repeated expression
  br i1 true, label %loop, label %merge

loop:
  %x5 = add i32 %a, %b           ; repeated expression again
  %x6 = mul i32 %x5, %c
  br label %merge

merge:
  %x7 = add i32 %a, %b           ; again repeated expression
  %x8 = mul i32 %x7, %c
  %x9 = add i32 %x8, %x6
  ret i32 %x9
}

