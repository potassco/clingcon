% Three tasks on one machine, scheduled by two heuristic directives.
#const n = 3.
#const h = 20.

&dom { 0..h } = start(T) :- T = 1..n.
&dom { 0..h } = makespan.

% Every task takes five time units and the next one starts after it.
&sum { start(T); 5 } <= start(T+1) :- T = 1..n-1.
&sum { start(n); 5 } <= makespan.

% The starts are branched on first, from their lower bounds. The makespan
% follows at the lower priority, once the starts are fixed.
&heuristic(0, first_fail, indomain_min) { start(T) : T = 1..n }.
&heuristic(1, input_order, indomain_min) { makespan }.

&minimize { makespan }.
