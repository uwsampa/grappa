function verifSearchWeights(maxGraphWeight, maxGraphEdgePair, E, V, G)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Function verifSearchWeights() - verify searchWeights().
%
% First level checks on the functionality of the code in Kernel 2:
% Search Large Sets.  It compares the output of the genScalData()'s
% known data struct 'E' to the output of searchWeights().
%
% Prints results to the Matlab console and plots graph figures.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
% INPUT
% 
% maxGraphWeight   - [int] maximum weight in the graph (from kernel 2).
%
% maxGraphEdgePair - [?x2 int array] one or more vertex pairs corresponding 
%                    to edges with the maxGraphWeight (from kernel 2).
%
% E.               - [struct] list of weighted edge tuples (from SDG):
%   StartVertex    - [1xM int array] start vertices.
%   EndVertex      - [1xM int array] end vertices.
%   Weight         - [1xM int array] integer weights.
%
% V.               - [struct] with misc verif info (from SDG and kernel 1):
%   lgN            - [int] rounded down log of number of vertices in set, 
%                    (only for TORUS data).
%   N              - [int] total number of vertices in the tuple set.
%   M              - [int] total number of edges in the tuple set.
%   C              - [int] maximum integer value of any of the weights.
%
% G.               - [struct] graph (from kernel 1):
%   adjMatrix      - [?x?] sparse weighted adjacency matrix of the graph.
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

declareGlobals;

        
fprintf('\n\tKernel 2 Search Large Sets - Results:\n');


%--------------------------------------------------------------------------
%  Verify max weight
%--------------------------------------------------------------------------

% Compare the results from searchWeights()'s to the results of
% searching over the original SDG's edges tuple structure.

% Find the maximum weight in the edge set.
maxTupleWeight = max(E.Weight);
    
if (maxTupleWeight == maxGraphWeight)
    fprintf('\n\tVERF PASS: The max weight in the tuple set is: \t%4d,', ...
        maxTupleWeight);
    fprintf('\n\twhich is equal to the max weight in the graph: \t%4d.\n', ...
        maxGraphWeight);
else
    fprintf('\n\tVERF FAIL: The max weight in the tuple set is: \t\t%4d,', ...
        maxTupleWeight);
    fprintf('\n\twhich is not equal to the max weight in the graph: \t%4d.\n', ...
        maxGraphWeight);
end


%--------------------------------------------------------------------------
%  Verify edge pairs (with max weight)
%--------------------------------------------------------------------------

% In the edge tuple set, find vertex pairs with the max weighted edges.  
% Note that there can be more than one vertex pair associated with the 
% max weight value. 
i_Edges = find(E.Weight == maxTupleWeight);
maxTupleEdgePair = [E.StartVertex(i_Edges) E.EndVertex(i_Edges)];

% Sort the found edges.
[startEdges startOrder] = sort( ...
    sub2ind([V.N, V.N], maxGraphEdgePair(:,1), maxGraphEdgePair(:,2)));
% Ignore multiple occurrences of the same weight for the same edge.
[verifEdges verifOrder] = unique( ...
    sub2ind([V.N, V.N], maxTupleEdgePair(:,1), maxTupleEdgePair(:,2)));

fail = 0;

[y1 x1] = size(maxGraphEdgePair(startOrder,:));
[y2 x2] = size(maxTupleEdgePair(verifOrder,:));
totGraph  = y1 * x1/2;       % how many are max in graph struct
totTuples = y2 * x2/2;       % how many are max in edges struct

if totGraph == totTuples
    fprintf(['\n\tVERF PASS: The same number of max weighted edges were ', ...
        '\n\tfound in the graph as in the tuple set: \t%.0f.\n'], totGraph);

    if isequal( reshape(maxGraphEdgePair(startOrder,:), totGraph, 2), ...
                reshape(maxTupleEdgePair(verifOrder,:), totTuples, 2) )
        
        fprintf(['\n\tVERF PASS: The same start and end vertex pair sets ', ...
            'were found \n\tfor the max weighted edges in the graph '...
            'as in the tuple set.\n']);
    else
        fail = 1;
        fprintf(['\n\tVERF FAIL: Different start and end vertex pair ', ...
            'sets were found \n\tfor the max weighted edges in graph ', ...
            'as in tuple set:\n']);
    end
    
else
    fail = 1;
    fprintf(['\n\tVERF FAIL: Different number of max weighted edges\n', ...
        '\tin the graph: \t%.0f, than in the tuple set: \t%.0f.\n'], totGraph, totTuples);
end

if (fail || ENABLE_MDUMP)
    
    fprintf('\n\tThe max weighted edge(s) in the graph is(are):\n');
    for ii = 1:totGraph
        fprintf('\t\t%4d. (%d,%d) =\t %d\n', ...
            ii, maxGraphEdgePair(ii,1), maxGraphEdgePair(ii,2), maxGraphWeight);
    end

        
    % TMDB: in singly connected torus (HALF_TORUS), is maxTupleEdgePair 
    % going to be half missing?  Debug this.
%     fprintf('\n\tThe max weighted edge(s) in the tuple set (with no multiple edges) is(are):\n');
%     for ii = 1:totTuples
%         fprintf('\t\t%4d. (%d,%d) =\t %d\n', ...
%             ii, maxTupleEdgePair(ii,1), maxTupleEdgePair(ii,2), maxTupleWeight);
%     end

    
end % of if (fail || ENABLE_MDUMP)

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
