# MUD Forwarder
To enable the MUD Forwader, set the environment variable `MUD_FORWARDER=1` before running the setup script:
```
MUD_FORWARDER=1 INFRA_IF_NAME=wlan0 ./script/setup
```

## Changes:
1. ./etc/cmake/options.cmake : if the OTBR_MUD_FORWARDER is set, the OTBR_ENABLE_MUD_FORWARDER compile definition is set to 1. I.e.: use this to define codeblocks that should only be active when MUD forwarding is enabled.
2. ./script/_otbr : if MUD_FORWARDER=1, the OTBR_MUD_FORWARDER option for this project and -DOT_MUDTHREAD and -DOT_MUD_URL are set for the openthread library..
3. ./src/CMakeLists.txt : if the option OTBR_MUD_FORWARDER is set, add the mud_forwarder subdirectory\ to the build.
4. ./scr/agent/application.?pp : the application class has been extended to hold a MudForwarder instance and deinit/init it.
5. ./src/mud_forwarder/ : directory providing the actual code for the mud_forwarder component.
6. ./src/agent/otbr-agent.default.in : added the "?uart-baudrate=115200&uart-flow-control" after the OTBR_RADIO_URL as this helps with connectivity issues.

# 