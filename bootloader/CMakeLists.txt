target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bootloader_flag.c)

# Only include uart_cmd_handler if not in exam mode
if(NOT DEFINED WHT_APP_RUN_MODE OR NOT WHT_APP_RUN_MODE EQUAL 6)
    target_sources(${PROJECT_NAME} PRIVATE uart_cmd_handler.c)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
