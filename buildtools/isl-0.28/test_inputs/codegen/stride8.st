# Check that the generate AST exploits the stride information and,
# in particular, does not involve any if-statement.
domain: "[N] -> { S[i0] : exists (e0 : i0 = floor(N/2) + 6*e0 and 0 <= e0 <= 1) }"
