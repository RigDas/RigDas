%%----Log Spectral Distance Calculation------%%
Clean=audioread('C:\old audio figure\audio\gt_clean\y_0.wav');
Generated=audioread('C:\old audio figure\audio\generated\y_hat_0.wav');
% Noisy=audioread('C:\old audio figure\audio\gt_noise\y_4.wav');
fs=48000;
LSD=LogSpectralDistance(Clean,Generated,fs);
display(LSD)

%RS=
%Range=
%LSD=LogSpectralDistance(Clean,Noisy,fs,RS,Range);


