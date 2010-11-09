function [maxGraphWeight, startSet] = searchWeights(G)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function searchWeights() - Kernel 2 - Search a Large Set.
%
% This function finds the maximum weighted edges over the entire graph.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
% INPUT
%
% G.               - [struct] graph (from kernel 1):
%   adjMatrix      - [?x?] sparse weighted adjacency matrix of the graph.
%
% OUTPUT
%
% maxGraphWeight  - [int] max value of integer weight in entire graph.
%
% startSet        - [2x? int array] one or more vertex pairs 
%                   corresponding to edges with the maxGraphWeight.
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

declareGlobals;


% Find the maximum weight, and turn to a scalar (instead of sparce matrix).
maxGraphWeight = full(max(max(G.adjMatrix)));

% Find the start and end vertices of the edges with the maximum weight.
[AA(:,1) AA(:,2)] = find(G.adjMatrix == maxGraphWeight);

startSet = AA';




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
