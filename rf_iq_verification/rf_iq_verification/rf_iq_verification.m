function rf_iq_verification()
% RF_IQ_VERIFICATION  End-to-end 5G RF IQ signal verification workflow.
%
%   Compares a measured RF baseband IQ capture against a known reference by
%   correcting timing offset, CFO, and complex channel gain/phase, then
%   computes EVM and ACLR, applies pass/fail limits, and logs the result.
%
%   Uses an oversampled, RRC-pulse-shaped waveform so ACLR is meaningful.
%
%   Usage:  rf_iq_verification    % auto-generates demo IQ if files missing
%
%   Author: Md Moklesur Rahman | github.com/dipucwc
%   License: MIT

    Fs       = 30.72e6;
    sps      = 8;
    symRate  = Fs / sps;
    BW       = symRate * 1.2;
    rolloff  = 0.22;
    EVM_LIM  = 3.5;
    ACLR_LIM = 30.0;
    REF = '../data/ref_iq.csv'; RX = '../data/rx_iq.csv';
    LOG = fullfile('results','verification_results.csv');

    rrc = rrc_filter(rolloff, sps, 10);

    if ~isfile(REF) || ~isfile(RX)
        make_demo(REF, RX, Fs, sps, rrc);
    end

    ref = norm_sig(read_iq(REF));
    rx  = norm_sig(read_iq(RX));

    % 1. timing
    [c, lags] = xcorr(rx, ref);
    [~, k] = max(abs(c));
    d = lags(k);
    rx_al = circshift(rx, -d);
    guard = abs(d) + 4*sps;
    if guard < numel(ref)/4
        ref_c = ref(guard+1:end-guard);
        rx_al = rx_al(guard+1:end-guard);
    else
        ref_c = ref;
    end

    % 2. CFO
    ph = unwrap(angle(rx_al .* conj(ref_c)));
    nidx = (0:numel(ph)-1).';
    p = polyfit(nidx, ph, 1);
    cfo = p(1) * Fs / (2*pi);
    rx_cfo = rx_al .* exp(-1j*2*pi*cfo*nidx/Fs);

    % 3. channel + equalize
    h = sum(rx_cfo .* conj(ref_c)) / sum(abs(ref_c).^2);
    rx_eq = rx_cfo / h;

    % 4. metrics
    evm = evm_symbols(rx_eq, ref_c, rrc, sps);
    aclr = compute_aclr(rx_cfo, Fs, BW);

    % 5. decision
    passed = (evm <= EVM_LIM) && (aclr >= ACLR_LIM);

    % 6. log
    if ~isfolder('results'); mkdir('results'); end
    fid = fopen(LOG, 'w');
    fprintf(fid, 'metric,value,limit,status\n');
    fprintf(fid, 'timing_offset_samples,%d,-,-\n', d);
    fprintf(fid, 'cfo_hz,%.2f,-,-\n', cfo);
    fprintf(fid, 'channel_mag,%.4f,-,-\n', abs(h));
    fprintf(fid, 'channel_phase_deg,%.2f,-,-\n', rad2deg(angle(h)));
    fprintf(fid, 'evm_percent,%.3f,%.1f,%s\n', evm, EVM_LIM, tf(evm<=EVM_LIM));
    fprintf(fid, 'aclr_db,%.2f,%.1f,%s\n', aclr, ACLR_LIM, tf(aclr>=ACLR_LIM));
    fprintf(fid, 'overall,-,-,%s\n', tf(passed));
    fclose(fid);

    % 7. summary
    fprintf('================================================\n');
    fprintf(' 5G RF IQ Verification Summary\n');
    fprintf('================================================\n');
    fprintf(' Timing offset : %d samples\n', d);
    fprintf(' CFO           : %.2f Hz\n', cfo);
    fprintf(' Channel       : |h|=%.4f, ang h=%.2f deg\n', abs(h), rad2deg(angle(h)));
    fprintf(' EVM           : %.3f %%   (limit %.1f %%)\n', evm, EVM_LIM);
    fprintf(' ACLR          : %.2f dB  (limit %.1f dB)\n', aclr, ACLR_LIM);
    fprintf(' RESULT        : %s\n', tf(passed));
    fprintf(' Log written   : %s\n', LOG);
