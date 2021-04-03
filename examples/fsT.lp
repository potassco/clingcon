#show assign/2.

index(1..N) :- N = #count { T: task(T)  }.
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
% Note that domains could be refined even further because for a task to start
% on a machine, we can calculate a lower bound based on the minimum duration it
% can possibly take to finish the task on the previous machines. Something
% similar can also be done for the upper bounds. This should lead to further
% reduce of variables in the clingcon translation (but probably does not matter
% with clingo-dl).
order(M,U,V) :- duration(T,M,D), U = (D,T), V = #min { (D',T') : duration(T',M,D'), (D',T') > U } < #sup.
index_order(M,I,U)   :- first(I), order(M,U,_), not order(M,_,U).
index_order(M,I+1,V) :- index_order(M,I,U), order(M,U,V).
index_dur(M,I,D) :- index_order(M,I,(D,_)).

reverse(I,L-I+F) :- index(I), first(F), last(L).

up_dur(I,M,0)     :- first(I), machine(M).
up_dur(I+1,M,S+D) :- up_dur(I,M,S), index(I+1), index_dur(M,I,D).

down_dur(I,M,D)     :- last(I), reverse(I,J), index_dur(M,J,D).
down_dur(I-1,M,S+D) :- down_dur(I,M,S), reverse(I-1,J), index_dur(M,J,D).

&sum { (I,M)     } >= S       :- up_dur(I,M,S).
&sum { (I,M) + S } <= bound   :- down_dur(I,M,S).
&sum { (I,M) + D } <= (I,M+1) :- index(I), first(F), index_dur(M,F,D), machine(M+1).
&sum { (I,M) + D } <= (I+1,M) :- index(I), first(F), index_dur(M,F,D), index(I+1).
