function bc = betwCentrality( G, K4approx, batchSize )

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function betwCentrality() - Kernel 4, analyze a graph's connectivity.
%
% The fourth computational kernel computes the betweenness centrality for 
% an unweighted graph, using only matrix operations. Betweenness centrality 
% is a measure of the importance of a vertex with respect to the shortest 
% paths between other vertices in the graph that it lies on.  This function 
% computes an ordered list of centralities, each centrality corresponding 
% to a specific vertex in the graph. 
%
% The high computational cost of kernel 4:  
% An exact implementation would consider all of the vertices as starting 
% points in the betweenness centrality metric; this implementation 
% can be 'dialed' to use a subset of starting vertices to obtain an
% approximation of the betweenness centrality.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
% NOTES: 
%
% This code is the vectorized version of the pseudo-code provided in the
% specification.  It is designed to process a full level in the search tree 
% at a time rather than just a single vertex.  All of the operations are 
% performed in he same way, only this code is able to perform them in 
% parallel using sparse matrices and matrix operations.  In addition, 
% rather than processing a single vertex at a time, it has a configurable 
% batch size parameter.  While increasing the size of a batch increases the 
% space required by the algorithm, it may also increase the performance.
%
% This uses Ulrik Brandes' Algorithm from "A faster algorithm for
% betweenness centrality", where variables are named in the following way:
%
%   Ulrik Brandes                     This Code
%   -----------------------------------------------------------------------
%   C_B                               bc
%   P, d                              (unused)
%   S, Q                              bfs
%   s                                 batch
%   sigma                             nsp
%   delta                             bcu
%
% S and Q can be stored using the same variable.  This optimization can be
% performed in the original algorithm as well by simply using a vector for
% storage rather than a stack and a queue.  Instead of discarding vertices
% from the top of Q, the vector pointer is advanced.  The stack S
% corresponds to the vertices of the array in reverse order.
%
% bfs is stored as a matrix rather than a vector.  Rather than looking at a
% single vertex at a time, all vertices at a particular depth are examined.
%
% D is not required.  It was used previously to determine the
% distance between two vertices.  In this implementation, this can be
% computed by looking at bfs.  In addition, since all the nodes at a 
% particular depth in the search are examined at the same time, all 
% previously unseen vertices must be on shortest paths.
%
% P is computed rather than stored by selecting edges that go between
% vertices at neighboring depths.
%
% References:
%
% D.A. Bader and K. Madduri, "Parallel Algorithms for Evaluating Centrality 
% Indices in Real-world Networks",  Proc. The 35th International Conference 
% on Parallel Processing (ICPP), Columbus, OH, August 2006.
%
% Ulrik Brandes, "A faster algorithm for betweenness centrality". Journal 
% of Mathematical Sociology, 25(2):163–177, 2001.
%
% L.C. Freeman,  "A set of measures of centrality based on betweenness". 
% Sociometry, 40(1):35–41, 1977.
%
%
% INPUT
%
% G.          - [struct] graph (from kernel 1).
%   adjMatrix - sparse weighted adjacency matrix of the graph.
% K4approx    - [int] binary exponent of the number of times that the 
%               algorithm is to loop, between 1 and SCALE. This 
%               simplification reduces its computational time from O(MN) 
%               to O(M*2^K4approx), which is important when testing large 
%               graphs. It determines the amount of work performed by   
%               kernel 4. When 'K4approx' equals 'SCALE', this 
%               implementation is exact.  Otherwise, distinct vertices  
%               are selected randomly (user).
% batchSize   - [int] the number of vertices to process at once.  The space
%               required by the algorithm increases linearly in this
%               parameter.  While there is no theoretical decrease in
%               runtime by increasing this parameter, in actual
%               implementations performance may increase due to batch
%               processing of the operations.
%
% OUTPUT
%
% bc          - [1D array, float] Betweenness centrality is a measure of  
%               the importance of a vertex with respect to the shortest 
%               paths between other vertices in the graph it lies on. bc is 
% 	            a list of the centralities that were computed (ordered by 
%               vertex number).
% 
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

% Grab in information for the global data
declareGlobals;

% Allocate the data structures and initialize variables:
%   Name     Dimension                        Entries
%   A      : B^(N x N)                        M
%   bfs    : B^(batchSize x N x N)            batchSize x N
%   nsp    : Z+^(batchSize x N)               batchSize x N
%   bcu    : R^(batchSize x N)                batchSize x N
%   nspInv : R^(batchSize x N)                batchSize x N
%   w      : R^(batchSize x N)                batchSize x N
%   fringe : Z+^(batchSize x N)               < batchSize x N
%   bc     : R^(N)                            N
%   batch  : Z+^(batchSize)                   batchSize

