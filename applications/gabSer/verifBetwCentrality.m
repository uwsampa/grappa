function verifBetwCentrality( SCALE, timeK4, K4approx, BC, G, V )

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Function verifBetwCentrality() - verify betwCentrality().
%
% Verification of the functionality the code in Kernel 4, for computing
% a graph's betweenness centrality (graph conectivity). (See 2.5).
%
% Prints results to the Matlab console and (if run under Matlab) plots graphs.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
%
% INPUT
%
% SCALE            - [int] scales the problem size (from user).
%
% timeK4           - [float] time it took to execute kernel 4 (from main).
%
% K4approx         - [int] reduces the compute time of kernel 4 (from user).
%
% BC               - [Nx1 array, float] Betw. centrality is a measure of
%                    the importance of a vertex with respect to the
%                    shortest paths between other vertices in the graph
% 	                 that it lies on. C is a list of the centralities
%                    that were computed (ordered by vertex number)
%                    (from kernel 4).
%
% G.               - [struct] graph (from Kernel 1).
%   adjMatrix      - {?x?] sparse weighted adjacency matrix of the graph.
%
% V.               - [struct] with misc info for verif. (SDG and kernel 1):
%   lgN            - [int] rounded down log of number of vertices in set, 
%                    (only for TORUS data).
%   N              - [int] total number of vertices in the tuple set.
%   M              - [int] total number of edges in the tuple set.
%   C              - [int] maximum value of any of the weights.
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

declareGlobals;


fprintf('\n\tKernel 4 Betweenness Centrality - Results:\n');


%--------------------------------------------------------------------------
% Performance metric.
%--------------------------------------------------------------------------

% Compute the rate of the number of Edges traversed per sec...
if (K4approx == SCALE)
    % if the exact number of Betweenness Centrality iterations was used:
    TEPS = 7 * V.N^2 / timeK4;
else
    % if an estimate of the Betweenness Centrality iterations was used:
    TEPS = 7 * V.N * 2^K4approx / timeK4;
end

fprintf('\n\tPerformance metric: %.2f TEPS.\n', TEPS);



%--------------------------------------------------------------------------
% Test 1: % All non-isolated vertices will have a positive betweenness
% centrality score.
%--------------------------------------------------------------------------

if ( (BC >= 0) .* (BC <= V.N*(V.N-1) ) ) > 0
    fprintf(['\n\tVERF PASS Test 1: All non-isolated vertices have a', ...
             '\n\tbetweenness centrality score between 0 and N*(N-1).\n']);
else
    fprintf(['\n\tVERF FAIL Test 1: All non-isolated vertices do not', ...
      '\n\thave a betweenness centrality score between 0 and N*(N-1).\n']);
end


%--------------------------------------------------------------------------
% Test 2. For vertices with the highest betweenness centrality scores,
% there is a direct correlation to their outdegree.
%--------------------------------------------------------------------------

if ENABLE_PLOTS && ENABLE_PLOT_K4

    % Compute the degree of the graph - the number of outgoing
    % edges from each vertex.
    G.outDegree = sum(G.adjMatrix > 0, 1);

    if ENABLE_DATA_TYPE == RMAT

        graphTitle = sprintf(['K4 - VERF Test 2: There is a correlation ', ...
            'between the number of outgoing edges from \na vertex ', ...
            '(or its Out Degree) and its centrality; it should be ',...
            'probabilistically increasing.']);

    else % if any torus

        graphTitle = sprintf(['K4 - VERF Test 2: For any torus, there ', ...
            'should be exactly one point.']);

    end

    hold('on');
    fh = placeFigure();
    scatter(G.outDegree, BC);  % Must be increasing, test with a large graph.
    H = title(graphTitle);
    if ENABLE_DATA_TYPE == RMAT
        if ispc
            set(H, 'FontSize', 7);
        end
    end
    ylabel('Betweenness Centrality Score');
    xlabel('Out Degree');
    hold('off');

    fprintf('\n\tVERF Test 2: (See Figure "K4 - VERF Test 2 ... ".)\n');

else

    fprintf('\n\tVERF Test 2: (Plot dissabled).\n');

end % of ENABLE_PLOTS && ENABLE_PLOT_K4



%--------------------------------------------------------------------------
% Test 3: Equivalence of the length of any shortest path, to
% the sum of the BC scores.
%--------------------------------------------------------------------------

