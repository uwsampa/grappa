#!/bin/csh
foreach c (1 2 3 4 5 6 7 8)
foreach u (0 1 2 4 8 16 32)
foreach l (9 10 11 12 13 14 15 19 20 21)
#foreach c (1 2)
#foreach u (0 1)
#foreach l (9 10)
  ./carlson 1 $c $u $l
end
end
end
