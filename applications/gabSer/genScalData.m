function [E, V] = genScalData( SCALE, SUBGR_PATH_LENGTH, K4approx, batchSize )

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Function genScalData() - Scalable Data Generator.
%
% This scalable data generator produces edge tuples containing the start
% vertex, end vertex, and weight for each directed edge, that represent
% data assigned to the edges of a graph. The edge weights are positive
% integers chosen from a uniform random distribution. The list of tuples
% does not exhibit locality that can be exploited by the computational
% kernels.
%
% The graph generator is based on the Recursive MATrix (R-MAT) power-law
% graph generation algorithm (Chakrabati, Zhan & Faloutsos). This model
% recursively sub-divides the graph into four equal-sized partitions and
% distributes edges within these partitions with unequal probabilities.
% Initially, the adjacency matrix is empty, and edges are added one at a
% time. Each edge chooses one of the four partitions with probabilities
% a, b, c, and d respectively. At each stage of the recursion, the
% parameters are varied slightly and renormalized. It is possible that the
% R-MAT algorithm may create a very small number of multiple edges between
% two vertices, and even self loops. Multiple edges, self-loops, and
% isolated vertices, may be ignored in the subsequent kernels.

% The algorithm also generates the data tuples with high
% degrees of locality. Thus, as a final step, vertex numbers must be
% randomly permuted, and then edge tuples randomly shuffled, before being
% presented to subsequent kernels.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
%
% INPUT
%
% SCALE             - [int] scales the problem size (from user).
%
% SUBGR_PATH_LENGTH - [int] desired path length of subgraph, kernel 3 (user).
%
% K4approx          - [int] binary exponent of the number of times that 
%                     kernel 4 's algorithm is to loop, between 1 and SCALE 
%                     (user).
%
% batchSize         - [int] the number of vertices to process at once, 
%                     kernel 4 (user).
%
% OUTPUT
%
% V.                - [data struct] misc. info for verification:
%   lgN             - [int] rounded down log of number of vertices in set, 
%                     (only for TORUS data).
%   N               - [int] total number of vertices in the tuple set.
%   M               - [int] total number of edges in the tuple set.
%   C               - [int] maximum value of any of the weights.
%
% E.               - [struct] list of weighted edge tuples:
%   StartVertex    - [1xM int array] start vertices.
%   EndVertex      - [1xM int array] end vertices.
%   Weight         - [1xM int array] integer weights.
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

declareGlobals;


% Calcutate the maximum integer value in any edge weight...
C = 2^SCALE;


%--------------------------------------------------------------------------
%  Generate the tuple list.
%--------------------------------------------------------------------------

if (ENABLE_DATA_TYPE == RMAT)  % If random recursive MATrix (R-MAT) power-law
    % graph data was opted...

    %----------------------------------------------------------------------
    %  From the selected SCALE, derive the problem's maximum size.
    %----------------------------------------------------------------------

    % Calculate the maximum number of vertices.
    maxN = 2^SCALE;     % maximum number of vertices in the graph.

    % Calculate the maximum number of edges; adjust at low SCALE values to
    % keep the resultant graph's sparsity below 0.25 (since for the
    % intended ranges of M and N, the sparsity of the graph will be much
    % less than 1).
    maxM = 8 * maxN;    % maximum number of edges in the graph.
    lookupM = [0.25, 0.5, 1, 2, 4];
    if (SCALE < 6)
        maxM = lookupM(SCALE) * maxN;
    end

    %----------------------------------------------------------------------
    %  R-MAT power law distributed
    %----------------------------------------------------------------------

    % Set the R-MAT probabilities. Create a single parameter family.
    % Parameters cannot be symmetric in order for it to be a power law
    % distribution.  (These probabilites will add to '1.0').
    p = 0.6;
    a = .55;   b = .1;   c = .1;   d = .25;

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

    E.Weight = unidrnd(C, 1, maxM)';  % Compute random integer weights.


