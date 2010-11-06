function subGraphs = findSubgraphs(G, pathLength, startSet)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function findSubgraphs() - Kernel 3 - Graph Extraction.
%
% The third computational kernel produces a series of directed
% subgraphs starting with a specified edge and continuing in the same
% direction for pathLength-1 further steps.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
% INPUT
%
% G                 - [struct] graph (from Kernel 1).
%   adjMatrix       - [?x?] sparse weighted adjacency matrix of the graph.
%
% pathLength        - [int] desired path length of the subgraph (user).
%
% startSet          - [2x? int array] one or more vertex pairs with which 
%                     to start the subGraph(s) (from Kernel 2).
%
%
% OUTPUT
%
% subGraphs         - [struct] cell array of resulting subgraph 
%                     structures, each of data type 'G'.
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


declareGlobals;


N = length(G.adjMatrix);

subGraphs = cell(1, size(startSet, 2));


for ii = 1:size(startSet, 2)

    % Loop over all of the start sets to create a sparse matrix with the 
    % start and end vertex along the diagonal.
    newVertices = sparse([startSet(1,ii), startSet(2,ii)], ...
        [startSet(1,ii), startSet(2,ii)], 1, N, N);

    subGraphs{1, ii} = sparse( [], [], [], N, N );
    
    seenPrevVertices = sparse([1, 1], [startSet(1,ii), startSet(2,ii)], 1, 1, N);

    % Start the BFS loop:
    for sbl = 1:pathLength

        % Get all of the new edges that are connected to the current vertices.
        newEdges = newVertices * G.adjMatrix;

        % Find the vertex numbers that the found edges connect to.
        pathsToVertices = sum(newEdges);

        % If we've seen them before, multiply by zero (to delete them).
        % 'newVerticesFlat' is a vector, not a matrix.
        newVerticesFlat = pathsToVertices .* ~seenPrevVertices;
        
        % Add only the new vertices in the subgraph set.
        seenPrevVertices = seenPrevVertices + newVerticesFlat;
        
        % Create a new set of current vertices.
        newVertices = sparse(1:N, 1:N, double(newVerticesFlat > 0), N, N);
        
        % Get the new edges that are connected to the current vertices.
        subGraphs{1, ii} = subGraphs{1, ii} + newEdges;
        
        % Return the found edges' weights back to what they are in the 
        % original matrix.
        subGraphs{1, ii} = spones(subGraphs{1, ii}) .* G.adjMatrix;
        
    end
end




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
