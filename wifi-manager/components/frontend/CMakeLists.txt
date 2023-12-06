idf_component_register(INCLUDE_DIRS "."
    EMBED_FILES index.html styles.css common.js logo.png
)

set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 100)
