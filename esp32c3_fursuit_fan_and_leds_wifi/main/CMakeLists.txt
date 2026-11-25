file(GLOB_RECURSE FILES "*.c")

idf_component_register(
    SRCS
        ${FILES}

    INCLUDE_DIRS
        "."
)

spiffs_create_partition_image(storage ../filesystem FLASH_IN_PROJECT)
