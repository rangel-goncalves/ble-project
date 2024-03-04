#include "central.h"

void main(void)
{
	int err = bt_enable(NULL);
	if (err) {
		printk("Falha na inicializacao do BLE: (erro %d)\n", err);
		return;
	}

	printk("BLE Iniciado\n");
	
	err = bt_set_name("Central");
	if (err) {
        printk("Falha ao definir nome do dispositivo (erro %d)\n", err);
        return;
    }
	
	start_scan();
	read_input();
}