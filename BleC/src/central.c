#include "central.h"

        // Handle do Atributo (Attribute Handle): Cada característica ou serviço GATT em um dispositivo BLE é identificado por um handle único.
        //Esse handle é um número inteiro que o sistema usa para localizar e acessar o atributo no dispositivo.

        // Handle de Valor (Value Handle): Cada característica GATT tem um valor associado a ela (por exemplo, dados que podem ser lidos ou escritos). 
        //O "handle de valor" é o identificador numérico desse valor específico dentro da característica.

        // Handle de Notificação (Notification Handle): No contexto de notificações,
        //é o handle associado ao atributo GATT que é usado para notificar outras partes quando o valor associado à característica muda.

// Define flags atômicas
CFLAG(discover_f);   // flag para descoberta de dispositivos
CFLAG(subscribed_f); // flag para subscrição a um serviço
CFLAG(cwrite_f);     // flag para escrita em um atributo

// Variáveis globais
static struct bt_conn *default_conn;  // Conexão Bluetooth padrão
        
static uint16_t chrc_h;   // Handle do atributo que contém a característica do serviço
static uint16_t notify_h; // Handle do atributo que contém a notificação do serviço
static volatile size_t num_not;  // Número de notificações recebidas
//Definindo os identificadores UUID
static struct bt_uuid_128 ble_upper = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00);
static struct bt_uuid_128 ble_receive = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03,0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x00);
static struct bt_uuid_128 ble_notify = BT_UUID_INIT_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03,0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x11);
static struct bt_uuid *primary_uuid = &ble_upper.uuid;

//Parâmetros da operação de descoberta GATT (Generic Attribute Profile)
static struct bt_gatt_discover_params disc_params;
// Parâmetros de subscrição ao serviço GATT
static struct bt_gatt_subscribe_params sub_params = {
	.notify = notify,
	.write = subscribe,
	.ccc_handle = 0, // Descoberta automática CCC
	.disc_params = &disc_params, // Descoberta automática CCC
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};

/*
 Função chamada após uma subscrição bem-sucedida a uma característica.
    conn Ponteiro para a conexão BLE.
    err Código de erro (se houver).
    params Ponteiro para os parâmetros de gravação GATT.
*/
void subscribe(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params){
    // Verifica se houve um erro na subscrição e imprime uma mensagem de erro (se houver).
    if (err)
    {
        printk("Falha na inscricao (erro %d)\n", err);
    }

    // Define a flag subscribed_f como verdadeira.
    SFLAG(subscribed_f);

    // Verifica se os parâmetros de gravação são nulos.
    if (!params) 
    {
        printk("parametros: NULL\n");
        return;
    }

    // Verifica se o handle dos parâmetros de gravação corresponde ao handle de notificação.
    if (params->handle == notify_h)
    {
        printk("Subscrição concluída na característica\n");
    }
    else
    {
        printk("Handle desconhecido %d\n", params->handle);
    }
}

/*
Função para lidar com as notificações recebidas do periférico.
    conn Ponteiro para a conexão Bluetooth.
    params Ponteiro para os parâmetros de assinatura.
    data Ponteiro para os dados de notificação recebidos.
    len Comprimento dos dados de notificação recebidos.
    Retorna BT_GATT_ITER_CONTINUE para indicar tratamento de notificação bem-sucedido.
*/
uint8_t notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t len) {
    // Incrementa o contador de notificações e imprime os detalhes da notificação
    printk("\nNotificacao recebida #%u, Tamanho: %d\n", num_not++, len);

    // Copiar dados de notificação para um buffer de string
    uint8_t string[len+1];
    memcpy(string, data, len);
    string[len] = '\0';

    // Imprime os dados recebidos
    printk("\nDados recebidos do periferico: %s\n\n", string);

    // Limpa os dados de notificação para evitar vazamentos de memória
    memset((void*)data, 0, len);

    return BT_GATT_ITER_CONTINUE;
}



// Função para fazer a subscrição ao serviço GATT
void gatt_subscribe(void){
	int err;
	UFLAG(subscribed_f);

	// Define o handle do valor a ser subscrito
	sub_params.value_handle = chrc_h;

	// Faz a subscrição ao serviço GATT
	err = bt_gatt_subscribe(default_conn, &sub_params);

	if (err < 0) {
	    printk("Falha na inscricao\n");
	} else {
	    printk("Solicitacao de inscricao enviada.\n");
	}

	WFLAG(subscribed_f);
}

