@echo off
set exit_code=0

echo "BUILD TYPE %BUILD_TYPE%"
echo "RUN %RUN%"

cmake .. ^
  -Ax64 ^
  %VXLDOLLAR_TEST% ^
  %CI% ^
  %ROCKS_LIB% ^
  -DPORTABLE=1 ^
  -DQt5_DIR="c:\qt\5.13.1\msvc2017_64\lib\cmake\Qt5" ^
  -DVXLDOLLAR_GUI=ON ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DACTIVE_NETWORK=vxldollar_%NETWORK_CFG%_network ^
  -DVXLDOLLAR_SIMD_OPTIMIZATIONS=TRUE ^
  -Dgtest_force_shared_crt=on ^
  -DBoost_NO_SYSTEM_PATHS=TRUE ^
  -DBoost_NO_BOOST_CMAKE=TRUE ^
  -DVXLDOLLAR_SHARED_BOOST=%VXLDOLLAR_SHARED_BOOST%

set exit_code=%errorlevel%
if %exit_code% neq 0 goto exit

:exit
exit /B %exit_code%