else % if ENABLE_DATA_TYPE == any torus

    %----------------------------------------------------------------------
    %  Multi-Dimensional Torus
    %----------------------------------------------------------------------

    %D = 1; lgN = 2; % Reasonable ranges are lgN = 2, ..., 18
    %D = 2; lgN = 3; % Reasonable ranges are lgN = 2, ...,  9
    %D = 3; lgN = 2; % Reasonable ranges are lgN = 2, ...,  6
    %D = 4; lgN = 2; % Reasonable ranges are lgN = 2, ...,  4

    D = TORUS_DIM;

    lgN = floor(SCALE/D);  

    baseN = 2;
    N = baseN^lgN;
    
    % For the exactly dialed problem size:
    lgND = repmat(lgN, 1, D);
    lgND(D) = SCALE - sum(lgND(1:(D-1)));
    ND = baseN.^lgND;
    
    % Compute total number vertices.
    Nv = prod(ND);         % number of vertices, n

    % Compute total number edges.
    if ENABLE_DATA_TYPE == HALF_TORUS_1D
        Ne = D*Nv;         % number of edges, m
    else % if ENABLE_DATA_TYPE == any doubly connected torus
        Ne = 2*D*Nv;
    end

    startVertex = 1:Nv;

    % Allocate tuples.
    if ENABLE_DATA_TYPE == HALF_TORUS_1D
        ii = repmat(startVertex,1,D);
    else % if ENABLE_DATA_TYPE == any doubly connected torus
        ii = repmat(startVertex,1,2*D);
    end

    jj = zeros(1,Ne);

    % Create indices.
    switch D
        case 1

            if ENABLE_DATA_TYPE == HALF_TORUS_1D
                % Build a verifyable singly connected torus that grows
                % with SCALE. For vertices 1, 2, 3, .., N, add directed
                % edges 1->2, 2->3,..., N-1->N, and N->1. This is a
                % singly connected 1-D torus with N vertices and M = N
                % edges. The betweenness centrality score for each
                % vertex in this torus will be (N-1)*(N-2)/2.
                [i] = startVertex;
                jj(1:Nv) = mod(i-1+1,ND(1))+1;

            else % this is a doubly connected 1D torus: TORUS_1D
                [i] = startVertex;
                jj(1:Nv) = mod(i-1+1,ND(1))+1;
                jj(Nv+1:2*Nv) = mod(i-1-1,ND(1))+1;
            end

        case 2
            % this is a doubly connected 2D torus: TORUS_2D
            [i j] = ind2sub(ND,startVertex);
            jj(1:Nv) = sub2ind(ND,mod(i-1+1,ND(1))+1,j);
            jj(Nv+1:2*Nv) = sub2ind(ND,mod(i-1-1,ND(1))+1,j);
            jj(2*Nv+1:3*Nv) = sub2ind(ND,i,mod(j-1+1,ND(2))+1);
            jj(3*Nv+1:4*Nv) = sub2ind(ND,i,mod(j-1-1,ND(2))+1);
        case 3
            % this is a doubly connected 3D torus: TORUS_3D
            [i j k] = ind2sub(ND,startVertex);
            jj(1:Nv) = sub2ind(ND,mod(i-1+1,ND(1))+1,j,k);
            jj(Nv+1:2*Nv) = sub2ind(ND,mod(i-1-1,ND(1))+1,j,k);
            jj(2*Nv+1:3*Nv) = sub2ind(ND,i,mod(j-1+1,ND(2))+1,k);
            jj(3*Nv+1:4*Nv) = sub2ind(ND,i,mod(j-1-1,ND(2))+1,k);
            jj(4*Nv+1:5*Nv) = sub2ind(ND,i,j,mod(k-1+1,ND(3))+1);
            jj(5*Nv+1:6*Nv) = sub2ind(ND,i,j,mod(k-1-1,ND(3))+1);
        case 4
            % this is a doubly connected 4D torus: TORUS_4D
            [i j k l] = ind2sub(ND,startVertex);
            jj(1:Nv) = sub2ind(ND,mod(i-1+1,ND(1))+1,j,k,l);
            jj(Nv+1:2*Nv) = sub2ind(ND,mod(i-1-1,ND(1))+1,j,k,l);
            jj(2*Nv+1:3*Nv) = sub2ind(ND,i,mod(j-1+1,ND(2))+1,k,l);
            jj(3*Nv+1:4*Nv) = sub2ind(ND,i,mod(j-1-1,ND(2))+1,k,l);
            jj(4*Nv+1:5*Nv) = sub2ind(ND,i,j,mod(k-1+1,ND(3))+1,l);
            jj(5*Nv+1:6*Nv) = sub2ind(ND,i,j,mod(k-1-1,ND(3))+1,l);
            jj(6*Nv+1:7*Nv) = sub2ind(ND,i,j,k,mod(l-1+1,ND(3))+1);
            jj(7*Nv+1:8*Nv) = sub2ind(ND,i,j,k,mod(l-1-1,ND(3))+1);
    end % of case


    % To be sure not to filter any edges when computing the torus's BC
    % score, limit its edge weights to '7' and below.
    E.Weight = unidrnd(7, 1, Ne)';  % Compute random integer weights.     
        
    V.lgN = lgN;
    maxN = Nv; % Maximum number of vertices.
    maxM = Ne; % Maximum number of edges.

