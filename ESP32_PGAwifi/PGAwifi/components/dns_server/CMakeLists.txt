idf_component_register(SRCS "dns_server.c"
    INCLUDE_DIRS "."
    REQUIRES wpa_supplicant lwip)


set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 100)
