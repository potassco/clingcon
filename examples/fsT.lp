#show assign/2.

index(1..N) :- N = #count { T: task(T) }.
first(I)    :- index(I), not index(I-1).
last(I)     :- index(I), not index(I+1).

1 { assign(I,T) : index(I) } 1 :- task(T).
1 { assign(I,T) : task(T) } 1 :- index(I).

% this projects out T
dur(I,M,D) :- assign(I,T), duration(T,M,D).

&sum { (I,M) + D } <= (I,M+1) :- index(I), dur(I,M,D), machine(M+1).
&sum { (I,M) + D } <= (I+1,M) :- index(I), dur(I,M,D), index(I+1).
&sum { (I,M) + D } <= bound   :- last(I), dur(I,M,D).

% static domains (cumbersome but straight-forward)
order(M,U,V) :- duration(T,M,D), U = (D,T), V = #min { (D',T') : duration(T',M,D'), (D',T') > U } < #sup.
index_order(M,I,U) :- first(I), order(M,U,_), not order(M,_,U).
index_order(M,I,V) :- index_order(M,I-1,U), order(M,U,V).
index_dur(M,I,D)     :- index_order(M,I,(D,_)).
index_rev(M,L-I+F,D) :- index_dur(M,I,D), first(F), last(L).

sum_up(M,I,0)   :- first(I), machine(M), not machine(M-1).
sum_up(M,I,S+D) :- first(I), machine(M), sum_up(M-1,I,S), index_dur(M-1,I,D).
sum_up(M,I,S+D) :- index(I), sum_up(M,I-1,S), index_dur(M,I-1,D).

sum_down(M,I,D)   :- last(I), machine(M), not machine(M+1), index_rev(M,I,D).
sum_down(M,I,S+D) :- last(I), machine(M), sum_down(M+1,I,S), index_rev(M,I,D).
sum_down(M,I,S+D) :- index(I), sum_down(M,I+1,S), index_rev(M,I,D).

min_dur(M,D) :- first(I), index_dur(M,I,D).

&sum { (I,M)     } >= S       :- sum_up(M,I,S).
&sum { (I,M) + S } <= bound   :- sum_down(M,I,S).
&sum { (I,M) + D } <= (I,M+1) :- index(I), min_dur(M,D), machine(M+1).
&sum { (I,M) + D } <= (I+1,M) :- index(I), min_dur(M,D), index(I+1).
