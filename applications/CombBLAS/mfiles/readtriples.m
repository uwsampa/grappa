function A = readtriples(inputname)

[fid message] = fopen(inputname,'r');

% XMAT is a 3-by-nnz(X) matrix of triples (i.e. each column is a triple)
AMAT = fscanf(fid,'%d\t%d\t%f\n',[3 inf]);
A = sparse(AMAT(1,2:end),AMAT(2,2:end),AMAT(3,2:end), AMAT(1,1), AMAT(2,1), AMAT(3,1));

