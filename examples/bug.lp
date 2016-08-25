#include "csp.lp".
loose :- &sum {x} <= 2.
:- loose.


&dom{1..10}=x.

&show {x}.
#show loose/0.

