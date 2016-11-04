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

 &sum{q(X)} != q(n) :- X=1..n-1.
  
&sum{q(X)+X-1} != q(n)+n-1 :- X=1..n-1.
&sum{q(X)-X+1} != q(n)-n+1 :- X=1..n-1.
