function writecolvector(v,name,isint)

[i,j,k] = find(v);
fid = fopen(name, 'w+');
fprintf(fid, '%d\t%d\n',size(v,1), nnz(v)); 
if(isint)
	for m = 1:nnz(v)
		fprintf(fid, '%d\t%d\n',i(m),k(m)); 
	end
else
	for m = 1:nnz(v)
		fprintf(fid, '%d\t%.5f\n',i(m),k(m)); 
	end
end
fclose(fid);
