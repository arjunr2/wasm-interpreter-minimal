(module
  (func (export "main") (param f64) (result i32)
    (i32.trunc_f64_s (local.get 0))
  )
)
