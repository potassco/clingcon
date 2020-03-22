# Clingcon

## Linting

The following linter configuration seems to be reasonable to implement:

    -DCMAKE_CXX_CLANG_TIDY:STRING="clang-tidy;-checks=clang-analyzer-*,readability-*,modernize-*,cppcoreguidelines-*,performance-*,bugprone-*,-modernize-use-trailing-return-type;-warnings-as-errors=*" ../..

Individual warnings can be disabled using `// NOLINT(warning)` in selected
lines.
