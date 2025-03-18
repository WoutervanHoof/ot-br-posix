add_library(otbr-mud-forwarder
    mud_forwarder.cpp
    mud_forwarder.hpp
)

target_link_libraries(otbr-mud-forwarder PRIVATE
    otbr-common
    otbr-utils
)