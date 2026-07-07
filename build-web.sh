source /home/kevin-james/emsdk-main/emsdk_env.sh
export CC=emcc
export cc=emcc
export cxx=em++
export CXX=em++
#rm -rf build
cmake -Bbuild -DPLATFORM=Web
cmake --build build