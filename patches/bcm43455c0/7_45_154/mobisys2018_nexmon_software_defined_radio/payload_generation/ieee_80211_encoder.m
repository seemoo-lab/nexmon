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

classdef ieee_80211_encoder < handle
    %IEEE_80211_ENCODER Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        mod_order;      % 2, 4, 16, 64
        code_rate;      % 1/2, 3/4, 2/3;
        rate_bits;      % RATE bits in the SIGNAL field [x x x x]
        k_mod;          % Scaling of the QAM: 0.95, 1/sqrt(2), 1/sqrt(10), 1/sqrt(42)
        ltf_f;          % ltf in the frequency domain
        htltf20_f;      % Long Training Field from 802.11n
        htltf40_f;      % Long Training Field from 802.11n
        stf_f;          % stf in the frequency domain
        
        FS       = 40e6;      % Sampling frequency
        N_FFT    = 128;       % Number of subcarriers
        N_SC     = 48;        % Number of used subcarriers
        CP_LEN   = 1/4;       % Cyclic prefix length relative to N_FFT
        ltf_THRESHOLD = 4;    % Threshold for ltf detection
        %TIMESHIFT     = 5;    % Shift by a certain number of samples into the CP
    end
    
    methods
        function obj = ieee_80211_encoder()
            obj.ltf_f = zeros(1,obj.N_FFT);
            obj.ltf_f(1:27) = [0 1 -1 -1 1 1 -1 1 -1 1 -1 -1 -1 -1 -1 1 1 -1 -1 1 -1 1 -1 1 1 1 1];
            obj.ltf_f((obj.N_FFT-25):obj.N_FFT) = [1 1 -1 -1 1 1 -1 1 -1 1 1 1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1];
            
            obj.htltf20_f = zeros(1,obj.N_FFT);
            obj.htltf20_f(1:29) = [0 1 -1 -1 1 1 -1 1 -1 1 -1 -1 -1 -1 -1 1 1 -1 -1 1 -1 1 -1 1 1 1 1 -1 -1];
            obj.htltf20_f((obj.N_FFT-27):obj.N_FFT) = [1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1];
            
            obj.stf_f = zeros(1,obj.N_FFT);
            obj.stf_f(1:27) = [0 0 0 0 -1-1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0 1+1i 0 0 0 1+1i 0 0 0 1+1i 0 0];
            obj.stf_f((obj.N_FFT-25):obj.N_FFT) = [0 0 1+1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0 -1-1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0];
        end
        
        function set_rate(obj, n)
            switch n
              case 1
                obj.mod_order = 2;          % BPSK modulation
                obj.code_rate   = 1/2;      % Coding rate 1/2 (unpunctured)
                obj.rate_bits   = [1 1 0 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1; 
              case 2                    % 4.5 Mbits/s
                obj.mod_order = 2;
                obj.code_rate   = 3/4;       % Coding rate 3/4 (punctured)
                obj.rate_bits   = [1 1 1 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1;         % 802.11-2012 Table 18.7
              case 3
                obj.mod_order = 4;          % QPSK modulation
                obj.code_rate   = 1/2;       % Coding rate 1/2 (unpunctured)
                obj.rate_bits   = [0 1 0 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(2);
              case 4                    
                obj.mod_order = 4;
                obj.code_rate   = 3/4;       % Coding rate 3/4 (punctured)
                obj.rate_bits   = [0 1 1 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(2); 
              case 5
                obj.mod_order = 16;         % 16-QAM modulation
                obj.code_rate   = 1/2;       % Coding rate 1/2 (unpunctured)
                obj.rate_bits   = [1 0 0 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(10);
              case 6                    % 18 Mbits/s
                obj.mod_order = 16;         % 16-QAM modulation
                obj.code_rate   = 3/4;       % Coding rate 3/4 (punctured)
                obj.rate_bits   = [1 0 1 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(10);
              case 7                    % 24 Mbits/s
                obj.mod_order = 64;         % 64-QAM modulation
                obj.code_rate   = 2/3;       % Coding rate 2/3 (punctured)
                obj.rate_bits   = [0 0 0 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(42);
              case 8                    % 27 Mbits/s
                obj.mod_order = 64;         % 64-QAM modulation
                obj.code_rate   = 3/4;       % Coding rate 3/4 (punctured)
                obj.rate_bits   = [0 0 1 1]; % RATE bits in the SIGNAL field
                obj.k_mod       = 1/sqrt(42);
              otherwise
                error(['Unknown rate: ' num2str(n)]);
            end
        end
        
        function out = generate_signal_field(obj, data)
            %% Create OFDM SIGNAL
            out = false(1,24);
            out(1:4) = obj.rate_bits;               % Insert rate bits
            out(6:17) = de2bi(length(data)/8,12);   % PSDU length in bytes, LSB first
            out(18) = mod(sum(out(1:17)),2);        % Set parity
        end
        
        function out = get_bits_per_subcarrier(obj)
            out = log2(obj.mod_order);
        end
        
        function [out, n_ofdm_syms] = get_data_length(obj, data)
            nCodedBitsPerOFDM = obj.N_SC*obj.get_bits_per_subcarrier();    % Coded bits per OFDM symbol
            nDataBitsPerOFDM  = nCodedBitsPerOFDM*obj.code_rate; % Data bits per OFDM symbol
            n_ofdm_syms = ceil((length(data)+22)/nDataBitsPerOFDM/8)*8;
            out = n_ofdm_syms*nDataBitsPerOFDM;
        end
        
        function out = generate_data_padding(obj, data)
            %data_length = (ceil((length(data)+22)/nDataBitsPerOFDM))*nDataBitsPerOFDM; %Length of PSDU + 16 bits service + 6 bits tail + padding
            %data_length = ceil(data_length/obj.N_SC)*obj.N_SC;
            data_length = obj.get_data_length(data);
            out = false(1,data_length);
            out(17:16+length(data)) = data;
        end
        
        function out = scramble_data(obj, data_padded, data_length)
            scrambling_seq = [ ...
                0 1 1 0 1 1 0 0 0 0 0 1 1 0 0 1 1 0 1 0 1 0 0 1 1 1 0 0 ...
                1 1 1 1 0 1 1 0 1 0 0 0 0 1 0 1 0 1 0 1 1 1 1 1 0 1 0 0 ...
                1 0 1 0 0 0 1 1 0 1 1 1 0 0 0 1 1 1 1 1 1 1 0 0 0 0 1 1 ...
                1 0 1 1 1 1 0 0 1 0 1 1 0 0 1 0 0 1 0 0 0 0 0 0 1 0 0 0 ...
                1 0 0 1 1 0 0 0 1 0 1 1 1 0 1];
            
            data_flipped = reshape(data_padded, 8, []);
            data_flipped = reshape(data_flipped(8:-1:1,:), 1, []);
            
            extended_scrambling_seq = repmat(scrambling_seq, 1, ceil(length(data_flipped)/127));
            
            out = xor(data_flipped, extended_scrambling_seq(1:length(data_flipped)));
            out(data_length+17:data_length+22) = false(1, 6);
        end
        
        function [data_encoded, signal_encoded] = trellis_encode_data_signal_field(obj, data_screambled, signal)
            trellis_encoder = poly2trellis(7,[133 171]);                   % Trellis encoder
            signal_encoded = convenc(signal,trellis_encoder);              % Encoding the SIGNAL field
            data_encoded = convenc(data_screambled,trellis_encoder);       % Encoding the DATA field
            switch obj.code_rate                                           % Puncturing procedure
              case 2/3
                data_encoded(4:4:end)=[];                
              case 3/4
                data_encoded(4:6:end)=[];
                data_encoded(4:5:end)=[];
            end
        end
        
        function [data_interleaved, signal_interleaved] = interleave_data_signal_field(obj, data_encoded, signal_encoded)
            %% Interleaver
            %Inteleave signal ofdm (only one permutation necessary)
            %Index equation from IEEE spec.
            signal_interleaved(48/16*mod((0:47),16)+floor((0:47)/16)+1) = signal_encoded;

            %Interleave data
            nCodedBitsPerOFDM = obj.N_SC*obj.get_bits_per_subcarrier();    % Coded bits per OFDM symbol
            
            % Interleave permutation 1
            k = 0:nCodedBitsPerOFDM-1;
            data_interleaved = reshape(data_encoded, nCodedBitsPerOFDM, []);
            data_interleaved(nCodedBitsPerOFDM/16 * ...
                mod(k,16)+floor(k/16)+1,:) = data_interleaved;
            
            % Interleave permutation 2
            s = obj.get_bits_per_subcarrier()/2;
            data_interleaved(s*floor(k/s) + ...
                mod(k+nCodedBitsPerOFDM-floor(16*k/nCodedBitsPerOFDM),s)+1,:) = data_interleaved;
            
            data_interleaved = reshape(data_interleaved,1,[]);
        end
        
        function out = encode_bit_vector(obj, data)
            out.signal = obj.generate_signal_field(data);
            out.data_padded = obj.generate_data_padding(data);
            out.data_scrambled = obj.scramble_data(out.data_padded, length(data));
            [out.data_encoded, out.signal_encoded] = ...
                obj.trellis_encode_data_signal_field(out.data_scrambled, out.signal);
            [out.data_interleaved, out.signal_interleaved] = ...
                obj.interleave_data_signal_field(out.data_encoded, out.signal_encoded);
        end
        
        function [data_mapped] = map_to_symbols(obj, data, mod_order)
            data_mapped = modx(mod_order,bi2de(reshape(data,log2(mod_order),[]).','left-msb')).';
        end
        
        function [data_demapped] = demap_to_data(obj, data_mapped, mod_order)
            data_demapped = reshape(de2bi(demodx(mod_order, data_mapped),log2(mod_order),'left-msb').',1,[]);
        end
        
        function [data_mapped, signal_mapped] = map_to_symbols_data_signal_field(obj, data_interleaved, signal_interleaved)
            %Modulate signal block, always BPSK
            signal_mapped = (signal_interleaved*2-1)+0i; % Just direct BPSK mapping

            data_mapped = obj.map_to_symbols(data_interleaved, obj.mod_order);
        end
        
        function [preamble, stf_t_pre, ltf_t_pre] = create_preamble(obj, stf_phase_shift, ltf_format)
            %% Create Preamble
            %stf
            stf_f = obj.stf_f * exp(1j*stf_phase_shift);
            stf_t = ifft(sqrt(13/6).*stf_f, obj.N_FFT);
            stf_t = stf_t(1:(obj.N_FFT/4));
            stf_t_pre = repmat(stf_t, 1, 10);
            stf_t_pre(1,1) = stf_t_pre(1,1)*0.5;                    %Windowing

            %ltf for CFO and channel estimation
            switch (ltf_format)
                case 'LTF'
                    ltf_t = ifft(obj.ltf_f, obj.N_FFT);
                case 'HT-LTF'
                    ltf_t = ifft(obj.htltf20_f, obj.N_FFT);
            end
            ltf_t_pre = [ltf_t((obj.N_FFT/2+1):obj.N_FFT) ltf_t ltf_t];
            ltf_t_pre(1,1) = ltf_t_pre(1,1)*0.5 + stf_t_pre(1,1);  %Windowing

            %Preamble composition
            preamble = [stf_t_pre  ltf_t_pre];
        end
        
        function ofdm_syms_pilot_mapped_mat = create_ofdm_symbols_in_frequency_domain(obj, data_mapped, signal_mapped)
            nOfdmSymbols = (length(data_mapped)/obj.N_SC)+1;  %Number of OFDM symbols
            
            pilot_sequence = [ ...
                 1, 1, 1, 1,-1,-1,-1, 1,-1,-1,-1,-1, 1, 1,-1, 1, ...
                -1,-1, 1, 1,-1, 1, 1,-1, 1, 1, 1, 1, 1, 1,-1, 1, ...
                 1, 1,-1, 1, 1,-1,-1, 1, 1, 1,-1, 1,-1,-1,-1, 1, ...
                -1, 1,-1,-1, 1,-1,-1, 1, 1, 1, 1, 1,-1,-1, 1, 1, ...
                -1,-1, 1,-1, 1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1, ...
                -1, 1,-1,-1, 1,-1, 1, 1, 1, 1,-1, 1,-1, 1,-1, 1, ...
                -1,-1,-1,-1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1, 1,-1, ...
                -1, 1,-1,-1,-1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1]; 
            
            pilot_length = ceil(nOfdmSymbols/length(pilot_sequence));      % Number of copies needed
            
            pilot_sequence_long = repmat(pilot_sequence,1,pilot_length);   % Generate enough copies
            pilot_sequence_long = pilot_sequence_long(1:nOfdmSymbols);
            
            % Reshape the symbol vector to a matrix with one column per OFDM symbol
            ofdm_syms_mat = reshape([signal_mapped data_mapped], obj.N_SC, ...
                nOfdmSymbols);
            
            % Matrix containing data and pilot subcarriers mapped to the
            % correct frequencies
            ofdm_syms_pilot_mapped_mat = zeros(obj.N_FFT, nOfdmSymbols);
            % Distribute data symbols directly FFT shifted
            ofdm_syms_pilot_mapped_mat([2:7 9:21 23:27 obj.N_FFT-64+[39:43 45:57 59:64]],:) = ...
                ofdm_syms_mat([25:48 1:24],:);
            ofdm_syms_pilot_mapped_mat(8,:) = pilot_sequence_long;              % pilot 3
            ofdm_syms_pilot_mapped_mat(22,:) = -pilot_sequence_long;            % pilot 4
            ofdm_syms_pilot_mapped_mat(obj.N_FFT-20,:) = pilot_sequence_long;   % pilot 1
            ofdm_syms_pilot_mapped_mat(obj.N_FFT-6,:) = pilot_sequence_long;    % pilot 2
        end
        
        function out = create_time_domain_signal(obj, ofdm_syms_pilot_mapped_mat, cp_replacement)
            % Perform IFFT
            out.ofdm_time_domain = ifft(ofdm_syms_pilot_mapped_mat, obj.N_FFT);

            % Cylic prefix insertion
            out.ofdm_time_domain_cp = ...
                out.ofdm_time_domain([(end-(obj.CP_LEN*obj.N_FFT)+1):end 1:end], :);
            
            if ~isempty(cp_replacement)
                out.ofdm_time_domain_cp(cp_replacement.mask,:) = ...
                    cp_replacement.time_domain_cp;
            end

            % Windowing of first and last sample
            %out.ofdm_time_domain_cp([1 end],:) = ...
            %    out.ofdm_time_domain_cp([1 end],:)*0.5;
            
            % Reshape to a vector
            out.ofdm_time_domain_signal = out.ofdm_time_domain_cp(:).';
        end
        
        function [time_domain_signal_struct, encoded_bit_vector, symbols_tx_mat] = ...
                create_standard_frame(obj, data, stf_phase_shift, ltf_format, data_mapped_plus, dirty_constellation_symbols, cp_replacement)
            % take bits from mac frame and pad, scramble and interleave them
            % also create a signal field
            encoded_bit_vector = obj.encode_bit_vector(data);

            % map bits to qam symbols
            [data_mapped, signal_mapped] = obj.map_to_symbols_data_signal_field( ...
                encoded_bit_vector.data_interleaved, encoded_bit_vector.signal_interleaved);
            
            ofdm_syms_pilot_mapped_mat = ...
                obj.create_ofdm_symbols_in_frequency_domain(data_mapped, signal_mapped);
            
            % Add symbols to additional subcarriers
            if size(data_mapped_plus,2) > 0
                ofdm_syms_pilot_mapped_mat(:,1:size(data_mapped_plus,2)) = ...
                    ofdm_syms_pilot_mapped_mat(:,1:size(data_mapped_plus,2)) + ...
                    data_mapped_plus;
            end
            
            % Add dirty constellation symbols to the existing symbols
            if size(dirty_constellation_symbols,2) > 0
                ofdm_syms_pilot_mapped_clean_mat = ...
                    ofdm_syms_pilot_mapped_mat;
                % 1+ to start after the signal field
                ofdm_syms_pilot_mapped_mat(:,1+(1:size(dirty_constellation_symbols,2))) = ...
                    ofdm_syms_pilot_mapped_mat(:,1+(1:size(dirty_constellation_symbols,2))) + ...
                    dirty_constellation_symbols;
            end
            
            
            
            % take mapped symbols and distribute them to subcarriers together with
            % pilots
            time_domain_signal_struct = obj.create_time_domain_signal( ...
                ofdm_syms_pilot_mapped_mat, cp_replacement);
            
            time_domain_signal_struct.ofdm_syms_pilot_mapped_mat = ...
                ofdm_syms_pilot_mapped_mat;
            
            if size(dirty_constellation_symbols,2) > 0
                time_domain_signal_struct.ofdm_syms_pilot_mapped_dirty_mat = ...
                    ofdm_syms_pilot_mapped_mat;
                time_domain_signal_struct.ofdm_syms_pilot_mapped_mat = ...
                    ofdm_syms_pilot_mapped_clean_mat;
            end

            preamble = obj.create_preamble(stf_phase_shift, ltf_format);
            
            time_domain_signal_struct.preamble = preamble;
            
            time_domain_signal_struct.tx_signal = ...
                [preamble time_domain_signal_struct.ofdm_time_domain_signal];
            
            symbols_tx_mat([25:48 1:24],:) = ...
                time_domain_signal_struct.ofdm_syms_pilot_mapped_mat( ...
                [2:7 9:21 23:27 obj.N_FFT-64+[39:43 45:57 59:64]],:);
        end
        
        function out = addCFO(obj, signal, offset)
            % regular CFO (offset in Hz)
            out = signal .* exp(2j*pi*offset*(1:length(signal))/obj.FS);

            % FSK modulated CFO
            %rnd_syms = randi([0 1],1,107);
            %out = rx_signal .* exp(2j*pi*kron([1 1 1 1 1-(0.01*(rnd_syms*2-1))],repmat(100000,1,160)).*(1:length(signal))/obj.FS);

            % FM Modulated CFO
            %fm_bb_sig = cos(2*pi*10000*(1:length(rx_signal))/fs);
            %rx_signal = rx_signal .* exp(2j * pi*100000/fs * cumsum(fm_bb_sig));
        end
        
        function [rx_signal, rx_RSSI, rx_gains, frame] = apply_channel_effects(obj, channel_model, signal, cfo, snrdb, shift, doppler_spread, warp_settings, mac_data, used_subcarrier_ratio)
            frame = [];
            
            % doppler_spread: Doppler spread at 2.4 GHz = 15 Hz Section 3.5.2 (next generation ...)
            if strcmp(channel_model, 'WARP')
                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                % Set up the WARPLab experiment
                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                USE_AGC = warp_settings.agc;
                NUMNODES = 2;

                %Create a vector of node objects
                nodes = wl_initNodes(NUMNODES);

                node_tx = nodes(1);
                node_rx = nodes(2);

                %Create a UDP broadcast trigger and tell each node to be ready for it
                eth_trig = wl_trigger_eth_udp_broadcast;
                wl_triggerManagerCmd(nodes,'add_ethernet_trigger',eth_trig);

                % Read Trigger IDs into workspace
                [T_IN_ETH_A, T_IN_ENERGY, T_IN_AGCDONE, T_IN_REG, T_IN_D0, T_IN_D1, T_IN_D2, T_IN_D3, T_IN_ETH_B] =  wl_getTriggerInputIDs(nodes(1));
                [T_OUT_BASEBAND, T_OUT_AGC, T_OUT_D0, T_OUT_D1, T_OUT_D2, T_OUT_D3] = wl_getTriggerOutputIDs(nodes(1));

                % For both nodes, we will allow Ethernet to trigger the buffer baseband and the AGC
                wl_triggerManagerCmd(nodes, 'output_config_input_selection', [T_OUT_BASEBAND, T_OUT_AGC], [T_IN_ETH_A, T_IN_REG]);

                % Set the trigger output delays. 
                nodes.wl_triggerManagerCmd('output_config_delay', [T_OUT_BASEBAND], 0);      
                nodes.wl_triggerManagerCmd('output_config_delay', [T_OUT_AGC], 3000);     %3000 ns delay before starting the AGC

                %Get IDs for the interfaces on the boards. Since this example assumes each
                %board has the same interface capabilities, we only need to get the IDs
                %from one of the boards
                [RFA,RFB] = wl_getInterfaceIDs(nodes(1));
                RF_TX = RFA;
                RF_RX = RFB;

                %Set up the interface for the experiment
                TxGainBB = warp_settings.txgainbb; % [0,1,2,3] [-5, -3, -1.5, 0]dB
                TxGainRF = warp_settings.txgainrf; % [0:63] [0:31]dB
                wl_interfaceCmd(nodes,'RF_ALL','tx_gains',TxGainBB,TxGainRF);
                wl_interfaceCmd(nodes,'RF_ALL','channel',2.4,warp_settings.wifi_channel);

                if(USE_AGC)
                    wl_interfaceCmd(nodes,'RF_ALL','rx_gain_mode','automatic');
                    wl_basebandCmd(nodes,'agc_target',-10);
                else
                    wl_interfaceCmd(nodes,'RF_ALL','rx_gain_mode','manual');
                    RxGainRF = warp_settings.rxgainrf; %Rx RF Gain in [1:3] [0,15,30]dB
                    RxGainBB = warp_settings.rxgainbb; %Rx Baseband Gain in [0:31] [0:63]dB
                    wl_interfaceCmd(nodes,'RF_ALL','rx_gains',RxGainRF,RxGainBB);
                end
                
                % Prepare preframe to occupy the channel
                ieeeenc = ieee_80211_encoder();
                ieeeenc.set_rate(1);
                x8 = @(x) uint8(hex2dec(reshape(x,2,[])')');
                clear mac_header llc_header mac_frame
                mac_header.frame_control_1 = bi2de([ 1 0 1 1 0 1 0 0 ],'left-msb'); % RTS
                mac_header.frame_control_2 = bi2de([ 0 0 0 0 0 0 0 0 ],'left-msb');
                mac_header.duration = x8('FF7F'); % To reserver the channel
                mac_header.address_1 = x8('DEADBEEF0000');
                mac_header.address_2 = x8('DEADBEEF0101');
                mac_frame.mac_header = struct2array(mac_header);
                mac_frame.fcs = x8(calculate_80211_fcs(struct2array(mac_frame)));
                rts_frame = reshape(logical(de2bi(struct2array(mac_frame),8,'left-msb')'),1,[]);
                
                preframe_signal = ...
                    ieeeenc.create_standard_frame(rts_frame,0,'LTF',[],[],[]).tx_signal;
                sifs_wait_signal = zeros(1,16e-6*obj.FS);
                
                normalize = @(sig) sig/sqrt(sum(sum(abs(sig).^2))/numel(sig));
                preframe_signal = normalize(preframe_signal)*10^(-11/20);
                
                signal_with_preframe = [preframe_signal sifs_wait_signal signal sifs_wait_signal];

                %Set up the baseband for the experiment
                wl_basebandCmd(nodes,'tx_delay',0);
                wl_basebandCmd(nodes,'tx_length',length(signal_with_preframe));
                wl_basebandCmd(nodes,'rx_length',length(signal_with_preframe)+100);

                wl_basebandCmd(node_tx,RF_TX, 'write_IQ', signal_with_preframe(:));
                wl_interfaceCmd(node_tx,RF_TX,'tx_en');
                wl_interfaceCmd(node_rx,RF_RX,'rx_en');

                wl_basebandCmd(node_tx,RF_TX,'tx_buff_en');
                wl_basebandCmd(node_rx,RF_RX,'rx_buff_en');

                %tcp = tcpclient('10.0.0.55', 8888);
                %tcp.Timeout = 5;
                % +26 for the radiotap header
                %write(tcp, uint16(length(mac_data)/8+26));
                %pause(0.5);
                
                %for k = 1:3
                %eth_trig.send();
                %end
                
                %len = read(tcp,1,'uint32');
                
                %frame.radio_tap.header_revision = read(tcp,1,'uint8');
                %frame.radio_tap.header_pad = read(tcp,1,'uint8');
                %frame.radio_tap.header_length = read(tcp,1,'uint16');
                %frame.radio_tap.present_flags = read(tcp,1,'uint32');
                %frame.radio_tap.mac_timestamp = read(tcp,1,'uint64');
                %frame.radio_tap.flags = read(tcp,1,'uint8');
                %frame.radio_tap.datarate = read(tcp,1,'uint8');
                %frame.radio_tap.channel_frequency = read(tcp,1,'uint16');
                %frame.radio_tap.channel_type = read(tcp,1,'uint16');
                %frame.radio_tap.ssi_signal = read(tcp,1,'uint8');
                %frame.radio_tap.antenna = read(tcp,1,'uint8');
                %frame.radio_tap.rx_flags = read(tcp,1,'uint16');
                
                %frame.payload = read(tcp, len - 26);
                
                %clear tcp;

                Ts = 1/(wl_basebandCmd(nodes(1),'tx_buff_clk_freq'));
                Ts_RSSI = 1/(wl_basebandCmd(nodes(1),'rx_rssi_clk_freq'));
                rx_signal = wl_basebandCmd(node_rx,RF_RX,'read_IQ', 0, length(signal_with_preframe)+100);
                
                if false
                    figure(23)
                    subplot(3,1,1)
                    plot(abs(signal_with_preframe))
                    subplot(3,1,2)
                    plot(abs(rx_signal))
                    hold on
                    plot([1 1] * length(preframe_signal)+length(sifs_wait_signal), [0 1],'r');
                    hold off
                end
                
                %max(max(real(5*signal_with_preframe),imag(5*signal_with_preframe)))
                rx_signal = rx_signal(length(preframe_signal)+length(sifs_wait_signal)+(1:length(signal)+100));
                rx_RSSI = wl_basebandCmd(node_rx,RF_RX,'read_RSSI',0,length(signal)+100/(Ts_RSSI/Ts));
                rx_gains = wl_basebandCmd(node_rx,RF_RX,'agc_state');

                wl_basebandCmd(nodes,'RF_ALL','tx_rx_buff_dis');
                wl_interfaceCmd(nodes,'RF_ALL','tx_rx_dis');
            elseif strcmp(channel_model,'PASSTHROUGH')
                rx_signal = signal;
                rx_RSSI = 0;
                rx_gains = 0;
            else
                switch channel_model
                    case 'B' % Residential
                        trms = 15e-9; % table 3.1 channel models (next generation ...)
                        resample_factor = 4;
                    case 'D' % Typical Office
                        trms = 50e-9;
                        resample_factor = 1;
                    case 'E' % Large Office
                        trms = 100e-9;
                        resample_factor = 1;
                end

                if ~strcmp(channel_model,'AWGN')
                    chan = stdchan(1/obj.FS/resample_factor, doppler_spread, '802.11g', trms);
                    chan.NormalizePathGains = 1;

                    % Noise and CFO are still missing
                    %tttt=tic;
                    rx_signal = resample(filter(chan, resample(signal,resample_factor,1)),1,resample_factor);
                    %toc(tttt)
                else
                    rx_signal = signal;
                end

                % apply CFO (cfo in Hz)
                rx_signal = obj.addCFO(rx_signal, cfo);

                % apply AWGN (snrdb in dB)
                signal_power = 20*log10(rms(rx_signal));
                %disp(signal_power); % xxx
                %rx_signal = awgn(rx_signal,snrdb,'measured');
                %rx_signal = awgn(rx_signal,snrdb,-11);
                rx_signal = awgn(rx_signal,snrdb,signal_power-10*log10(used_subcarrier_ratio)); % xxx

                % apply shift in time domain
                if shift > 0
                    rx_signal = [wgn(1,shift,signal_power-snrdb) rx_signal];
                end
                
                rx_RSSI = 0;
                rx_gains = 0;
            end
        end
        
        function [signal, cfoest] = estimate_and_correct_cfo(obj, signal, start_of_stf)
            signal = reshape(signal,1,[]);
            x = 0:16:200;
            stf_segment_length = obj.N_FFT/4;
            cfoest = zeros(size(x));
            for x_idx = 1:length(x)
                cfoest(x_idx) = angle(signal(start_of_stf + stf_segment_length+x(x_idx)+(1:stf_segment_length)) * ...
                    signal(start_of_stf + stf_segment_length+x(x_idx)+(stf_segment_length+1:(2*stf_segment_length)))')/2/pi/stf_segment_length*obj.FS;
            end
            signal = signal .* exp(2j*pi * mean(cfoest) * (1:length(signal))/obj.FS);
        end
        
        function position = find_frame_positions(obj, signal)
            ltf_t = ifft(obj.ltf_f, obj.N_FFT);
            
            xxx = 1:length(signal);
            yyy = xxx(abs(conv(sign(signal), ltf_t)) > obj.ltf_THRESHOLD);
            if numel(yyy) == 0
                position.stf = 0;
                %disp('FRAME POSITION NOT FOUND');
            else
                position.stf = yyy(1)-512;
                %disp(position.stf);
            end
            position.ltf = position.stf + 320;
            position.signal_field = position.stf + 640;
        end
        
        function [Hest, Hest_HTLTF, Hest1, Hest2] = estimate_channel(obj, position, signal, timeshift)
            Hest1 = fft(signal((position.ltf+obj.N_FFT/2)-timeshift+(1:obj.N_FFT)),obj.N_FFT);
            Hest2 = fft(signal((position.ltf+obj.N_FFT/2)-timeshift+obj.N_FFT+(1:obj.N_FFT)),obj.N_FFT);
            
            Hest1_HTLTF = Hest1.*obj.htltf20_f;
            Hest2_HTLTF = Hest2.*obj.htltf20_f;
            Hest_HTLTF = (Hest1_HTLTF+Hest2_HTLTF)/2;
            
            Hest1 = Hest1.*obj.ltf_f;
            Hest2 = Hest2.*obj.ltf_f;
            Hest = (Hest1+Hest2)/2;
        end
        
        function [symbols_mat_f, symbols_mat_t, cp_mat_t] = ...
                ofdm_demodulate(obj, position, signal, timeshift)
            signal_field_start = position.signal_field - timeshift;
            symbol_cp_length = (1 + obj.CP_LEN) * obj.N_FFT;
            cp_length = obj.CP_LEN * obj.N_FFT;
            max_num_sym = floor((length(signal)-(signal_field_start)) / ...
                (symbol_cp_length));
            symbols_cp_mat_t = reshape(signal((signal_field_start) + (1 : ...
                (max_num_sym * symbol_cp_length))), ...
                symbol_cp_length, []);

            cp_mat_t = symbols_cp_mat_t(1:cp_length,:);
            symbols_mat_t = symbols_cp_mat_t(cp_length+1:end,:);

            symbols_mat_f = fft(symbols_mat_t, obj.N_FFT);
        end
        
        function [stf_f_eq, stf_f, stf_phase_shift_est] = ...
                extract_stf_and_phase_shift(obj, position, signal, timeshift, Hest)
            last_stf_start = position.ltf - timeshift - obj.N_FFT;
            stf_f = fft(signal(last_stf_start+(1:obj.N_FFT)), obj.N_FFT);
            stf_f_eq = stf_f./Hest;
            stf_phase_shift_est = ...
                angle(mean(stf_f_eq(abs(obj.stf_f)>.5) ./ ...
                obj.stf_f(abs(obj.stf_f)>.5)));
        end
        
        function [symbols_eq_mat, symbols_eq_alt_mat, symbols_eq_mat_f, symbols_eq_alt_mat_f, Hest_pilots, Hest_pilots_interp] = ...
                equalize_channel(obj, symbols_mat_f, Hest)
            symbols_eq_mat_f = symbols_mat_f ./ repmat(Hest.',1,size(symbols_mat_f,2));
            
            pilot_sequence = [ ...
                 1, 1, 1, 1,-1,-1,-1, 1,-1,-1,-1,-1, 1, 1,-1, 1, ...
                -1,-1, 1, 1,-1, 1, 1,-1, 1, 1, 1, 1, 1, 1,-1, 1, ...
                 1, 1,-1, 1, 1,-1,-1, 1, 1, 1,-1, 1,-1,-1,-1, 1, ...
                -1, 1,-1,-1, 1,-1,-1, 1, 1, 1, 1, 1,-1,-1, 1, 1, ...
                -1,-1, 1,-1, 1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1, ...
                -1, 1,-1,-1, 1,-1, 1, 1, 1, 1,-1, 1,-1, 1,-1, 1, ...
                -1,-1,-1,-1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1, 1,-1, ...
                -1, 1,-1,-1,-1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1]; 

            pilot_length = ceil(size(symbols_eq_mat_f,2)/length(pilot_sequence));      % Number of copies needed

            pilot_sequence_long = repmat(pilot_sequence,1,pilot_length);   % Generate enough copies
            pilot_sequence_long = pilot_sequence_long(1:size(symbols_eq_mat_f,2));

            pilots_eq_mat = symbols_eq_mat_f([obj.N_FFT-20 obj.N_FFT-6 8 22],:);

            Hest_pilots = pilots_eq_mat ./ [pilot_sequence_long;pilot_sequence_long;pilot_sequence_long;-pilot_sequence_long];
            
            %xxx=fftshift(1:128);
            %Hest_pilots_interp = fftshift(interp1(xxx([obj.N_FFT-20 obj.N_FFT-6 8 22]), Hest_pilots,1:obj.N_FFT,'spline'));
            %Hest_pilots_interp = interp1([obj.N_FFT-20 obj.N_FFT-6 8 22], Hest_pilots,1:obj.N_FFT,'spline');
            %B=fir1(20,0.01,'low');
            %Hest_pilots = filtfilt(B,1,Hest_pilots.').';
            %Hest_pilots = repmat(mean(Hest_pilots,2),1,107);
            %Hest_pilots(abs(Hest_pilots) > 1.4) = 1;
            %Hest_pilots(abs(Hest_pilots) < 0.6) = 1;
            Hest_pilots_interp = interp1([[128-20 128-6 8 22]-128 128-20 128-6 8 22 [128-20 128-6 8 22]+128], repmat(Hest_pilots,3,1),1:128,'spline');
            
            symbols_eq_alt_mat_f = symbols_eq_mat_f./Hest_pilots_interp;
            
            symbols_eq_mat = zeros(obj.N_SC, size(symbols_eq_mat_f,2));
            symbols_eq_mat([25:48 1:24],:) = ...
                symbols_eq_mat_f([2:7 9:21 23:27 obj.N_FFT-64+[39:43 45:57 59:64]],:);

            symbols_eq_alt_mat = zeros(obj.N_SC, size(symbols_eq_alt_mat_f,2));
            symbols_eq_alt_mat([25:48 1:24],:) = ...
                symbols_eq_alt_mat_f([2:7 9:21 23:27 obj.N_FFT-64+[39:43 45:57 59:64]],:);
        end
        
        function [signal_field_decoded, signal_field_demapped, signal_field_deinterleaved] = ...
                demap_and_decode_signal_field(obj, symbols_eq_mat)
            signal_field_demapped = symbols_eq_mat(:,1) > 0;
            signal_field_deinterleaved = signal_field_demapped(48/16*mod((0:47),16)+floor((0:47)/16)+1).';

            trellis_encoder = poly2trellis(7,[133 171]);
            signal_field_decoded.bits = circshift(...
                vitdec(signal_field_deinterleaved, trellis_encoder, 6, 'cont', 'hard'),-6,2);

            signal_field_decoded.rate = signal_field_decoded.bits(1:4);
            signal_field_decoded.reserved = signal_field_decoded.bits(5);
            signal_field_decoded.length_bits = signal_field_decoded.bits(6:17);
            signal_field_decoded.length_in_bits = bi2de(signal_field_decoded.length_bits)*8;
            signal_field_decoded.parity = signal_field_decoded.bits(18);

            % Check parity
            signal_field_decoded.parity_check = ...
                mod(sum(signal_field_decoded.bits(1:17)),2) == signal_field_decoded.parity;

            switch bi2de(signal_field_decoded.rate)
                case bi2de([1 1 0 1])
                    signal_field_decoded.mod_order   = 2;          % BPSK modulation
                    signal_field_decoded.code_rate   = 1/2;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1; 

                case bi2de([1 1 1 1])
                    signal_field_decoded.mod_order   = 2;          % BPSK modulation
                    signal_field_decoded.code_rate   = 3/4;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1; 

                case bi2de([0 1 0 1])
                    signal_field_decoded.mod_order   = 4;          % BPSK modulation
                    signal_field_decoded.code_rate   = 1/2;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(2); 

                case bi2de([0 1 1 1])
                    signal_field_decoded.mod_order   = 4;          % BPSK modulation
                    signal_field_decoded.code_rate   = 3/4;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(2); 

                case bi2de([1 0 0 1])
                    signal_field_decoded.mod_order   = 16;          % BPSK modulation
                    signal_field_decoded.code_rate   = 1/2;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(10); 

                case bi2de([1 0 1 1])
                    signal_field_decoded.mod_order   = 16;          % BPSK modulation
                    signal_field_decoded.code_rate   = 3/4;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(10); 

                case bi2de([0 0 0 1])
                    signal_field_decoded.mod_order   = 64;          % BPSK modulation
                    signal_field_decoded.code_rate   = 2/3;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(42); 

                case bi2de([0 0 1 1])
                    signal_field_decoded.mod_order   = 64;          % BPSK modulation
                    signal_field_decoded.code_rate   = 3/4;      % Coding rate 1/2 (unpunctured)
                    signal_field_decoded.k_mod       = 1/sqrt(42); 
            end
        end
        
        function data_demapped = demap_data(obj, symbols_eq_mat, signal_field_decoded)
            data_demapped = obj.demap_to_data(reshape(symbols_eq_mat(:,2:end),1,[]).', signal_field_decoded.mod_order).';
        end
        
        function data_deinterleaved = deinterleave_data(obj, data_demapped)
            % Deinterleaving
            % 2nd Permutation
            nCodedBitsPerOFDM = obj.N_SC * obj.get_bits_per_subcarrier();
            k = 0:nCodedBitsPerOFDM-1;
            s = obj.get_bits_per_subcarrier()/2;
            data_demapped = reshape(data_demapped,nCodedBitsPerOFDM,[]);
            data_deinterleaved = ...
                data_demapped(s*floor(k/s) + ...
                mod(k+nCodedBitsPerOFDM-floor(16 * k / nCodedBitsPerOFDM),s)+1,:);

            % 1st Permuation
            data_deinterleaved = ...
                data_deinterleaved(nCodedBitsPerOFDM/16 * mod(k,16)+floor(k/16)+1,:);

            data_deinterleaved = data_deinterleaved(:);
        end
        
        function data_decoded = data_decode(obj, data_deinterleaved, signal_field_decoded)
            trellis_encoder = poly2trellis(7,[133 171]);
            switch signal_field_decoded.code_rate
                case 1/2
                    data_decoded = vitdec(data_deinterleaved, trellis_encoder, 6, 'cont', 'hard');
                case 2/3
                    data_decoded = vitdec(data_deinterleaved, trellis_encoder, 6, 'cont', 'hard', [1 1 1 0]);
                case 3/4
                    data_decoded = vitdec(data_deinterleaved, trellis_encoder, 6, 'cont', 'hard', [1 1 1 0 0 1]);
            end
            data_decoded = circshift(data_decoded,-6,1).';
        end
        
        function [data_descrambled, service_descrambled] = data_descrambled(obj, data_decoded, signal_field_decoded)
            scrambling_seq = [ ...
                0 1 1 0 1 1 0 0 0 0 0 1 1 0 0 1 1 0 1 0 1 0 0 1 1 1 0 0 ...
                1 1 1 1 0 1 1 0 1 0 0 0 0 1 0 1 0 1 0 1 1 1 1 1 0 1 0 0 ...
                1 0 1 0 0 0 1 1 0 1 1 1 0 0 0 1 1 1 1 1 1 1 0 0 0 0 1 1 ...
                1 0 1 1 1 1 0 0 1 0 1 1 0 0 1 0 0 1 0 0 0 0 0 0 1 0 0 0 ...
                1 0 0 1 1 0 0 0 1 0 1 1 1 0 1];
            extended_scrambling_seq = repmat(scrambling_seq, 1, ceil(length(data_decoded)/length(scrambling_seq)));
            data_descrambled = xor(data_decoded, extended_scrambling_seq(1:length(data_decoded)));
            data_descrambled = reshape(data_descrambled, 8, []);
            data_descrambled = reshape(data_descrambled(8:-1:1,:), 1, []);
            service_descrambled = data_descrambled(1:16);
            data_descrambled = data_descrambled(17:16+signal_field_decoded.length_in_bits);
        end
    end
    
end

