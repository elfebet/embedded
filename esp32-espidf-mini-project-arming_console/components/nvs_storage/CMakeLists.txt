idf_component_register(
        SRCS "nvs_storage.c"
        INCLUDE_DIRS "include"
        REQUIRES 
        PRIV_REQUIRES nvs_flash
)
