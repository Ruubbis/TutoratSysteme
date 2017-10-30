#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>


int main(){
	int id_vendor = 0x12d1;
	int id_product = 0x107e;
	int target = 0;
	int interfaces;
	int k;

	//INITIALISATION BIBLIOTHEQUE
	libusb_context *context;
	int status=libusb_init(&context);
	
	if(status!=0){perror("libusb_init"); exit(-1);}
	
	libusb_device **list;
	ssize_t count = libusb_get_device_list(context,&list);

	libusb_device_handle * handle;

	if(count<0) {perror("libusb_get_device_list"); exit(-1);}
	//DETECTION DU PERIPH SOUHAITE
	ssize_t i=0;
	for(i=0;i<count;i++){
		libusb_device *device=list[i];
		struct libusb_device_descriptor desc;
		int status=libusb_get_device_descriptor(device,&desc);
		if(status!=0) continue; //SI ERREUR, RETOUR DEBUT DE BOUCLE
		uint8_t bus=libusb_get_bus_number(device);
		uint8_t address=libusb_get_device_address(device);
		if(desc.idProduct == id_product && desc.idVendor == id_vendor){
			printf("Device %x|%x Found @ (Bus:Address) %d:%d\n",id_vendor, id_product,bus,address);
			int success = libusb_open(device, &handle);
			target = 1;
			if(success!=0){perror("libusb_open"); exit(-1);}
			}
	}
	if(target==1){
		libusb_device * target_device = libusb_get_device(handle);
		struct libusb_config_descriptor * config;
		int success = libusb_get_config_descriptor(target_device,0,&config);
		if(success!=0){perror("libusb_get_config_descriptor"); exit(-1);}
	printf("Interfaces Number : %d\n",config->bNumInterfaces);
	printf("Max Power : %d\n",config->MaxPower); 	
	int N_interfaces = config->bNumInterfaces;
	struct libusb_interface interface;
	struct libusb_interface_descriptor desc_interface;
	struct libusb_endpoint_descriptor endpoint_desc;
	int interface_number;
	int n;

	for(n=0;n<N_interfaces;n++){
		interface = config->interface[n];
		desc_interface = interface.altsetting[0];
		interface_number = desc_interface.bInterfaceNumber;
		for(k=0;k<desc_interface.bNumEndpoints;k++){
			endpoint_desc = desc_interface.endpoint[k];
			printf("Interface : %d -- Numéro : %d -- Point d'acces : %d -- Type d'acces : %d\n",n,interface_number,endpoint_desc.bEndpointAddress, endpoint_desc.bmAttributes);
			
		} 
	}	


	}
	else{printf("Target %x:%x not found\n",id_vendor,id_product);}
	//FREE
	libusb_free_device_list(list,1);
	
	//CLOTURE DU CONTEXTE USB
	libusb_exit(context);
}
