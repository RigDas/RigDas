%%----PSNR for Mel Spectrogram------%%
Clean_Image=imread('C:\old audio figure\figures\gt\y_spec_clean_4.png');
Generated_Image=imread('C:\old audio figure\figures\generated\y_hat_spec_4.png');
PSNR(Generated_Image, Clean_Image);



%%------PSNR for Audio Spectrogram------%%
Clean=audioread('C:\old audio figure\audio\gt_clean\y_4.wav');
% Noisy=audioread('C:\old audio figure\audio\generated\y_hat_3.wav');
Noisy=audioread('C:\old audio figure\audio\gt_noise\y_4.wav');
Spec_CI=spectrogram(Clean);
Spec_GI=spectrogram(Noisy);

PSNR(Spec_GI, Spec_CI);
