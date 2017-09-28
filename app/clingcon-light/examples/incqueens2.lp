%%% use --distinct-to-card=false
#const istop = "UNKNOWN".
#include <incmode>.


#program step(n).

pos(n).
&sum{q(n)} > 0.
&show {q(n)}.

% temporarily restrict the domain
&sum{q(X)} <= n :- pos(X), query(n).

&distinct{q(X) : pos(X)}.
%diagonals
&distinct{q(X)+X-1 : pos(X)}.
&distinct{q(X)-X+1 : pos(X)}.

