#include <csp>.

&sum {a; b; c} >= 113.

vars(a;b;c).
&show {X : vars(X)}.
&dom {1..100} = X :- vars(X).
&minimize{1*c@3;100*a@3}.
&minimize{1*c@2;100*b@1}.
