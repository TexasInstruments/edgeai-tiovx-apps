# Edge AI TIOVX Apps
> Repository to host TIOVX based Edge AI applications for TI devices


## Steps to compile

1. Building on the target

    ```console
    root@j7-evm:/opt# git clone https://github.com/TexasInstruments/edgeai-tiovx-apps.git
    root@j7-evm:/opt# cd edgeai-tiovx-apps
    root@j7-evm:/opt/edgeai-tiovx-apps# export SOC=(j721e or j721s2 or j784s4 or am62a)
    root@j7-evm:/opt/edgeai-tiovx-apps# ./make_apps.sh
    ```
2. Cross-compilation for target
    The app can be cross-compiled on an x86_64 machine for the target. Here are the steps for cross-compilation.
    Here 'work_area' is PSDK RTOS workarea.

    1) cd work_area
    2) git clone https://github.com/TexasInstruments/edgeai-tiovx-apps.git
    3) cd edgeai-tiovx-apps/
    4) Update cmake/setup_cross_compile.sh to specify tool paths and settings
    5) source cmake/setup_cross_compile.sh
    6) mkdir build
    7) cd build
    8) export SOC=(j721e/j721s2/j784s4/am62a)
    9) cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/cross_compile_aarch64.cmake ..
    10) make -j2


## Steps to run

1. Go to edgeai-tiovx-apps directory on target under /opt

    ```console
    root@j7-evm:/opt# cd edgeai-tiovx-apps
    ```

2. Run the app

    ```console
    root@j7-evm:/opt/edgeai-tiovx-apps# ./bin/Release/app_edgeai ./configs/image_classification.yaml
    ```