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

% start at first index and on first machine
up_dur(I,M,0)     :- first(I), machine(M), not machine(M-1).
% start at first index
up_dur(I,M,S'+D') :- first(I), machine(M),                up_dur(I,M-1,S'), index_dur(M-1,I,D').
% start on first machine
up_dur(I,M,S+D)   :- up_dur(I-1,M,S), index_dur(M,I-1,D), index(I), machine(M), not machine(M-1).
% start at/on other indices/machines
up_dur(I,M,S+D)   :- up_dur(I-1,M,S), index_dur(M,I-1,D), up_dur(I,M-1,S'), index_dur(M-1,I,D'), S+D>=S'+D'.
up_dur(I,M,S'+D') :- up_dur(I-1,M,S), index_dur(M,I-1,D), up_dur(I,M-1,S'), index_dur(M-1,I,D'), S+D<S'+D'.

% Note: bounds can be refined using a similar scheme as above
down_dur(I,M,D)   :- last(I), index_rev(M,I,D).
down_dur(I,M,S+D) :- down_dur(I+1,M,S), index_rev(M,I,D).

&sum { (I,M)     } >= S       :- up_dur(I,M,S).
&sum { (I,M) + S } <= bound   :- down_dur(I,M,S).
&sum { (I,M) + D } <= (I,M+1) :- index(I), first(F), index_dur(M,F,D), machine(M+1).
&sum { (I,M) + D } <= (I+1,M) :- index(I), first(F), index_dur(M,F,D), index(I+1).
