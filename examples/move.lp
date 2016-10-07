#include "../csp.lp".
#const end=20.
step(0..end).
#const stepsize=7.

&sum {at(player,0)} = 0.

{move(T)} :- step(T), T > 0.

%effects
&sum {at(player,V); stepsize} = at(player,T) :- move(T), V = T-1.

%frame axiom
&sum {at(player,V)} = at(player,T)    :- not move(T); step(T); step(V); V = T-1.

loose :- &sum {at(player,end)} <= 100. 
:- loose.

&show {at(player,X):step(X)}.
#show loose/0.
#show move/1.
