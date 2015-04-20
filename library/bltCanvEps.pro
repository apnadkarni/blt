% -*- mode: tcl; indent-tabs-mode: nil -*- 
%
% bltCanvEps.pro
%
% PostScript encapulator prolog file of the BLT "eps" canvas item.
%
% Copyright 2015 George A. Howlett. All rights reserved.  
%
%   Redistribution and use in source and binary forms, with or without
%   modification, are permitted provided that the following conditions are
%   met:
%
%   1) Redistributions of source code must retain the above copyright
%      notice, this list of conditions and the following disclaimer.
%   2) Redistributions in binary form must reproduce the above copyright
%      notice, this list of conditions and the following disclaimer in the
%      documentation and/or other materials provided with the
%      distribution.
%   3) Neither the name of the authors nor the names of its contributors
%      may be used to endorse or promote products derived from this
%      software without specific prior written permission.
%   4) Products derived from this software may not be called "BLT" nor may
%      "BLT" appear in their names without specific prior written
%      permission from the author.
%
%   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
%   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
%   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
%   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
%   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
%   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
%   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
%   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
%   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
%   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
%   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%

%
% The definitions of the next two macros are from Appendix H of 
% Adobe's "PostScript Language Reference Manual" pp. 709-736.
% 

% Prepare for EPS file

/BeginEPSF {				
  /beforeInclusionState save def
  /dictCount countdictstack def		% Save the # objects in the dictionary
  /opCount count 1 sub def		% Count object on operator stack
  userdict begin			% Make "userdict" the current 
					% dictionary
    /showpage {} def			% Redefine showpage to be null
    0 setgray 
    0 setlinecap
    1 setlinewidth
    0 setlinejoin
    10 setmiterlimit
    [] 0 setdash
    newpath
    /languagellevel where {
      pop languagelevel 
      1 ne {
	false setstrokeadjust false setoverprint
      } if
    } if
    % note: no "end"
} bind def

/EndEPSF { %def
  count opCount sub {
    pop
  } repeat
  countdictstack dictCount sub { 
  end					% Clean up dictionary stack
  } repeat
  beforeInclusionState restore
} bind def


%
% Set up a clip region based upon a bounding box (x1, y1, x2, y2).
%
/SetClipRegion {
  % Stack: x1 y1 x2 y2
  newpath
  4 2 roll moveto
  1 index 0 rlineto
  0 exch rlineto
  neg 0 rlineto
  closepath
  clip
  newpath
} def

