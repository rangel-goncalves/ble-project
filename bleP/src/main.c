#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <kernel.h>
#include <ble_application.h>
#include <ble_uart_service.h>

// Função de callback que é acionada quando dados são recebidos pela interface BLE
static void on_ble_rx_data(const uint8_t *buffer, size_t len) {
    ble_uart_service_transmit(buffer, len);
}

// Função de callback que é acionada quando a pilha BLE está pronta
static void on_ble_stack_ready(struct bt_conn *conn) {
    (void)conn;
    ble_uart_service_register(on_ble_rx_data);
}

void main (void) {
    // Inicia a aplicação BLE com a função de callback "on_ble_stack_ready"
    ble_application_start(on_ble_stack_ready);
    int err = bt_set_name("Periferico");
    if (err) {
        printk("Falhar ao definir nome do dispositivo (erro %d)\n", err);
        return;
    }
}