% Knapsack problem: select items to maximize value without exceeding capacity.
% Conditional sums aggregate weight and value only for selected items.

#const capacity = 10.

item(1..5).
weight(1,2). weight(2,3). weight(3,4). weight(4,2). weight(5,5).
value(1,3).  value(2,4).  value(3,5).  value(4,2).  value(5,6).

% choose a subset of items
{ select(I) : item(I) }.

% total weight of selected items must not exceed capacity
&sum { W : select(I), weight(I,W) } <= capacity.

% total value of selected items
&sum { V : select(I), value(I,V) } = total.

% maximize total value (minimize negated)
&minimize { -1*total }.

#show select/1.
