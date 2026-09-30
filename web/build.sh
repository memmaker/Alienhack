#!/bin/sh
# Build AlienHack for the web (Emscripten). Run from anywhere: sh web/build.sh
# Needs emcc on PATH and the boost_headers port (fetched by -sUSE_BOOST_HEADERS).
set -e
cd "$(dirname "$0")"
WEB=$(pwd); ROOT=$(cd .. && pwd)
DEPS=$WEB/deps
mkdir -p $DEPS
# Dependencies, pinned (not committed): RL-Shared, Boost.Serialization/Filesystem sources.
[ -d $DEPS/RL-Shared ] || { git clone -q https://github.com/SockPuppet8/RL-Shared $DEPS/RL-Shared && git -C $DEPS/RL-Shared checkout -q c42d3889d58672a5198ee2066c5cb1af509889f1 && git -C $DEPS/RL-Shared apply $WEB/rl-shared.patch; }
[ -d $DEPS/serialization ] || git clone -q --depth 1 -b boost-1.83.0 https://github.com/boostorg/serialization $DEPS/serialization
[ -d $DEPS/filesystem ] || git clone -q --depth 1 -b boost-1.83.0 https://github.com/boostorg/filesystem $DEPS/filesystem
RLS=$DEPS/RL-Shared
OBJ=$WEB/obj${SAN:+-asan}; mkdir -p $OBJ
CXXFLAGS="${OPT:--O2} -std=gnu++17 -Wno-register -fexceptions -sUSE_BOOST_HEADERS=1 -I$RLS -I$RLS/Include -include $WEB/prefix.hpp -I$ROOT/src/Console -I$ROOT/src/Model -I$ROOT/src -DBOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF -DBOOST_FILESYSTEM_SINGLE_THREADED ${SAN:+-fsanitize=address} $EXTRA"
SRCS="$(find $ROOT/src $RLS \( -name '*.cpp' -o -name '*.cc' \) ! -path '*/Console/Console.cpp' ! -name main.cpp) $ROOT/src/Console/main.cpp $WEB/Console-web.cpp"
for f in text_iarchive text_oarchive basic_text_iprimitive basic_text_oprimitive basic_archive basic_iarchive basic_oarchive basic_iserializer basic_oserializer basic_pointer_iserializer basic_pointer_oserializer basic_serializer_map extended_type_info extended_type_info_typeid void_cast archive_exception stl_port utf8_codecvt_facet codecvt_null; do SRCS="$SRCS $DEPS/serialization/src/$f.cpp"; done
for f in codecvt_error_category exception operations path path_traits portability unique_path utf8_codecvt_facet directory; do SRCS="$SRCS $DEPS/filesystem/src/$f.cpp"; done
OBJS=""
for s in $SRCS; do
  o=$OBJ/$(echo "${s#$ROOT/}" | tr '/' '_' | sed 's/\.cc*p*$/.o/')
  OBJS="$OBJS $o"
  if [ ! -f $o ] || [ $s -nt $o ]; then echo "$s" > $o.src; fi
done
# compile stale objects in parallel
ls $OBJ/*.src >/dev/null 2>&1 && ls $OBJ/*.src | sed 's/\.src$//' | xargs -P $(nproc) -n 1 sh -c 'em++ -c $(cat $1.src) -o $1 '"$CXXFLAGS"' -I'"$DEPS"'/filesystem/src && rm $1.src' _
mkdir -p $WEB/pkg && cp $ROOT/keys.txt $ROOT/ah_readme.txt $WEB/pkg/
mkdir -p $WEB/dist
em++ $OBJS -o $WEB/dist/alienhack.js ${OPT:--O2} -fexceptions -sUSE_BOOST_HEADERS=1 ${SAN:+-fsanitize=address} \
  -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB \
  -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,HEAP8 -sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web \
  --preload-file $WEB/pkg@/ahdata $LDEXTRA
cp $WEB/index.html $WEB/dist/
echo built $WEB/dist
