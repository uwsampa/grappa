function A = readvector(inputname)

[fid message] = fopen(inputname,'r');

% V is a 2-by-nnz(X) matrix of pairs (i.e. each column is a pair)
V = fscanf(fid,'%d\t\t%f\n',[2 inf]);
A = sparse(V(1,2:end),ones(V(2,1),1),V(2,2:end), V(1,1), 1, V(2,1));

