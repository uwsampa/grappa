function dispElapsedTime(startTime)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function  dispElapsedTime() - prints the elapsed time
%
% Prints the elapsed time, in days, hours, minutes and seconds.
%
% INPUT
% startTime - [1x6 array] initialized by an earlier call to Matlab's CLOCK.
%             CLOCK = [year month day hour minute seconds]
%                     [ int   int int  int    int   float]
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 


endTime = clock; 

elapsedTime = endTime - startTime; 

% get hours
if elapsedTime(4) < 0 
    elapsedTime(3) = elapsedTime(3) - 1;
    elapsedTime(4) = 24 + elapsedTime(4);
end  
% get minutes
if elapsedTime(5) < 0
    elapsedTime(4) = elapsedTime(4) - 1;
    elapsedTime(5) = 60 + elapsedTime(5);
end  
% get seconds
if elapsedTime(6) < 0
    elapsedTime(5) = elapsedTime(5) - 1;
    elapsedTime(6) = 60 + elapsedTime(6);
end  

fprintf(['\n\tTime elapsed: %d day(s), %d hour(s), %d min(s), ', ...
     '%4.2f sec(s)\n'], ...
     elapsedTime(3), elapsedTime(4), elapsedTime(5), elapsedTime(6));




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
