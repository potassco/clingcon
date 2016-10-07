#include "../csp.lp".
#const end=20.
#const stepsize=7.
step(0..end).

% initial state
&sum {at(0)} = 0.

% actions
{move(T)} :- step(T), T > 0.

%effects
&sum {at(V); stepsize} = at(T) :- move(T), V = T-1.

%frame axiom
&sum {at(V)} = at(T) :- not move(T); step(T); step(V); V = T-1.

%goals
:- &sum {at(end)} <= 100. 

&show {at(X):step(X)}.
#show move/1.
