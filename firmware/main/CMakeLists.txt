set(SRCS
    main.c
    isr_mgr.c
    ble.c
)

idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
