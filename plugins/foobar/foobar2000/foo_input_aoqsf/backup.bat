del _.zip
"C:\Program Files\7-Zip\7z.exe" a -tzip _.zip -mx9 -r "*.bat" "*.txt" "*.sln" "*.vcproj" "*.dsw" "*.dsp" "*.cpp" "*.c" "*.h" "*.rc" "*.rh" "*.ico"
if %errorlevel% neq 0 pause
