@ECHO OFF
set /p in="Path to input image:"
set /p out="Path to output image:"
set /p max_threads="Enter max threads value:"
set /p binary="Path to .exe:"
FOR /L %%I IN (0,1,%max_threads%) DO %binary% %%I %in% %out%
PAUSE
