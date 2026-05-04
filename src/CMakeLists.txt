add_executable(kkbd_usb
    main.c
    usb_host.c
    keymap.c
    uart_out.c
    config.c
    led.c
    keyrepeat.c
)

target_include_directories(kkbd_usb PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kkbd_usb PRIVATE
    pico_stdlib
    hardware_uart
    hardware_gpio
    tinyusb_host
)

pico_add_extra_outputs(kkbd_usb)
