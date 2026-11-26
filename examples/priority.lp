% Two optimization levels: the cost is minimized first, the delay second.
#const n = 10.

&dom { 0..n } = cost.
&dom { 0..n } = delay.

&sum { cost; delay } >= n.

% The higher level is optimized first, so the optimum is cost=0 delay=n.
&minimize { cost@2; delay@1 }.
