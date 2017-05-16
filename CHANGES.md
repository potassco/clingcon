3.3.0
 - updated to clingo-5.2.0 refactor-solve branch
 - changed --flatten-optimization default to false
 - changed --permutation-optimization to --distinct-permutation
 - changed --pidgeon-optimization to --distinct-pigeon
 - changed --domain-size to --domain-propagation
 - changed --strict to --dont-care-propagation with opposing meaning
 - changed --sort-descend-coef to --sort-descend-coefficient
 - changed --sort-descend-dom to --sort-descend-domain

3.2.1
 - bugfix with linear preprocessing
3.2.0
 - changed default configuration
   --split-size=-1
   --distinct-to-card=false
   --flatten-optimization=true
   --translate-constraints=10000 
 - simplified incqueens examples
 - fixed bug which prevented multi-level optimization
 - fixed bug in equality-preprocessing
 - fixed bug in --flatten-optimization

3.1.4
 - bugfix for variable size estimation
 - added warnings for unrestricted variables

3.1.3
- fixed bug in equality preprocessing

3.1.2
- fixed reading of non gringo aspif format
- fixed bug in is benchmark class
