cmake -S ../ -B ../build 
::cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja -S ../ -B ../build/ninja 

::xcopy /Y ..\build\ninja\compile_commands.json ..\
