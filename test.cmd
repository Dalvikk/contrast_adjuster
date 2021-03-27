@ECHO OFF
set INPUT=image2.ppm
set OUTPUT=result2.ppm
set MAX_THREADS=16
set BINARY=contrast_adjuster.exe
FOR /L %%I IN (0,1,%MAX_THREADS%) DO %BINARY% %%I %INPUT% %OUTPUT%
PAUSE
