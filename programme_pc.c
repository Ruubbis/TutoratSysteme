#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>


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
	printf("target %d:%d not found",id_vendor,id_product);
	return -1;
}

int get_interface_number(libusb_device_handle * handle, int * configValue, int * interfaceNumber){
//Recupere le numero de l'interface utilise et le numero de la configuration du peripherique.
//Retourne 0 au succes, sinon -1
	int k;
	libusb_device * target_device = libusb_get_device(handle);
	struct libusb_config_descriptor * config;

	int success = libusb_get_config_descriptor(target_device,0,&config);
	if(success!=0){perror("libusb_get_config_descriptor"); return -1;}
	*configValue = config->bConfigurationValue;	
	
	printf("Interfaces Number : %d\n",config->bNumInterfaces);
	printf("Max Power : %d\n",config->MaxPower);
	
	int N_interfaces = config->bNumInterfaces;
	struct libusb_interface interface;
	struct libusb_interface_descriptor desc_interface;
	struct libusb_endpoint_descriptor endpoint_desc;
	int tmp_interface_number;
	int n;
	for(n=0;n<N_interfaces;n++){
		interface = config->interface[n];
		desc_interface = interface.altsetting[0];
		tmp_interface_number = desc_interface.bInterfaceNumber;
		for(k=0;k<desc_interface.bNumEndpoints;k++){
			endpoint_desc = desc_interface.endpoint[k];
			printf("Interface : %d -- Numéro : %d -- Point d'acces : %d -- Type d'acces : %d\n",n,tmp_interface_number,endpoint_desc.bEndpointAddress, endpoint_desc.bmAttributes);		
			if(endpoint_desc.bmAttributes == 3){
			*interfaceNumber = tmp_interface_number;
			return 0;
			}
		}
	}
	return -1;
}

int main(){
	int id_vendor = 0x413c;
	int id_product = 0x2003;

	//INITIALISATION BIBLIOTHEQUE
	libusb_context *context;
	init_context(&context);

	//RECUPERATION DE LA POIGNEE DE LA CIBLE
	libusb_device_handle * handle;
	find_target(id_vendor,id_product,context,&handle);

	//RECUPERATION DE L INTERFACE D INTERRUPTION
	int configValue;
	int interfaceNumber;
	get_interface_number(handle, &configValue, &interfaceNumber);

	int status = libusb_set_configuration(handle,configValue);
	if(status!=0){perror("libusb_set_configuration"); exit(-1);}
	

	status=libusb_claim_interface(handle,interfaceNumber);
	if(status!=0){perror("libusb_claim_interface"); exit(-1); }
	

	status = libusb_release_interface(handle,interfaceNumber);
	if(status!=0){perror("libusb_release_interface"); exit(-1);}
	//CLOTURE DU CONTEXTE USB
	libusb_exit(context);
}
