# Edge AI TIOVX Apps
> Repository to host TIOVX based Edge AI applications for TI devices

Detailed documentation can be found on [WIKI](https://github.com/TexasInstruments/edgeai-tiovx-apps/wiki/Edge-AI-TIOVX-Apps-reference-guide)

# Directory Structure

```
.
├── apps
│       Contains OpenVX based Deep learning applications with differenct input and output options
│       like  v4l2 capture, tiovx capture, h264 decode etc.. to Display, encode to file etc..
├── cmake
│       Build files
├── configs
│       YAML based config file for apps
│       Refer to template config file for details on how to modify or write new configs
├── modules
│       A thin framework to abstract some OpenVX things to make the application code
│       simpler. These modules can directly be used to write custom pipelines. Plese
│       refer to tests
├── tests
│       Some simple tests for modules that also servers as examples on how to use
│       modules to write custom pipelines
└── utils
        Some utility functions used by apps and modules
```


## Steps to compile

1. Building on the target

    ```console
    root@j7-evm:/opt# cd edgeai-tiovx-apps
    root@j7-evm:/opt/edgeai-tiovx-apps# export SOC=(j721e or j721s2 or j784s4 or j722s or am62a or j742s2)
    root@j7-evm:/opt/edgeai-tiovx-apps# mkdir build
    root@j7-evm:/opt/edgeai-tiovx-apps# cd build
    root@j7-evm:/opt/edgeai-tiovx-apps/build# cmake ..
    root@j7-evm:/opt/edgeai-tiovx-apps/build# make -j2 install
    ```

2. Please use [EdgeAI App Stack](https://github.com/TexasInstruments/edgeai-app-stack) for cross compilation

## Steps to run

1. Go to edgeai-tiovx-apps directory on target under /opt

    ```console
    root@j7-evm:/opt# cd edgeai-tiovx-apps
    ```

2. Run the app

    ```console
    root@j7-evm:/opt/edgeai-tiovx-apps# ./bin/Release/edgeai-tiovx-apps-main configs/linux/object_detection.yaml
    ```