end % of if ENABLE_DATA_TYPE == any torus


if (ENABLE_RANDOM_VERTICES == 1)

    % Randomly permute the vertex numbers.
    randVertices = randperm(maxN);
    V.invRandVert(randVertices) = 1:maxN; % Save its inverse for verif.
    ii = randVertices(ii);
    jj = randVertices(jj);

end

% Build the returned edge tuple data structure.
E.StartVertex = ii;
E.EndVertex   = jj;



%--------------------------------------------------------------------------
%  Measure the actual generated data's size.
%--------------------------------------------------------------------------

% Measure the actual sizes of the resulting data.
actualN = max( max(E.StartVertex), max(E.EndVertex) );
actualM = length(E.StartVertex);

% Count how many vertex pairs are diagonals.
% [ignore cnt] = find(E.StartVertex == E.EndVertex);
% numDiagonals = sum(cnt);

% Also build a struct to be returned with other relevant info for verif.
V.N = actualN;  % total number of vertices in the tuple set.
V.M = actualM;  % total number of edges in the tuple set.
V.C = C;        % maximum integer weight value in the tuple set.

% Compute what will be the eventual graph's sparsity.
graphSparsity = actualM/actualN^2;


%--------------------------------------------------------------------------
%  Echo the current problem's parameters.
%--------------------------------------------------------------------------

% Echo the user's selections on the edge tuple set and on what will be
% the eventual graph:

fprintf([ ...
    '\n\n GRAPH PROBLEM SIZE:  \n\tSCALE == %d,', ...
    '\n\tN == %d (actual vertices), M == %d (actual edges),', ...
    '\n\tgraph''s actual sparsity == %.2f, C == %d (max int weight),'], ...
    SCALE, actualN, actualM, graphSparsity, C);


fprintf('\n\tENABLE_DATA_TYPE == ');
switch ENABLE_DATA_TYPE
    case RMAT
        fprintf('RMAT,');
    case HALF_TORUS_1D
        fprintf('HALF_TORUS_1D,');
    case TORUS_1D
        fprintf('TORUS_1D,');
    case TORUS_2D
        fprintf('TORUS_2D,');
    case TORUS_3D
        fprintf('TORUS_3D,');
    case TORUS_4D
        fprintf('TORUS_4D,');
end


if (ENABLE_DATA_TYPE ~= RMAT)
    
    fprintf(' where lgN == %.2f,', lgN);
    
    fprintf('\n\t  ND = [');
    for ii = 1:length(ND)
        fprintf('%d ', ND(ii));
    end
    fprintf(['\b] (number of vertices per torus''s dimension,', ...
        '\n\t  or the vertex length of each dimension of the torus),\n', ...
        '\t  and, Nv == %d (the product of ND, \n', ...
        '\t  or the total number of vertices),'], Nv);
end


fprintf([ ...
    '\n\tENABLE_RANDOM_VERTICES == %d,', ...
    '\n\tK3 SUBGR_PATH_LENGTH == %d,', ...
    '\n\tK4approx = %d (number of BC iterations, from 1 to SCALE),' ...
    '\n\tbatchSize = %d (number of BC vertices to process at once)'], ...
    ENABLE_RANDOM_VERTICES, SUBGR_PATH_LENGTH, K4approx, batchSize);


fprintf('.\n\n');



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Copyright � 2009, Massachusetts Institute of Technology
% All rights reserved.
% 
% Redistribution and use in source and binary forms, with or without
% modification, are permitted provided that the following conditions are  
% met:
%    * Redistributions of source code must retain the above copyright
%      notice, this list of conditions and the following disclaimer.
%    * Redistributions in binary form must reproduce the above copyright
%      notice, this list of conditions and the following disclaimer in the
%      documentation and/or other materials provided with the distribution.
%    * Neither the name of the Massachusetts Institute of Technology nor  
%      the names of its contributors may be used to endorse or promote 
%      products derived from this software without specific prior written 
%      permission.
%
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
% IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
% THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
% PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
% CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
% EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
% PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
% PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
% LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
% NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   
% SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
