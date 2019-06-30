%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                         %
%          ###########   ###########   ##########    ##########           %
%         ############  ############  ############  ############          %
%         ##            ##            ##   ##   ##  ##        ##          %
%         ##            ##            ##   ##   ##  ##        ##          %
%         ###########   ####  ######  ##   ##   ##  ##    ######          %
%          ###########  ####  #       ##   ##   ##  ##    #    #          %
%                   ##  ##    ######  ##   ##   ##  ##    #    #          %
%                   ##  ##    #       ##   ##   ##  ##    #    #          %
%         ############  ##### ######  ##   ##   ##  ##### ######          %
%         ###########    ###########  ##   ##   ##   ##########           %
%                                                                         %
%            S E C U R E   M O B I L E   N E T W O R K I N G              %
%                                                                         %
% License:                                                                %
%                                                                         %
% Copyright (c) 2018 Matthias Schulz                                      %
%                                                                         %
% Permission is hereby granted, free of charge, to any person obtaining a %
% copy of this software and associated documentation files (the           %
% "Software"), to deal in the Software without restriction, including     %
% without limitation the rights to use, copy, modify, merge, publish,     %
% distribute, sublicense, and/or sell copies of the Software, and to      %
% permit persons to whom the Software is furnished to do so, subject to   %
% the following conditions:                                               %
%                                                                         %
% 1. The above copyright notice and this permission notice shall be       %
%    include in all copies or substantial portions of the Software.       %
%                                                                         %
% 2. Any use of the Software which results in an academic publication or  %
%    other publication which includes a bibliography must include         %
%    citations to the nexmon project a) and the paper cited under b) or   %
%    the thesis cited under c):                                           %
%                                                                         %
%    a) "Matthias Schulz, Daniel Wegemer and Matthias Hollick. Nexmon:    %
%        The C-based Firmware Patching Framework. https://nexmon.org"     %
%                                                                         %
%    b) "Matthias Schulz, Jakob Link, Francesco Gringoli, and Matthias    %
%        Hollick. Shadow Wi-Fi: Teaching Smart- phones to Transmit Raw    %
%        Signals and to Extract Channel State Information to Implement    %
%        Practical Covert Channels over Wi-Fi. Accepted to appear in      %
%        Proceedings of the 16th ACM International Conference on Mobile   %
%        Systems, Applications, and Services (MobiSys 2018), June 2018."  %
%                                                                         %
%    c) "Matthias Schulz. Teaching Your Wireless Card New Tricks:         %
%        Smartphone Performance and Security Enhancements through Wi-Fi   %
%        Firmware Modifications. Dr.-Ing. thesis, Technische Universität  %
%        Darmstadt, Germany, February 2018."                              %
%                                                                         %
% 3. The Software is not used by, in cooperation with, or on behalf of    %
%    any armed forces, intelligence agencies, reconnaissance agencies,    %
%    defense agencies, offense agencies or any supplier, contractor, or   %
%    research associated.                                                 %
%                                                                         %
% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS %
% OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              %
% MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  %
% IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    %
% CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    %
% TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       %
% SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  %
%                                                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

normalize = @(sig) sig/sqrt(sum(sum(abs(sig).^2))/numel(sig));
nexmonsamples = @(s,m) bitor(bitshift(bitand(int32(real(s) * m), hex2dec('ffff')), 16), ...
                    bitand(int32(imag(s) * m), hex2dec('ffff')));

ieeeenc = ieee_80211_encoder();
ieeeenc.set_rate(1);

mac_packet = [ ...
    '80000000ffffffffffff827bbef096e0827bbef096e0602f54e02a09000000006' ...
    '4001111000f4d79436f766572744368616e6e656c01088c129824b048606c0504' ...
    '0103000007344445202401172801172c01173001173401173801173c011740011' ...
    '764011e68011e6c011e70011e74011e84011e88011e8c011e0020010023021300' ...
    '30140100000fac040100000fac040100000fac0200002d1aef0917ffffff00000' ...
    '000000000000000000000000000000000003d16280f0400000000000000000000' ...
    '0000000000000000007f080000000000000040bf0cb259820feaff0000eaff000' ...
    '0c005012a000000c30402020202dd0700039301770208dd0e0017f20700010106' ...
    '80ea96f0be7bdd090010180200001c0000dd180050f2020101800003a4000027a' ...
    '4000042435e0062322f0046050200010000ce9405ef' ];
mac_data = reshape(de2bi(hex2dec(mac_packet'),4,'left-msb')',1,[]);

ltf_format = 'LTF';

[time_domain_signal_struct, encoded_bit_vector, symbols_tx_mat] = ...
    ieeeenc.create_standard_frame(mac_data, 0, 'LTF', [], [], []);
tx_signal = time_domain_signal_struct.tx_signal;

tx_signal = normalize(tx_signal)*10^(-11/20);
len = length(tx_signal);
tx_signal = [tx_signal(:); zeros(ceil(len/1000)*1000 - len + 2000,1)];

%%
tx_signal_nexmon = nexmonsamples(tx_signal,10000);

% split to chunks of length l
l = 250;
n = length(tx_signal_nexmon)/l;
start_offset = 1500*4;
tx_signal_nexmon_hdr = [ ...
    int32(start_offset + (0:n-1)*4*l); ...  % offsets
    repmat(int32(l * 4),1,n); ...           % length
    reshape(tx_signal_nexmon, l, n) ...     % data
];

NEX_WRITE_TEMPLATE_RAM = 426;
NEX_SDR_START_TRANSMISSION = 427;
NEX_SDR_STOP_TRANSMISSION = 428;

fileID = fopen('myframe.sh','w');

fprintf(fileID, '#!/system/bin/sh\n\n');

for i=1:n
    fprintf(fileID, 'nexutil -s%d -b -l1500 -v%s\n', NEX_WRITE_TEMPLATE_RAM, ...
        matlab.net.base64encode(typecast(tx_signal_nexmon_hdr(:,i),'uint8')));
end

sdr_start_params = [ ...
    int32(length(tx_signal_nexmon)); ...    % num_samps
    int32(start_offset/4); ...              % start_offset
    int32(hex2dec('1001')); ...             % chanspec
    int32(40); ...                          % power_index
    int32(0); ...                           % endless
];

fprintf(fileID, 'nexutil -s%d -b -l20 -v%s\n', NEX_SDR_START_TRANSMISSION, ...
    matlab.net.base64encode(typecast(sdr_start_params(:),'uint8')));

fclose(fileID);
