
COMPONENT_ADD_INCLUDEDIRS = . \
deps/ \
deps/esp32-camera/driver/include \
deps/esp32-camera/conversions/include

COMPONENT_PRIV_INCLUDEDIRS = deps/esp32-camera/driver/private_include \
deps/esp32-camera/conversions/private_include \
deps/esp32-camera/sensors/private_include

COMPONENT_SRCDIRS = . \
deps/esp32-camera/driver \
deps/esp32-camera/conversions \
deps/esp32-camera/sensors

CXXFLAGS += -fno-rtti
