%%% use --distinct-to-card=false
#const istop = "UNKNOWN".
#include <incmode>.
#include "../csp.lp".


#program step(n).

pos(n).
&sum{q(n)} > 0.
&show {q(n)}.

% temporarily restrict the domain
&sum{q(X)} <= n :- pos(X), query(n).

&distinct{q(X) : pos(X)} :- query(n).
%diagonals
&distinct{q(X)+X-1 : pos(X)} :- query(n).
&distinct{q(X)-X+1 : pos(X)} :- query(n).
