; ModuleID = 'test/sample.ll'
source_filename = "test/sample.ll"

define i32 @main() {
entry:
  %a = add i32 1, 2
  ret i32 %a
}
