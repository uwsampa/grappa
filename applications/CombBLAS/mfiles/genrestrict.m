function genrestrict(scale)

% generate restriction operation for multigrid
% or similarly the S matrix for graph contraction

size = 2^scale;
order = 2;	% interpolation order
while (order < 10)
	rhs = size/order;	% interpolation order
	name = ['galerkin_scale', num2str(scale), '_order', num2str(order)];
	A = sprand(size, rhs, order/size);	% approximatly 'size' nonzeros
	writematrix(A, name);
	order = order * 2;
end
