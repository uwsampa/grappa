function G = rmat (SCALE, RANDOMIZE)
% RMAT
%
% G = rmat (scale)
% G = rmat (scale, randomize)
%
% scale - An input parameter to describe the scale of the graph
% randomize - If true, randomize vertices.
%
% G     - A graph with m=2^SCALE nodes and n=8*2^SCALE edges
%         Edge weights are selected from the normal distribution.
%         Since some edges are repeated, the actual number of edges
%         is less than n.
%
% 5-Jan-07   Viral Shah (Adapted from MIT LL code)

% Calculate the maximum number of vertices.
maxN = 2^SCALE;     % maximum number of vertices in the graph.

% Calculate the maximum number of edges; adjust at low SCALE values to
% keep the resultant graph's sparsity below 0.25 (since for the intended
% ranges of M and N, the sparsity of the graph will be much less than 1).
maxM = 8 * maxN;    % maximum number of edges in the graph.
lookupM = [0.25, 0.5, 1, 2, 4];
if (SCALE < 6 & mod(SCALE,1)==0)
    maxM = lookupM(SCALE) * maxN;
end

%----------------------------------------------------------------------
%  R-MAT power law distributed
%----------------------------------------------------------------------

% Set the R-MAT probabilities. Create a single parameter family.
% Parameters cannot be symmetric in order for it to be a power law
% distribution.  (These probabilites will add to '1.0').
p = 0.6;
a = p;   b = (1 - a)/3;   c = b;   d = b;

% Create index arrays.
ii = ones(maxM, 1);    % start vertices
jj = ones(maxM, 1);    % end vertices

% Loop over each order of bit.
ab = a + b;
c_norm = c/(c + d);
a_norm = a/(a + b);

for ib = 1:SCALE
  % Compare with probabilities, and set bits of indices.
  ii_bit = rand(maxM, 1) > ab;
  jj_bit = rand(maxM, 1) > ( c_norm .* ii_bit + a_norm .* not(ii_bit) );
  
  ii = ii + (2^(ib - 1)) .* ii_bit;
  jj = jj + (2^(ib - 1)) .* jj_bit;
end

if nargin == 2 && RANDOMIZE
  % Randomly permute the vertex numbers.
  randVertices = randperm(maxN);
  ii = randVertices(ii);
  jj = randVertices(jj);  
end

vv = 10*randn (length(ii), 1);
G = graph(sparse (ii, jj, vv, 2^SCALE, 2^SCALE));

