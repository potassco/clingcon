#include "csp.lp".
#const end=2.
step(0..end).

%&sum {at(player,0)} = 0.
set(p,0).

%%loose :- &sum {at(player,end)} < end. 
loose :- &sum {at(player,end)} <= end. 

%%%{keep(p,T)} :- step(T), step(T-1).
%%%%%%&sum {at(player,T),-at(player,V)} = 0 :- keep(p,T), V = T-1, V >= 0.
%%%%%% Fehlerhaft war das KOMMA, das heisst das das nen Tuple wahr, so wie in sum {W,L,I : bla(W,L,I)}
%%%&sum {at(player,T); -at(player,V)} = 0 :- keep(p,T), V = T-1, V >= 0.
%%%set(p,T) :- keep(p,T).
%%%
%%%
%%%{move(T)} :- step(T), T > 0.
%%%&sum {at(player,T); -at(player,V)} = 2 :- move(T), V = T-1, V >= 0.
%%%set(p,T) :- move(T).
%%%
%%%
%%%:- step(T), not set(p,T).

:- loose.

%&show {at(player,X):step(X)}.
#show loose/0.
#show keep/2.
#show move/1.
