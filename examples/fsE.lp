% select a cycle

1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

% make sure the cycle is connected
reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

% select a start point
1 { start(T) : task(T) } 1.

% obtain an order
permutation(T,U) :- cycle(T,U), not start(U).

% place tasks sequentially on machines
seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&sum { T2 - T1 } >= D :- seq(T1,T2,D).
&sum { (T,M) } >= 0       :- duration(T,M,D).
&sum { (T,M) } <= bound-D :- duration(T,M,D).

#show permutation/2.
