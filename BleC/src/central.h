#ifndef CENTRAL_H
#define CENTRAL_H

#include <bluetooth/att.h>     // Define as funções de gerenciamento de ATT Bluetooth
#include <bluetooth/bluetooth.h>  // Define as estruturas básicas do Bluetooth
#include <bluetooth/conn.h>    // Define as funções de gerenciamento de conexão Bluetooth
#include <bluetooth/gatt.h>    // Define as funções de gerenciamento de GATT Bluetooth
#include <bluetooth/hci.h>     // Define as funções do controlador Bluetooth
#include <bluetooth/uuid.h>    // Define as funções de gerenciamento de UUID Bluetooth
#include <console/console.h>   // Define funções para o console do Zephyr
#include <errno.h>             // Define valores de erro
#include <stddef.h>            // Define tipos básicos e macros
#include <stdint.h>            // Define tipos inteiros de tamanho específico
#include <sys/byteorder.h>     // Define as funções para conversão de endianess
#include <sys/printk.h>        // Define função printk para imprimir no console
#include <sys/slist.h>         // Define as funções para gerenciamento de listas
#include <zephyr.h>            // Define funções básicas do kernel
#include <zephyr/types.h>      // Define tipos básicos de dados

// Define macros para flags atômicas
#define CFLAG(flag) static atomic_t flag = ATOMIC_INIT(false) //Define uma variável como atômica e como false, além de tornar acessível em todo escopo da função
#define SFLAG(flag) atomic_set(&flag, true) // Define o valor de uma variável atômica como true
#define UFLAG(flag) atomic_set(&flag, false) // Define uma variável atômica como false
// Faz um loop enquanto o valor de uma variável atômica for false, com uma pausa de 1 ms entre cada iteração, usada para controlar sincronização entre threads
#define WFLAG(flag) \ 
	while (!atomic_get(&flag)) { \
		k_sleep(K_MSEC(1)); \
	}

// Define constantes para o handle dos atributos
#define BT_ATT_FIRST_ATTRIBUTE_HANDLE 0x0001
#define BT_ATT_LAST_ATTRIBUTE_HANDLE  0xffff

//Calbacks
void subscribe(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params);
uint8_t notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t len);
void gatt_subscribe(void);
void gatt_discover(void);
uint8_t discover(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params);
void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);
void gatt_write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params);
void start_scan(void);
void connected(struct bt_conn *conn, uint8_t err);
void disconnected(struct bt_conn *conn, uint8_t reason);
void read_input(void);

#endif //CENTRAL_H//
