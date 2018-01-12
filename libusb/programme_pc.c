#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>

#define MAX_DATA 8

typedef struct{
	int interface;
	int address;
	int direction;
}EndPoint;


int init_context(libusb_context ** context){
	int status=libusb_init(context);
	if(status!=0){perror("libusb_init"); return -1;}
	return 0;
}

int find_target(int id_vendor, int id_product, libusb_context * context, libusb_device_handle ** handle){
	libusb_device **list;
	ssize_t count = libusb_get_device_list(context,&list);
	if(count<0) {perror("libusb_get_device_list"); exit(-1);}
	ssize_t i = 0;
	for(i=0;i<count;i++){
		libusb_device *device = list[i];
		struct libusb_device_descriptor desc;
		int status=libusb_get_device_descriptor(device,&desc);
		if(status!=0) continue; //SI ERREUR, RETOUR DEBUT DE BOUCLE
		uint8_t bus=libusb_get_bus_number(device);
		uint8_t address=libusb_get_device_address(device);
		if(desc.idProduct == id_product && desc.idVendor == id_vendor){
			printf("Device %x|%x Found @ (Bus:Address) %d:%d\n",id_vendor, id_product,bus,address);
			int success = libusb_open(device, handle);
			if(success!=0){perror("libusb_open"); return -1;}
			return 0;
		}
	}
	printf("target %x:%x not found\n",id_vendor,id_product);
	return -1;
}

