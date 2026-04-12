% Optional job scheduling: select jobs to run within a time limit.
% Conditional sums aggregate durations and profits only for selected jobs.

#const limit = 15.

job(1..5).
duration(1,3). duration(2,5). duration(3,4). duration(4,2). duration(5,6).
profit(1,4).   profit(2,7).   profit(3,5).   profit(4,3).   profit(5,8).

% choose which jobs to run
{ run(J) : job(J) }.

% total duration of selected jobs must fit within the time limit
&sum { D : run(J), duration(J,D) } <= limit.

% total profit of selected jobs
&sum { P : run(J), profit(J,P) } = total.

% maximize profit (minimize negated total)
&minimize { -1*total }.

#show run/1.
