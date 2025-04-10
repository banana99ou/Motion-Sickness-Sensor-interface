% model_init.m
% Simulink 모델 실행 전 필요한 모든 변수 한 번만 정의

%  게인 값 
SVC_VV.K_a = 0.1;   
SVC_VV.K_w = 0.1;    
SVC_VV.K_wc = 10;   
SVC_VV.K_ac = 0.5;  
SVC_VV.K_vc = 2.5;  
SVC_VV.K_vvc = 2.5;  

%  시상수 
SVC_VV.tau_LP = 2.0;   
SVC_VV.tau_a = 190.0;   
SVC_VV.tau_d = 7.0;     

%  MSI 
SVC_VV.b = 0.5;        
SVC_VV.tau_I = 720.0;   
SVC_VV.P = 85;          


