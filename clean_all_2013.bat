del .\GdiOle\GdiOle_i.c
del .\GdiOle\GdiOle_i.h
del .\GdiOle\GdiOle_p.c
del .\compiled\*.pdb
del /S /F /Q vc13
rmdir /S /Q .\compiled\Settings\BackUp
rmdir /S /Q .\compiled\Settings\FileLists
rmdir /S /Q .\compiled\Settings\Logs
rmdir /S /Q .\compiled\Settings\HubLists
del /S /F /Q *.tmp
del /S /F /Q *.tlog
del /S /F /Q *.cpp.orig
del /S /F /Q *.h.orig
del .\compiled\*.ilk
del /S /F /Q ipch\*.ipch
del /S /F /Q compiled\update\*.bz2
del /S /F /Q compiled\update\*.xml
del /S /F /Q compiled\update\*.rtf
del /S /F /Q compiled\update\*.sign
"%VS120COMNTOOLS%..\ide\devenv" FlylinkDC_2013.sln /Clean "Release|Win32"
"%VS120COMNTOOLS%..\ide\devenv" FlylinkDC_2013.sln /Clean "Release|x64"
"%VS120COMNTOOLS%..\ide\devenv" FlylinkDC_2013.sln /Clean "Debug|Win32"
"%VS120COMNTOOLS%..\ide\devenv" FlylinkDC_2013.sln /Clean "Debug|x64"