end

% ----------------------------- helpers --------------------------------- %
function s = tf(b); if b; s='PASS'; else; s='FAIL'; end; end

function x = norm_sig(x)
    x = x - mean(x);
    p = sqrt(mean(abs(x).^2));
    if p>0; x = x/p; end
end

function iq = read_iq(path)
    M = readmatrix(path);
    iq = M(:,1) + 1j*M(:,2);
end

function taps = rrc_filter(beta, sps, span)
    n = (-span*sps : span*sps).';
    t = n / sps;
    taps = zeros(size(t));
    for i = 1:numel(t)
        ti = t(i);
        if abs(ti) < 1e-8
            taps(i) = 1 - beta + 4*beta/pi;
        elseif beta>0 && abs(abs(ti) - 1/(4*beta)) < 1e-8
            taps(i) = (beta/sqrt(2)) * ((1+2/pi)*sin(pi/(4*beta)) + ...
                       (1-2/pi)*cos(pi/(4*beta)));
        else
            num = sin(pi*ti*(1-beta)) + 4*beta*ti*cos(pi*ti*(1+beta));
            den = pi*ti*(1-(4*beta*ti)^2);
            taps(i) = num/den;
        end
    end
    taps = taps / sqrt(sum(taps.^2));
end

function evm = evm_symbols(rx_eq, ref, rrc, sps)
    mf  = conv(rx_eq, rrc, 'same');
    mfr = conv(ref,   rrc, 'same');
    start = 4*sps + 1;
    rx_s  = mf(start:sps:end);
    ref_s = mfr(start:sps:end);
    m = min(numel(rx_s), numel(ref_s));
    rx_s = rx_s(1:m); ref_s = ref_s(1:m);
    rx_s  = rx_s  / sqrt(mean(abs(rx_s).^2));
    ref_s = ref_s / sqrt(mean(abs(ref_s).^2));
    err = rx_s - ref_s;
    evm = sqrt(mean(abs(err).^2)/mean(abs(ref_s).^2)) * 100;
end

function aclr = compute_aclr(x, Fs, BW)
    N = numel(x);
    spec = abs(fftshift(fft(x))).^2;
    f = (-N/2:N/2-1).' * (Fs/N);
    bp = @(lo,hi) sum(spec(f>=lo & f<hi));
    main  = bp(-BW/2, BW/2);
    lower = bp(-3*BW/2, -BW/2);
    upper = bp(BW/2, 3*BW/2);
    aclr = 10*log10(main / max([lower, upper, 1e-12]));
end

function make_demo(refFile, rxFile, Fs, sps, rrc)
    rng(42);
    nSym = 2048;
    bits = randi([0 1], nSym, 2);
    syms = ((bits(:,1)*2-1) + 1j*(bits(:,2)*2-1)) / sqrt(2);
    up = zeros(nSym*sps, 1);
    up(1:sps:end) = syms;
    ref = conv(up, rrc, 'same');
    ref = ref / sqrt(mean(abs(ref).^2));
    d = 5*sps + 3; cfo = 1.5e3; h = 0.85*exp(1j*deg2rad(25));
    idx = (0:numel(ref)-1).';
    rx = h * circshift(ref, d) .* exp(1j*2*pi*cfo*idx/Fs);
    rx = rx + (randn(numel(ref),1)+1j*randn(numel(ref),1))*0.01;
    writematrix([real(ref) imag(ref)], refFile);
    writematrix([real(rx)  imag(rx)],  rxFile);
    fprintf('[info] generated demo files: %s, %s\n', refFile, rxFile);
end
