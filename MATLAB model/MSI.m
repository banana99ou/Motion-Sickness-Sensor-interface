function [sys, x0, str, ts] = MSI(t, x, u, flag, SVC_VV)
    % MSI_ 블록 (Motion Sickness Incidence 모델)
    % 입력:
    %   - delta_v [3x1] : 시각 신호 v_s 와 내무 모델 시각 신호 v_s^ 의 차이
    %   - SVC_VV : 시스템 파라미터를 담고 있는 구조체
    % 출력:
    %   - MSI [1x1] : 멀미 발생 확률 0-1

    switch flag
        case 0  % 초기화
            sizes = simsizes;
            sizes.NumContStates  = 0;  
            sizes.NumDiscStates  = 0; 
            sizes.NumOutputs     = 1;  % MSI (스칼라 값)
            sizes.NumInputs      = 3;  % delta_v (3x1)
            sizes.DirFeedthrough = 1;  
            sizes.NumSampleTimes = 1;
            sys = simsizes(sizes);
            x0  = [];  
            str = [];
            ts  = [0 0];  % Fixed-Step Auto 설정

        case 3  
            % 입력 받기 (시각-전정 신호 차이)
            delta_v = u(1:3);  

            % SVC_VV 구조체에서 필요한 변수 가져오기
            P = SVC_VV.P;
            tau_I = SVC_VV.tau_I;
            b = SVC_VV.b;

            % L2 Norm 계산 (시각-전정 신호 차이 크기)
            norm_delta_v = norm(delta_v);

            % Hill Function 적용
            H = ( (norm_delta_v / b)^2 ) / ( 1 + (norm_delta_v / b)^2 );

            % 최종 MSI 변환
            MSI_raw = H * (P / ( (tau_I + 1)^2 ));
            
            sys = MSI_raw; % 최종 출력값

        otherwise
            sys = [];
    end
end
