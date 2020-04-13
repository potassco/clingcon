% jobs start at zero and must finish before a given bound
&dom { 0..B } = (J,M) :- duration(J,M,T), B=bound-T.

% a job has to be finished on a machine before starting on the next one.
&sum{ (J,M)+T } <= (J,M+1) :- duration(J,M,T), machine(M+1).

% the processing times of two jobs must not overlap on a machine
&disjoint { (J,M)@T : duration(J,M,T) } :- machine(M).

#show.