if ENABLE_K4_TEST3

    % Breath First Search (BFS)
    adjacencyMatrix = double(logical(mod(G.adjMatrix,8) > 0));
    totalShortestPaths = 0;

    for ii = 1:V.N

        % Loop over all of the start sets:
        % Create sparse matrix with the start and end vertex along the diagnal.
        newVertices = sparse(1, ii, 1, 1, V.N);
        seenPrevVertices = sparse(1, ii, 1, 1, V.N);

        % Count the distance from the source vertex from which we are doing BFS.
        distance = 0;

        % Start the BFS loop:
        % (while there are still newVertices to analyze)
        while nnz(newVertices) > 0

            % Get the new edges that are connected to the current vertices.
            newVertices = newVertices * adjacencyMatrix;

            % Find the vertex numbers that the found edges connect to.
            % 'newVertices' is a vector, not a matrix. If we've seen them
            % before, multiply by zero (to delete them).
            newVertices = newVertices & ~seenPrevVertices;
            totalShortestPaths = totalShortestPaths + nnz(newVertices) * distance;
            
            % Add only the new vertices in the subgraph set.
            seenPrevVertices = seenPrevVertices + newVertices;

            distance = distance + 1;

        end
    end

    sumOfCentrality = sum(BC);

    if (round(totalShortestPaths) == round(sumOfCentrality))
        fprintf(['\n\tVERF PASS Test 3: The sum of the shortest paths = %8.2f,\n', ...
            '\tis the same as sum of the centrality scores = %.2f.\n'], ...
            totalShortestPaths, sumOfCentrality);
    else
        fprintf(['\n\tVERF FAIL Test 3: The sum of the shortest paths = %f,\n', ...
            '\tis not the same as sum of the centrality scores = %f.\n'], ...
            totalShortestPaths, sumOfCentrality);
        fprintf(['\tLIKELY A FALSE ALARM: This Test Is Under Construction.\n']);
    end


end % of if ENABLE_K4_TEST3



%--------------------------------------------------------------------------
% If using deterministic torus test data.
%--------------------------------------------------------------------------

if ~(ENABLE_DATA_TYPE == RMAT)  % if any torus...

    % 1. For deterministic data, should be able to scramble the vertices,
    % and rerun, and should still get the same BC score (echo on what to
    % try).

    fail = 0;
    lenBC  = length(BC);
    verifBC = zeros(1,V.N);
    lenVBC = length(verifBC);
    
    if ENABLE_DATA_TYPE == HALF_TORUS_1D
        % Half Torus:  For vertices 1, 2, 3, .., N, add directed edges 1->2,
        % 2->3,..., N-1->N, and N->1. This is a connected 1-D torus with N
        % vertices and M = N edges. The betweenness centrality score for each
        % vertex will then be (N-1)*(N-2)/2.
        verifBC(:) = ((V.N - 1) * (V.N - 2)) / 2;
        typeName = 'HALF_TORUS_1D';
    end % of if ENABLE_DATA_TYPE == HALF_TORUS_1D
    
    if ENABLE_DATA_TYPE == TORUS_1D
        verifBC(:) = 1/4 * (-2 + V.N) * V.N - V.N/2 + 1;
        typeName = 'TORUS_1D';
    end % of if ENABLE_DATA_TYPE == TORUS_1D

    if ENABLE_DATA_TYPE == TORUS_2D
        verifBC(:) = (4-mod(SCALE,2)) .* 2.^(SCALE + floor((SCALE-5)./2)) - 2.^SCALE  + 1;
        typeName = 'TORUS_2D';
    end % of if ENABLE_DATA_TYPE == TORUS_2D

    if ENABLE_DATA_TYPE == HALF_TORUS_1D || ...
       ENABLE_DATA_TYPE == TORUS_1D || ...
       ENABLE_DATA_TYPE == TORUS_2D
        if (lenVBC ~= lenBC)
            fail = 1;
            fprintf(['\n\tVERF ', typeName, ' FAIL: Incorrect number of BC scores.', ...
                ' lenVBC = %d, lenBC = %d\n'], lenVBC, lenBC);
        elseif isequal(round(verifBC), round(BC))
            fprintf(['\n\tVERF ', typeName, ' PASS: Each of the BC scores \n', ...
                '\twere correctly computed to be = %8.2f.\n'], ...
                BC(1));
        else
            fail = 1;
            fprintf(['\n\tVERF HALF_TORUS_1D FAIL: Incorrect BC scores.\n', ...
                'verifBC'' and BC:\n']);
        end
    end % of if ENABLE_DATA_TYPE == TORUS

    if (fail || ENABLE_MDUMP)
        fprintf('\n\tverifBC, the desired betweenness centrality (per vertex number):\n\n');
        for ii=1:lenVBC
            fprintf('\t\t%4d. =\t%8.2f\n', ii, verifBC(ii));
        end
        fprintf('\n\BC, the actual betweenness centrality obtained (per vertex number):\n\n');
        for ii=1:lenVBC
            fprintf('\t\t%4d. =\t%8.2f\n', ii, BC(ii));
        end
    end

end % of if ~(ENABLE_DATA_TYPE == RMAT)


if ENABLE_MDUMP
    fprintf('\n\tThe resulting betweenness centrality (per vertex number):\n\n');
    for ii=1:length(BC)
        fprintf('\t\t%4d. =\t%8.2f\n', ii, BC(ii));
    end
end

fprintf('\n');




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
