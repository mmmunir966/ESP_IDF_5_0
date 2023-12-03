idf_component_register(SRCS "wifi_manager.c" 
    INCLUDE_DIRS "."
    REQUIRES esp_wifi freertos lwip nvs_flash json)


set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 100)
