#const istop = "UNKNOWN".
#include <incmode>.

#const max=300.
#program step(n).

pos(n).
&dom{1..max} = q(n).
&show {q(n)}.

% temporarily restrict the domain
&sum{q(X)} <= n :- pos(X), query(n).

&distinct{q(X) : pos(X)} :- query(n).
%diagonals
&distinct{q(X)+X-1 : pos(X)} :- query(n).
&distinct{q(X)-X+1 : pos(X)} :- query(n).
