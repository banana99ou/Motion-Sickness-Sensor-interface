% Suppose after running your Simulink model, you have a variable 'vidFrames'
% in your workspace. Let's discover its size:

dims = size(vout);
disp('Size of vidFrames:');
disp(dims);

% Decide on output file
videoFileName = 'myOutput.avi';
v = VideoWriter(videoFileName, 'Motion JPEG AVI'); 
% or: v = VideoWriter('myOutput.mp4','MPEG-4');

open(v);

switch length(dims)
    case 4
        % 4D: [Height x Width x 3 x NumFrames] (color)
        [H, W, C, numFrames] = size(vout);
        for i = 1:numFrames
            frameRGB = vout(:,:,:,i); 
            frameRGB = im2uint8(frameRGB); 
            writeVideo(v, frameRGB);
        end

    case 3
        % 3D: [Height x Width x NumFrames] (grayscale)
        [H, W, numFrames] = size(vout);
        for i = 1:numFrames
            frameGray = vout(:,:,i);
            frameGray = im2uint8(frameGray);
            % Some codecs accept single-channel directly, others need 3-ch
            writeVideo(v, frameGray);
        end
end

close(v);
fprintf('Saved video to %s\n', videoFileName);
