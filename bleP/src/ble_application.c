#include <ble_application.h>

// Definição do pacote de dados de anúncio (Advertisement Data)
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)), // Flags indicando dispositivo BLE general e sem suporte BREDR
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0d, 0x18, 0x0f, 0x18, 0x0a, 0x18), // Lista de UUIDs suportados (por exemplo, Serviços GATT)
};

// Estrutura que representa a conexão BLE
static struct bt_conn *ble_connection = NULL;

// Callback chamada quando o dispositivo BLE está pronto
static ble_ready_callback ready_callback = NULL;

// Callbacks de conexão BLE
static struct bt_conn_cb conn_callbacks;

// Status atual da conexão BLE
static ble_status status = kBleDisconnected;

// Função para iniciar o anúncio BLE
static uint32_t ble_start_advertise(void)  {
	return (bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0));
}

// Callback chamada quando uma conexão BLE é estabelecida
static void ble_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk(" Falha ao conectar BLE: (err %u)\n", err);
    } else {
        ble_connection = bt_conn_ref(conn);
        status = kBleConnected;
        printk(" BLE Conectado! \n");
    }
}

// Callback chamada quando uma conexão BLE é encerrada
static void ble_disconnected(struct bt_conn *conn, uint8_t reason) {
	printk(" BLE disconectado, Motivo: %u \n", reason);

	// Libera a referência à conexão BLE
	if (ble_connection) {
		bt_conn_unref(ble_connection);
		ble_connection = NULL;
	}

    // Atualiza o status para desconectado
    status = kBleDisconnected;

    // Reinicia o anúncio BLE
    ble_start_advertise();
    printk(" BLE reiniciou o processo de anúncio! \n");
}

// Callback chamada quando a pilha BLE está pronta
static void ble_stack_ready(int err) {
    (void)err;

    // Chama a callback registrada indicando que o dispositivo está pronto
    if (ready_callback) {
        ready_callback(ble_connection);
    }

    // Inicia o anúncio BLE
    ble_start_advertise();
    printk(" BLE iniciou o processo de anúncio! \n");
}

// Função para iniciar a aplicação BLE
uint32_t ble_application_start(ble_ready_callback callback) {
    if (!callback) 
        return (-1);

    // Configura as callbacks de conexão BLE
    conn_callbacks.connected = ble_connected;
    conn_callbacks.disconnected = ble_disconnected;
    ready_callback = callback;

    // Registra as callbacks de conexão BLE
	bt_conn_cb_register(&conn_callbacks);

    // Inicializa a pilha BLE e chama a callback quando pronta
    return ((uint32_t)bt_enable(ble_stack_ready));
}

// Função para obter a referência da conexão BLE
struct bt_conn *ble_get_connection_ref() {
	return ble_connection;
}
