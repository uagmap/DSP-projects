function B = fircoeffs(N, type, fs, fL, fH)
 M           = (N-1)/2;
 WnL         = 2*pi*fL/fs;
 WnH         = 2*pi*fH/fs;
 hH          = sin(WnH*[-M:1:-1])./([-M:1:-1]*pi);
 hH(M+1)     = WnH/pi;
 hH(M+2:1:N) = hH(M:-1:1);
 hL          = sin(WnL*[-M:1:-1])./([-M:1:-1]*pi);
 hL(M+1)     = WnL/pi;
 hL(M+2:1:N) = hL(M:-1:1);

 if strcmp(type, 'low')           % low-pass filter
   h(1:N) = hL(1:N);
 end
 if strcmp(type, 'high')          % high-pass filter
   h(1:N) = -hH(1:N);
   h(M+1) = 1+h(M+1);
 end
 if strcmp(type,'bandpass')       % band-pass filter
   h(1:N) = hH(1:N)-hL(1:N);
 end
 if strcmp(type, 'bandstop')      % band-stop filter
   h(1:N) = hL(1:N)-hH(1:N);
   h(M+1) = 1+h(M+1);
 end
 B = h;
