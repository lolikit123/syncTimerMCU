set -euo pipefail

make -C stm32/third_party/libopencm3 TARGETS=stm32/f4 -j"$(nproc)"
cmake -S stm32/app -B stm32/app/buildR \
  -DCMAKE_BUILD_TYPE=Release \
  -DAPP_ENABLE_TIMEBASE_TESTS=OFF \
  -DAPP_BUILD_MASTER=ON \
  -DAPP_BUILD_SLAVE=ON
cmake --build stm32/app/buildR -j"$(nproc)"