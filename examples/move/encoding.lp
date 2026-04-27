#const end=20.
#const stepsize=7.
step(0..end).

% domain knowledge
&dom { 0..T*stepsize } = at(T) :- step(T).

% initial state
&sum { at(0) } = 0.

% actions
{ move(T) } :- step(T); T > 0.

% effects
&sum { at(T-1); stepsize } = at(T) :- move(T).

% frame axiom
&sum { at(T-1) } = at(T) :- not move(T); step(T); step(T-1).

% goal: must reach beyond position 100
:- &sum { at(end) } <= 100.

% count total moves with a conditional sum: contributes 1 for each active move
&sum { 1 : move(T), step(T), T > 0 } = num_moves.

% find the plan with fewest moves
&minimize { num_moves }.

#show move/1.
&show { num_moves }.