int get_interface_number(libusb_device_handle * handle, int * configValue, int ** interfaces_list, int * nb_interface, EndPoint ** endpoint_list, int * nb_endpoint){
//Recupere le numero de l'interface utilise et le numero de la configuration du peripherique.
//Retourne 0 au succes, sinon -1
	int k;
	libusb_device * target_device = libusb_get_device(handle);
	struct libusb_config_descriptor * config;
	int endpoint_nb = 0;
	int success = libusb_get_config_descriptor(target_device,0,&config);
	if(success!=0){perror("libusb_get_config_descriptor"); return -1;}
	printf("configValue = %d\n",config->bConfigurationValue);
	*configValue = config->bConfigurationValue;	
	
	printf("Interfaces Number : %d\n",config->bNumInterfaces);
	printf("Max Power : %d\n",config->MaxPower);
	
	int N_interfaces = config->bNumInterfaces;
	struct libusb_interface interface;
	struct libusb_interface_descriptor desc_interface;
	struct libusb_endpoint_descriptor endpoint_desc;
	int tmp_interface_number;
	int n;
	*nb_interface = N_interfaces;
	*interfaces_list = (int *)malloc(N_interfaces*sizeof(int));
	for(n=0;n<N_interfaces;n++){
		interface = config->interface[n];
		desc_interface = interface.altsetting[0];
		tmp_interface_number = desc_interface.bInterfaceNumber;
		*((*interfaces_list)+n) = tmp_interface_number;
		for(k=0;k<desc_interface.bNumEndpoints;k++){
			endpoint_desc = desc_interface.endpoint[k];
		//	printf("Interface : %d -- Numéro : %d -- Point d'acces : %d -- Type d'acces : %d\n",n,tmp_interface_number,endpoint_desc.bEndpointAddress, endpoint_desc.bmAttributes);		
			if(endpoint_desc.bmAttributes == 3){
				endpoint_nb++;
				*endpoint_list = realloc(*endpoint_list, endpoint_nb * sizeof(EndPoint));
				((*endpoint_list)+(endpoint_nb-1))->interface = tmp_interface_number;
				((*endpoint_list)+(endpoint_nb-1))->address = endpoint_desc.bEndpointAddress;
				((*endpoint_list)+(endpoint_nb-1))->direction = (((endpoint_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0)?1:0);
			}
		}
	
	}
	*nb_endpoint = endpoint_nb;
	return 0;
}

int claim_interface(libusb_device_handle * handle, int configValue, int interfaceNumber){
	printf("CLAIM INTERFACE : Handle = %x -- ConfigValue = %d -- interfaceNumber = %d\n",handle,configValue,interfaceNumber);
	int status=libusb_claim_interface(handle,interfaceNumber);
	if(status!=0){perror("libusb_claim_interface"); return -1;}
	return 0;
}

int release_interface(libusb_device_handle * handle, int interfaceNumber){
	printf("RELEASE INTERFACE : Handle = %x -- interfaceNumber = %d\n",handle,interfaceNumber);
	int status = libusb_release_interface(handle,interfaceNumber);
	if(status!=0){perror("libusb_release_interface"); return -1;}
	return 0;
}

int claim_all_interfaces(libusb_device_handle * handle, int configValue, int * interfaces_list, int nb_interfaces){
	printf("CLAIM ALL INTERFACE : Handle = %x -- ConfigValue = %d\n",handle, configValue);
	int i;
	int status = libusb_set_configuration(handle,configValue);
	if(status!=0){perror("libusb_set_configuration"); return -1;}
	
	for(i=0;i<nb_interfaces;i++){
		claim_interface(handle, configValue, (*interfaces_list+i));
	}
	return 0;
}

int release_all_interfaces(libusb_device_handle * handle, int * interfaces_list, int nb_interfaces){
	printf("RELEASE ALL INTERFACE : Handle = %x \n",handle);
	int i;
	for(i=0;i<nb_interfaces;i++){
		int status = release_interface(handle, (*interfaces_list+i));
		if(status!=0){perror("release_all_interfaces"); return -1;}
	}	
	return 0;
}

int read_interruption(libusb_device_handle * handle, int endpoint_in){
	printf("READ INTERRUPTION : Handle = %x -- Endpoint IN = %d\n",handle, endpoint_in);
	unsigned char data;
	int transferred;
	int timeout = 5000;
	int status = libusb_interrupt_transfer(handle, endpoint_in, &data, 1, &transferred, timeout);
	if(status!=0){printf("status = %d\n",status);printf("%s\n",libusb_error_name(status)); perror("libusb_interrupt_transfer");}
	if(transferred==1){printf("data received !\n");
	printf("data = %d\n",data);}
	return 0;

}

int change_led_status(libusb_device_handle * handle, int endpoint_out, int led_state){
	printf("CHANGE LED STATUS : Handle = %x -- Endpoint OU = %d\n",handle, endpoint_out);
	unsigned char data;
	int transferred;
	int timeout = 1000;
	data = (led_state == 1)?0xF0:0x0F;
	printf("ep=%x\n",endpoint_out);
	int status = libusb_interrupt_transfer(handle, endpoint_out, &data, 1, &transferred, timeout);
	if(status!=0){printf("status = %d\n",status); perror("libusb_interrupt_transfer"); return -1; }
	return 0;
}

int release_kernel(libusb_device_handle * handle, int * interfaces_list, int nb_interfaces){ 
	printf("RELEASE KERNEL : Handle = %x\n",handle);
	int i;
	printf("INTERFACES : N1 %d -- N2 %d\n",*(interfaces_list+0),*(interfaces_list+1));
	for(i=0;i<nb_interfaces;i++){	
		if(libusb_kernel_driver_active(handle,*(interfaces_list+i))){
			int status=libusb_detach_kernel_driver(handle,*(interfaces_list+i));
			if(status!=0){perror("libusb_detach_kernel_driver"); exit(-1);}
		}
	}
	printf("RELEASE KERNEL : Success\n");
	return 0;
}


int main(){
	int id_vendor = 0xabcd;
	int id_product = 0x1234;
	int i;

	//INITIALISATION BIBLIOTHEQUE
	printf("Initialisation de la bibliotheque...\n");
	libusb_context *context;
	init_context(&context);
	
	//RECUPERATION DE LA POIGNEE DE LA CIBLE
	printf("Recuperation de la poignée de la cible...\n");
	libusb_device_handle * handle;
	int status = find_target(id_vendor,id_product,context,&handle);
	if(status == -1){printf("Arret du programme\n"); return -1;}
	printf("HANDLE = %x\n",handle);
	
	//RECUPERATION DE L INTERFACE D INTERRUPTION
	printf("Recuperation des interfaces et des points d'acces d'interruptions...\n");
	int configValue;
	EndPoint * endpoint_list = NULL;
	int nb_ep;
	
	int * interfaces = NULL;
	int nb_interfaces;
	
	get_interface_number(handle, &configValue,&interfaces, &nb_interfaces, &endpoint_list, &nb_ep);
	release_kernel(handle, interfaces, nb_interfaces);
	printf("CONFIG VALUE = %d\n",configValue);

	printf("Endpoint number : %d \n",nb_ep);
	int endpoints[2]; // ENDPOINT[0] => IN -------- ENDPOINT[1] => OUT
	for(i=0;i<nb_ep;i++){
		printf("Endpoint address : %d -- Endpoint direction : %d \n",endpoint_list[i].address,endpoint_list[i].direction);
		if(endpoint_list[i].direction == 1){
			endpoints[0] = endpoint_list[i].address;
		}
		else{
			endpoints[1] = endpoint_list[i].address;
		}
	}


	//RECLAMATION DE L INTERFACE
	printf("Reclamation des interfaces...\n");
	claim_all_interfaces(handle, configValue, interfaces, nb_interfaces);
	
	//GESTION DES INTERRUPTION
	printf("Lecture des interruptions...\n");
	read_interruption(handle, endpoints[0]);
	//change_led_status(handle, endpoints[1], 1);
	

	//LIBERATION DES INTERFACE ET CLOTURE DU CONTEXTE USB
	printf("Liberations des interfaces...\n");
	release_all_interfaces(handle, interfaces, nb_interfaces);
	libusb_close(handle);
	libusb_exit(context);
}