% Variable Description:
%   A         : The adjacency matrix.  An entry at (x,y) indicates an edge 
%               coming from vertex x going to vertex y.  Used only in its 
%               boolean form in this computation (unweighted).
%   N         : The number of vertices in the graph.
%   batchSize : The number of vertices to process simultaneously.  
%               Increasing this number increases the amount of storage 
%               required by the algorithm, but may also increase the 
%               performance due to batch processing of the data.
%   batch     : The vertices in the current batch to be processed.
%   bfs       : The breadth-first search tree discovered.  An entry at 
%               (x,y,z) indicates that for the root vertex batch(x), vertex
%               y was discovered at depth z in the breadth-first search.
%   nsp       : The number of shortest paths.  An entry (x,y)=m indicates
%               that for root vertex batch(x), vertex y has m shortest
%               paths to it.
%   bcu       : The centrality updates.  An entry (x,y)=m indicates that
%               root vertex batch(x) contributes m to the betweenness
%               centrality for vertex y.
%   nspInv    : The inverse of the number of shortest paths.  An entry
%               (x,y)=m indicates that for root vertex batch(x), vertex y
%               has 1/m shortest paths to it.
%   w         : The child weights during the centrality update.  An entry
%               (x,y)=m indicates that for root vertex batch(x), child  
%               vertex y applies a weight of m to all its parent vertices 
%               during the centrality update.
%   fringe    : The current open queue of the breadth-first search.  When 
%               the depth is d in the breadth-first search, an entry 
%               (x,y)=m indicates that for root vertex batch(x), vertex y 
%               is at depth d and has m paths going to it.
%   bc        : The centrality scores.  An entry (y)=m indicates that
%               vertex y has a betweenness centrality score of m.
   
% Convert the adjacency matrix to an unweighted graph, filter the edges
A = logical(mod(G.adjMatrix,8) > 0);

% Get the number of vertices of the graph.
N = length(A);

% Initialize the centrality
bc = zeros(1,N);

% Fix any issues with the approximation and get the number of passes
if ENABLE_DATA_TYPE == RMAT
    if (2^K4approx > N) % Cannot perform more than N approximations
        K4approx = floor(log2(N));
    end
    nPasses = 2^K4approx;
else % if a torus, use the max loop count for BC verification
    nPasses = N;
end

% Get the total number of batches
numBatches = ceil(nPasses/batchSize);

for p = 1:numBatches
    % Zero out the BFS
    bfs = [];
    
    % Get the vertices in the current batch
    batch = ((p-1).*batchSize + 1):min(p.*batchSize,N);
    
    % Get the size of the current batch
    curSize = length(batch);
    
    % Set the number of paths to all root vertices to one
    nsp = accumarray([(1:curSize)',batch'],1,[curSize,N]);
    
    % Set the counter for the depth in the BFS
    depth = 0;

    % Set the initial fringe to be the neighbors of the root vertices
    fringe = double(A(batch,:));

    % While there are vertices in the fringe to iterate over
    while nnz(fringe) > 0
        % Increment the depth
        depth = depth + 1;
        % Add in the shortest path counts from the fringe
        nsp = nsp + fringe;
        % Add in the vertices discovered from the fringe to the BFS
        bfs(depth).G = logical(fringe);
        % Compute the the next fringe
        fringe = (fringe * A) .* not(nsp);
    end

    % Free up memory
    clear('fringe');  

    % Pre-compute 1/nsp
    [rows cols vals] = find(nsp);
    if(curSize==1) rows = rows';  cols = cols'; end
    nspInv = accumarray([rows,cols],1./vals,[curSize,N]);

    % Free up memory
    clear('rows','cols','vals');

    % Pre-compute (1+bcUpdate)
    bcu = ones(curSize,N);
    
    % Compute the bc update for all vertices except the sources
    for depth = depth:-1:2
        % Compute the weights to be applied based on the child values
        w = (bfs(depth).G .* nspInv) .* bcu;
        % Apply the child value weights and sum them up over the parents
        % Then apply the weights based on parent values
        bcu = bcu + ((A * w')' .* bfs(depth-1).G) .* nsp;
    end
    
    % Update the bc with the bc update
    bc = bc + sum(bcu,1);

    % Free up memory
    clear('w','nspInv','nsp','bcu','bfs');
end

% Subtract off the additional values added in by precomputation
bc = bc - nPasses;




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Copyright © 2009, Massachusetts Institute of Technology
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
