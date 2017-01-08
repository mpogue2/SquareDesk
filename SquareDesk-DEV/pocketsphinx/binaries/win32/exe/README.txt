To update the Win32 version of PocketSphinx:

1) Download Win32 (latest) from CMU
2) Unpack to C:\pocketsphinx
3) Copy from C:\pocketsphinx\bin\Release\Win32 the files: sphinxbase.dll, pocketsphinx.dll, and pocketsphinx_continuous.exe to this directory
4) Update the HMM by copying the entire directory
c:\pocketsphinx\model\en-us\en-us to here:
../models/en-us
5) Test by running ps_test (in this directory), and
give it a few calls (e.g. Square thru 4).  They should
all be transcribed properly (ignore lines starting with
"INFO:"