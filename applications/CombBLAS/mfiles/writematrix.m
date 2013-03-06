function writematrix(A,name,isint)

[i,j,k] = find(A);
fid = fopen(name, 'w+');
fprintf(fid, '%d\t%d\t%d\n',size(A,1), size(A,2), nnz(A)); 
if(isint)
    for m = 1:nnz(A)
        fprintf(fid, '%d\t%d\t%d\n',i(m),j(m),k(m)); 
    end
else
    for m = 1:nnz(A)
        fprintf(fid, '%d\t%d\t%.6f\n',i(m),j(m),k(m)); 
    end
end
fclose(fid);

