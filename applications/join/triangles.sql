\timing

select E1.src,E2.src,E3.src 
from followedby E1, followedby E2, followedby E3
where E1.dest=E2.src and E2.dest=E3.src and E3.dest=E1.src -- triangle select
and E1.src < E2.src and E2.src < E3.src; -- no duplicates