/*
    Inicia o processo de descoberta de serviços e características disponíveis no dispositivo periférico. 
    A função configura os parâmetros necessários para a descoberta e chama a função 
    bt_gatt_discover() para iniciar o processo.
*/
void gatt_discover(void){
	printk("Descobrindo servicos e caracteristicas\n");
	static struct bt_gatt_discover_params discover_params;

	discover_params.uuid = primary_uuid;
	discover_params.func = discover;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    // Inicia a descoberta GATT e verifica se houve algum erro
    if (bt_gatt_discover(default_conn, &discover_params)) {
        printk("Falha na descoberta\n");
    } else {
        WFLAG(discover_f);
        printk("Descoberta finalizada.\n");
    }
}

/*
    Callback chamado durante a descoberta de serviços e características. Ele processa os atributos encontrados, 
    identificando serviços e características de interesse (por meio de comparação de UUIDs)
    e armazenando handles relevantes para uso posterior.
*/
uint8_t discover(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params){
    int err;

    // Verifica se o atributo é nulo, o que indica o fim da iteração
    if (attr == NULL) {
        // Caso não tenha descoberto a característica long_chrc, retorna a iteração como parada e sinaliza a flag de descoberta
        if (!chrc_h) {
            printk("Nao descobriu long_chrc (%x)\n", chrc_h);
        }
        // Zera os parâmetros de descoberta para poder utilizá-los em outra descoberta
        memset(params, 0, sizeof(*params));

        // Sinaliza a flag de descoberta como completa
        SFLAG(discover_f);
        // Retorna que a iteração deve parar
        return BT_GATT_ITER_STOP;
    }

    // Imprime o handle do atributo encontrado
    printk("[ATRIBUTO] handle %u\n", attr->handle);

    // Se o tipo de descoberta é primário e o UUID é igual ao do serviço superior desejado
    if (params->type == BT_GATT_DISCOVER_PRIMARY && bt_uuid_cmp(params->uuid, &ble_upper.uuid) == 0) {
        // Imprime que o serviço foi encontrado
        printk("Servico encontrado\n");
        // Zera o UUID para poder descobrir as características
        params->uuid = NULL;
        // Define o início da iteração para o próximo atributo após o encontrado
        params->start_handle = attr->handle + 1;
        // Define o tipo da descoberta para características
        params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

        // Inicia uma nova descoberta
        err = bt_gatt_discover(conn, params);
        if (err != 0) {
            // Caso haja erro, imprime a mensagem de erro
            printk("Falha na descoberta (err %d)\n", err);
        }
        // Retorna que a iteração deve parar
        return BT_GATT_ITER_STOP;
    } else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        // Obtém a estrutura de características
        struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

        if (bt_uuid_cmp(chrc->uuid, &ble_receive.uuid) == 0) {
            // Se o UUID da característica for igual ao desejado para receber dados, salva o handle
            printk("rvd_chrc encontrado\n");
            chrc_h = chrc->value_handle;
        } else if (bt_uuid_cmp(chrc->uuid, &ble_notify.uuid) == 0) {
            // Se o UUID da característica for igual ao desejado para notificação, salva o handle
            printk("notify_chrc encontrado\n");
            notify_h = chrc->value_handle;
        }
    }
    // Retorna que a iteração deve continuar
    return BT_GATT_ITER_CONTINUE;
}

//Callback chamada após uma operação de descoberta de dispositivo
void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad){
    char addr_str[BT_ADDR_LE_STR_LEN];
    int err;
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str)); //converte o endereço bluetooth para uma string
    printk("Dispositivo encontrado: %s (RSSI %d)\n", addr_str, rssi);

    if (default_conn) {
        return; // já está conectado
    }

    if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return; // não é um evento connectable
    }

    if (rssi < -70) {
        return; 
    }

    if (bt_le_scan_stop()) {
        printk("Falha ao interromper a verificacao\n");
        return;
    }

    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &default_conn);
    if (err) {
        printk("Falha ao criar coneccao com %s (%d)\n", addr_str, err);
        start_scan();
    }

}

