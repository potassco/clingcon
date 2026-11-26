# Changes

## clingcon 5.2.3

- detect cycles of constraints that tighten bounds round after round (#95)
- add `@priority` levels for `&minimize` and `&maximize` with `--lex-opt` (#96)
- fix the constant of an objective changing its sign in later solve calls
- add `&heuristic` directives to select variables and values (#65)
- add `--order-heuristic` values `earliest-start` and `tightest-window`
- add bisecting bound steps for optimization with `--bound-step` (#65)

## clingcon 5.2.2

- improve build scripts (#123)
- extend syntax of `&dom` directives
- extend syntax of `&diff` atoms

## clingcon 5.2.1

- add option `--enable-python`
- fix `clingcon_configure` to have an effect (#102)
- fix multi-shot solving (#104)
- fix parsing of numbers (#107)

## clingcon 5.2.0

- add support for ASPIF parsing

## clingcon 5.1.0

- improve translation of linear constraints (#73)
- fix incremental minimization (#77)
- fix enumeration of optimal models (#28)
- enumerate all models if there is minimize directive (#69)
- various bugs are fixed by updating to clingo 5.5.1

## clingcon 5.0.0

- initial release
