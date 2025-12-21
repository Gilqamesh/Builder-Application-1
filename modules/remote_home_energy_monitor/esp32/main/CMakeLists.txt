idf_component_register(
    SRCS "olc_esp_energymonitor_main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_driver_gpio
        esp_timer
    EMBED_TXTFILES other.pem
)
