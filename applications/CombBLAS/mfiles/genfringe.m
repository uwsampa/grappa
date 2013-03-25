function genfringe(scale, rhs, limit)

size = 2^scale;

i = 1;
while (i < limit)
	name = ['fringe_scale', num2str(scale), '_rect', num2str(rhs), '_sparse', num2str(i)];
	A = sprand(size, rhs, i / size);
	writematrix(A, name);
	i = i*10;
end
