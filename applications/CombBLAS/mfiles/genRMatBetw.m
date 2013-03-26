function genRMatBetw(scale)
%%
% Create an RMAT matrix (8 nonzeros per column) with given scale 
% Filter 1/8 of its edges, make it unweighted (boolean), and transpose it
% Write both A and A^T to a file.
% Written by : Aydin BULUC. July 22, 2009
%%

G1 = rmat(scale, true);
A = grsparse(G1);
A = logical(mod(A,8) > 0);		% filter 1/8 of the edges
AT = A';				% transpose the graph

fprintf('Dimensions %d %d\n', size(A,1), size(A,2));

[i1,j1,k1] = find(A);
fid = fopen(['betwinput_scale',num2str(scale)], 'w+');
fprintf(fid, '%d\t%d\t%d\n',size(A,1), size(A,2), nnz(A)); 
for m = 1:nnz(A)
	fprintf(fid, '%d\t%d\t%d\n',i1(m),j1(m),k1(m)); 
end
fclose(fid);


[i1,j1,k1] = find(AT);
fid = fopen(['betwinput_transposed_scale',num2str(scale)], 'w+');
fprintf(fid, '%d\t%d\t%d\n',size(AT,1), size(AT,2), nnz(AT)); 
for m = 1:nnz(AT)
	fprintf(fid, '%d\t%d\t%d\n',i1(m),j1(m),k1(m)); 
end
fclose(fid);
