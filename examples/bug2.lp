#include "csp.lp".

step(1).

1{move(T)}1 :- step(T).
%move(T) :- step(T).


&sum {at(player,T); at(enemy,T)} = 0 :- move(T).

&show {at(enemy,X) :step(X); at(player,X) :step(X)}.
