#include <ble_uart_service.h>

// Declarando variáveis e definindo constantes
static ble_uart_service_rx_callback rx_callback = NULL; // Declarando um ponteiro para uma função de retorno de chamada
#define BLE_UART_SERVICE_TX_CHAR_OFFSET    3 // Definindo um offset para um caractere


#define CHRC_SIZE 100 // Definindo o tamanho máximo do buffer de caracteres
static uint8_t chrc_data[CHRC_SIZE]; // Criando um buffer de caracteres com o tamanho definido acima

// Definindo macros para flags
#define CFLAG(flag) static atomic_t flag = (atomic_t)false
#define SFLAG(flag) (void)atomic_set(&flag, (atomic_t)true)

CFLAG(flag_long_subscribe); // Declarando uma flag para inscrição longa

// Definindo UUIDs para o serviço BLE
static struct bt_uuid_128 ble_uart_uppercase = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00);
static struct bt_uuid_128 ble_uart_receive_data = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03,0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x00);
static struct bt_uuid_128 ble_uart_notify = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03,0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x11);

// Função de retorno de chamada para quando um pacote é recebido
ssize_t uart_rx_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    static uint8_t prepare_count; 

    // Transformando o dado recebido em uma string
    uint8_t string[len+1];
    for(int i = 0; i < len;i++)
        string[i] = *((char*)buf+i);

    string[len] = '\0';
    printk("\nDados recebidos: %s\n",string);

    // Verificando se o tamanho do dado é válido
    if (len > sizeof(chrc_data))
    {
        printk("Tamanho invalido\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    } 
    
    // Verificando se o offset do dado é válido
    else if (offset + len > sizeof(chrc_data))
    {
        printk("Tamanho e offset invalido!\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    
    // Verificando se o dado é de preparação
    if (flags & BT_GATT_WRITE_FLAG_PREPARE)
    {
        printk("prepare_count %u\n", prepare_count++);
        return BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
    }

    (void)memcpy(chrc_data + offset, buf, len); // Copia o buffer para o array de dados do atributo GATT
    prepare_count = 0;

    if (rx_callback) {
        
        rx_callback(buf, len); // Chama a função de retorno de chamada
    }
    
    return len;
}


/*Define um conjunto de atributos, onde o segundo parâmetro define uma característica sem necessidade de resposta e o terceiro parâmetro
define que se tenha a característica de notificar uma resposta à central
*/
static struct bt_gatt_attr attrs[] = { 
        BT_GATT_PRIMARY_SERVICE(&ble_uart_uppercase),
		BT_GATT_CHARACTERISTIC(&ble_uart_receive_data.uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, uart_rx_callback, NULL),
        BT_GATT_CHARACTERISTIC(&ble_uart_notify.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
        BT_GATT_CCC(ble_uart_ccc_changed, BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service peripheral = BT_GATT_SERVICE(attrs);

// Callback para quando o valor do Cliente Characteristic Configuration (CCC) é alterado
void ble_uart_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    //Verifica se as notificações foram habilitadas
    const bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	// Se as notificações foram habilitadas, é marcada a flag "flag_long_subscribe"
    if (notif_enabled)
		SFLAG(flag_long_subscribe);
	printk("Notificacoes %s\n", notif_enabled ? "Habilitado" : "Desabilitado");
}

/* Função para registrar o serviço GATT no periférico BLE UART 
Recebe como argumento um ponteiro para uma função de callback 
*/
int ble_uart_service_register(const ble_uart_service_rx_callback callback)
{
    // Atribui a função de callback ao ponteiro global rx_callback
    rx_callback = callback;
	// Registra o serviço GATT no periférico BLE UART usando a API bt_gatt_service_register() e retorna o resultado da operação
    return 	bt_gatt_service_register(&peripheral);
}

int ble_uart_service_transmit(const uint8_t *buffer, size_t len)
{
    printk("\nTransmitindo...\n");

    // Verifica se o buffer e o tamanho são válidos
	if (!buffer || !len)
		return -1;

    // Obtém a referência da conexão atual
    struct bt_conn *conn = ble_get_connection_ref();
    
    // Converte os caracteres minúsculos em maiúsculos
    uint8_t string[len+1];
    for (int i = 0; i < len; i++)
    {
        string[i] = toupper(buffer[i]);
    }
    string[len] = '\0';

    // Verifica se há conexão ativa
    if (conn)
       // Notifica o dispositivo central com a string convertida
       return bt_gatt_notify(conn, &attrs[2], string, len);
    else
        return -1;
}