/*
Função callback chamada após uma operação de escrita 
    conn: Conexão Bluetooth entre os dispositivos.
    err: Código de erro retornado pela operação de escrita.
    params: Informações sobre a operação de escrita.
*/
bool erroFlag = false;
void gatt_write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params){
    if (err != BT_ATT_ERR_SUCCESS) {
        if(err == 0x06){
            printk("Falha na escrita: 0x%02X [Limite max de caracteres atigindo (20)]\n\n", err);
            erroFlag = true;
        }else{
            printk("Falha na escrita: 0x%02X\n\n", err);
            erroFlag = true;
        }
    }
    memset(params, 0, sizeof(*params)); // Zera os parâmetros da operação de escrita.
    SFLAG(cwrite_f);
}

/*
Função para escrever em um handle GATT
    handle: handle GATT em que os dados serão escritos
    chrc_data: dados que serão escritos no handle
*/
static void gatt_write(uint16_t handle, char* chrc_data)
{
    // Estrutura para parâmetros da escrita GATT
    static struct bt_gatt_write_params write_params;

    // Variável para armazenar o retorno da função de escrita GATT
    int err;

    // Verifica se o handle é igual ao handle do characteristic
    if (handle == chrc_h)
    {
        printk("Transcrevendo para chrc\n");

        // Atribui os dados e seu tamanho à estrutura de parâmetros de escrita
        write_params.data = chrc_data;
        write_params.length = strlen(chrc_data);
    }

    // Define a função de callback de escrita
    write_params.func = gatt_write_cb;
    // Define o handle GATT em que os dados serão escritos
    write_params.handle = handle;

    // Sinaliza que uma escrita foi iniciada
    UFLAG(cwrite_f);

    // Chama a função para escrever os dados no handle GATT
    err = bt_gatt_write(default_conn, &write_params);

    // Verifica se houve erro na escrita e imprime na tela
    if (err != 0)
        printk("bt_gatt_write falhou: %d\n\n", err);

    // Sinaliza que a escrita foi finalizada
    WFLAG(cwrite_f);
    //verifica se houve algum erro na escrita
    if(!erroFlag){
        printk("Escrita bem-sucedida\n\n");
    }
        
}

// Inicia o scan por dispositivos Bluetooth
void start_scan(void)
{
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
    if (err)
    {
        printk("Falha em iniciar verificacao (erro %d)\n", err);
        return;
    }

    printk("Verificacao iniciada com sucesso\n");
}

/*
Função chamada quando ocorre uma conexão bem-sucedida ou mal-sucedida.
    conn: Ponteiro para a estrutura que representa a conexão Bluetooth.
    err: Código de erro retornado pela tentativa de conexão.
*/
void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    // Obtém o endereço do dispositivo conectado
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    // Caso ocorra um erro de conexão, tenta reconectar com o dispositivo anteriormente conectado
    if (err) {
        printk("Falha ao conectar-se a %s (%u)\n", addr, err);

        if (default_conn) {
            bt_conn_unref(default_conn);
            default_conn = NULL;
        }

        start_scan();
        return;
    }

    // Se a conexão for bem-sucedida, imprime a mensagem de conexão bem-sucedida na tela
    if (conn == default_conn) {
        printk("Conectado: %s\n", addr);
    }
}


void disconnected(struct bt_conn *conn, uint8_t reason)
{
    // Verifica se a conexão interrompida é a conexão padrão
    if (conn != default_conn) {
        return;
    }

    // Converte o endereço da conexão em uma string para uso em mensagens de log
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconectado: %s (motivo 0x%02x)\n", addr, reason);

    // Desconecta ativamente a conexão antes de liberar a memória associada
    int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        printk("Falha ao desconectar: %d\n", err);
    }

    // Libera a memória associada à conexão e define o ponteiro como NULL
    bt_conn_unref(conn);
    default_conn = NULL;

    // Reinicia a busca por dispositivos
    start_scan();
}

void read_input(void)
{
    //Inicializa a leitura no console
    console_getline_init();
	//Variável para controlar a subscrição e descoberta de características do periférico
    bool ctrl = true;
    while(true)
    {
        //Solicita a entrada de dados do usuário
        printk("Digite uma mensagem: ");
        char *s = console_getline();

        //Imprime a entrada recebida
        printk("Mensagem recebida : %s\n", s);

		//Realiza a descoberta e subscrição das características do periférico
		if(ctrl){
			gatt_discover();
			gatt_subscribe();
		}
		//Envia dados ao periférico
        gatt_write(chrc_h, s);
		ctrl = false;
    }
}

//Define as callbacks de conexão e desconexão
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
