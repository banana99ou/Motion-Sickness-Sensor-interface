classdef MyIpcamSystem < matlab.System
    % MyIpcamSystem
    % A simple System object that reads frames from an MJPEG IP camera.
    % In Simulink, use a MATLAB System block that references this class.
    %
    % Usage:
    % 1) In your Simulink model, add a "MATLAB System" block
    % 2) Double-click the block, set "System object name" to MyIpcamSystem
    % 3) Provide your camera stream URL in block parameters or here in code
    % 4) The block outputs the camera frame each step
    
    % Public, tunable properties
    properties
        % URL of the MJPEG stream (e.g. from ESP32-CAM)
        CameraURL = 'http://192.168.0.71:80/stream';
    end
    
    % Private properties
    properties(Access = private)
        camObj  % handle to the ipcam object
    end
    
    methods(Access = protected)
        
        function setupImpl(obj)
            % Called once before the simulation starts
            % Initialize the ipcam object
            obj.camObj = ipcam(obj.CameraURL);
        end
        
        function frame = stepImpl(obj)
            % Called every simulation step
            % Acquire one frame from the ipcam object
            frame = snapshot(obj.camObj);
        end
        
        function releaseImpl(obj)
            % Called when the simulation stops or the System object is
            % released. We can clear the ipcam object if we want.
            obj.camObj = [];
        end
        
        %------------------------------------------------------------------
        % Specify output characteristics
        %------------------------------------------------------------------
        
        function num = getNumOutputsImpl(~)
            % We have exactly 1 output: the image frame
            num = 1;
        end
        
        function outSize = getOutputSizeImpl(~)
            % Example: fixed 720p size
            outSize = [720 1280 3];
        end
        
        function varSize = isOutputFixedSizeImpl(~)
            varSize = true;
        end
        
        function outType = getOutputDataTypeImpl(~)
            % Typically, snapshot() returns uint8 images
            outType = 'uint8';
        end
        
        function cplx = isOutputComplexImpl(~)
            cplx = false; % images are real data
        end
        
    end
end
