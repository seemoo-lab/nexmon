# Check that the generate AST exploits the stride information and,
# in particular, does not involve any if-statement.
domain: "[N] -> { S[a] : exists (e0: 0 <= a <= 59 and 1 + N + 6e0 <= 4a <= 2 + N + 6e0)}"
child:
  context: "[N] -> { [] : N >= 0 }"
