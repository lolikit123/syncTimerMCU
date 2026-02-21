set -euo pipefail

make -C stm32/third_party/libopencm3 TARGETS=stm32/f4 -j"$(nproc)"
cmake -S stm32/app -B stm32/app/buildD -DCMAKE_BUILD_TYPE=Debug -DAPP_ENABLE_TIMEBASE_TESTS=ON
cmake --build stm32/app/buildD -j"$(nproc)"