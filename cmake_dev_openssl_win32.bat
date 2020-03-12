mkdir build_jobs_win32_msvc_openssl
cd build_jobs_win32_msvc_openssl
:: openssl : see https://slproweb.com/products/Win32OpenSSL.html
:: libcurl : 
::     "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
::     cd winbuild 
::     nmake /f Makefile.vc mode=static VC=15 MACHINE=x64
cmake .. -G "Visual Studio 15 2017" -DCMAKE_BUILD_TYPE=Debug -DLIBUV_ROOT=C:\workspace\lib\network\prebuilt\Win32 -DMSGPACK_ROOT=C:\workspace\lib\protocol\msgpack\prebuilt -DLIBCURL_ROOT=C:\workspace\lib\network\prebuilt\Win32 -DOPENSSL_ROOT_DIR=C:\OpenSSL-Win32  -DPROJECT_ENABLE_SAMPLE=OFF -DPROJECT_ENABLE_UNITTEST=OFF  -DPROJECT_ENABLE_TOOLS=OFF -DBUILD_SHARED_LIBS=OFF
