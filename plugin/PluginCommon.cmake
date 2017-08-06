include_directories(${PLUGIN_INCLUDE_DIR})
include(${PLUGIN_INCLUDE_DIR}/EmbedMetadata.cmake)

embed_metadata(METADATA_FILE meta.json)

add_library(${PROJECT_NAME} SHARED main.cpp meta.json ${whereami_files})
