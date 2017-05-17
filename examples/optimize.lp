#include <csp>.

&sum {a; b; c} = 113.

vars(a;b;c).
&show {X : vars(X)}.
&dom {1..100} = X :- vars(X).
&minimize{1*c;100*a}.
