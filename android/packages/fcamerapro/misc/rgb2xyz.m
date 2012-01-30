 
%clc;
clear all;

% sRGB color space primiaries chromacity coords (xy from xyY)
xy = [0.64 0.33; 0.3 0.6; 0.15 0.06];
% sRGB white point temparature
T = 6500;

% 1) compute white point chromacity coords based on color temparature (valid
% range of temparatures: 1667-25000K)
% if T < 4000    
%     wxc = -0.2661239 * (1000000000 / T ^ 3) - 0.2343580 * (1000000 / T ^ 2) + 0.8776956 * (1000 / T) + 0.179910;    
% else
%     wxc = -3.0258469 * (1000000000 / T ^ 3) + 2.1070379 * (1000000 / T ^ 2) + 0.2226347 * (1000 / T) + 0.240390;
% end
% 
% if T < 2222
%     wyc = -1.1063814 * wxc ^ 3 - 1.34811020 * wxc ^ 2 + 2.18555832 * wxc - 0.20219683;
% elseif T < 4000
%     wyc = -0.9549476 * wxc ^ 3 - 1.37418593 * wxc ^ 2 + 2.09137015 * wxc - 0.16748867;
% else
%     wyc = 3.0817580 * wxc ^ 3 - 5.87338670 * wxc ^ 2 + 3.75112997 * wxc - 0.37001483;
% end

% 2) compute white point coords in CIE 1960 color space (approximation range: 1000-15000K)
%up = (0.860117757 + 1.54118254e-4 * T + 1.28641212e-7 * T ^ 2) / (1 + 8.42420235e-4 * T + 7.08145163e-7 * T ^ 2);
%vp = (0.317398726 + 4.22806245e-5 * T + 4.20481691e-8 * T ^ 2) / (1 - 2.89741816e-5 * T + 1.61456053e-7 * T ^ 2);

% convert to chromacity coords
%wxc = 3 * up / (2 * up - 8 * vp + 4);
%wyc = 2 * vp / (2 * up - 8 * vp + 4);

% 3) correlated color temperature of a CIE D-illuminant to the chromaticity
% of that D-illuminant (valid range: 4000-25000K)
if T < 7000
    wxc = -4.6070e9 / T ^ 3 + 2.9678e6 / T ^ 2 + 0.09911e3 / T + 0.244063;
else
    wxc = -2.0064e9 / T ^ 3 + 1.9018e6 / T ^ 2 + 0.24748e3 / T + 0.237040;
end
wyc = -3.0 * wxc ^ 2 + 2.870 * wxc - 0.275;

% sRGB color space white point in XYZ space
%WP = [0.9505 1 1.0890];
WP = [wxc / wyc 1 (1 - wxc - wyc) / wyc];

% compute color space primaries in XYZ coords
XYZ = [(xy(:,1) ./ xy(:,2))'; 1 1 1; ((1 - xy(:,1) - xy(:,2)) ./ xy(:,2))'];

S = inv(XYZ) * WP';

% conversion matrix RGB->XYZ
M = XYZ .* (S * [1 1 1])'

%% ====================================================================

% validation: color temparature 
c = [1 1 1]';
cxyz = M * c;

cx = cxyz(1) / sum(cxyz);
cy = cxyz(2) / sum(cxyz);

% compute color temp (approximation range: 3000-50000K)
n = (cx - 0.3366) / (cy - 0.1735);
cct = -949.86315 + 6253.80338 * exp(-n / 0.92159) + 28.70599 * exp(-n / 0.20039) + 0.00004 * exp(-n / 0.07125)




