idf_component_register(SRCS "web_server.c"
    INCLUDE_DIRS "."
    REQUIRES wpa_supplicant esp_http_server frontend wifi_manager)

set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 100)
