classdef MyIpcamSystem < matlab.System
    properties
        CameraURL = 'http://192.168.0.71:80/stream';
    end
    
    properties(Access = private)
        camObj
    end
    
    methods(Access = protected)
        function setupImpl(obj)
            obj.camObj = ipcam(obj.CameraURL);
        end
        
        function frame = stepImpl(obj)
            frame = snapshot(obj.camObj);
        end
        
        function releaseImpl(obj)
            obj.camObj = [];
        end
        
        % Number of outputs
        function num = getNumOutputsImpl(~)
            num = 1;
        end
        
        % Hard-code the output size
        function outSize = getOutputSizeImpl(~)
            outSize = [720 1280 3];   % or [480 640 3], etc.
        end
        
        % Data type
        function outType = getOutputDataTypeImpl(~)
            outType = 'uint8';
        end
        
        % Not complex
        function cplx = isOutputComplexImpl(~)
            cplx = false;
        end
        
        % Fixed-size output
        function fixed = isOutputFixedSizeImpl(~)
            fixed = true;
        end
    end
end
