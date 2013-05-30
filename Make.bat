rem remove "exec /bin/bash" if you do not want to see the console when done

if exist C:\MinGW\msys\1.0\bin\bash.exe (
    C:\MinGW\msys\1.0\bin\bash.exe -l -i -c "cd \"%CD%\"; make; make clean; exec /bin/bash" 
) else (
    echo msgbox "Error: Missing C:\MinGW\msys\1.0\bin\bash.exe" > "%temp%\popup.vbs"
    wscript.exe "%temp%\popup.vbs"
)
