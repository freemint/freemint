/*
 * FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define	udd_register 		(*api->udd_register)
#define	udd_unregister 		(*api->udd_unregister)
//#define	fname 			(*api->fname)

//#define	usb_init 		(*api->usb_init)
//#define	usb_stop 		(*api->usb_stop)

#define	usb_set_protocol 	(*api->usb_set_protocol)
#define	usb_set_idle 		(*api->usb_set_idle)
#define	usb_get_dev_index 	(*api->usb_get_dev_index)
#define	usb_control_msg 	(*api->usb_control_msg)
#define	usb_bulk_msg 		(*api->usb_bulk_msg)
#define	usb_submit_int_msg 	(*api->usb_submit_int_msg)
#define	usb_disable_asynch 	(*api->usb_disable_asynch)
#define	usb_maxpacket 		(*api->usb_maxpacket)
#define	usb_get_configuration_no 	(*api->usb_get_configuration_no)
#define	usb_get_report 		(*api->usb_get_report)
#define	usb_get_class_descriptor 	(*api->usb_get_class_descriptor)
#define	usb_clear_halt 		(*api->usb_clear_halt)
#define	usb_string 		(*api->usb_string)
#define	usb_set_interface 	(*api->usb_set_interface)
#define	usb_parse_config 	(*api->usb_parse_config)
#define	usb_set_maxpacket 	(*api->usb_set_maxpacket)
#define	usb_get_descriptor 	(*api->usb_get_descriptor)
#define	usb_set_address 	(*api->usb_set_address)
#define	usb_set_configuration 	(*api->usb_set_configuration)
#define	usb_get_string 		(*api->usb_get_string)
#define	usb_alloc_new_device 	(*api->usb_alloc_new_device)
#define	usb_new_device 		(*api->usb_new_device)

/* For now we leave this hub specific
 * stuff out of the api.
 */	
//#define	usb_get_hub_descriptor 	(*api->usb_get_hub_descriptor)
//#define	usb_clear_port_feature 	(*api->usb_clear_port_feature)
//#define	usb_get_hub_status 	(*api->usb_get_hub_status)
//#define	usb_set_port_feature 	(*api->usb_set_port_feature)
//#define	usb_get_port_status 	(*api->usb_get_port_status)
//#define	usb_hub_allocate 	(*api->usb_hub_allocate)
//#define	usb_hub_port_connect_change 	(*api->usb_hub_port_connect_change)
//#define	usb_hub_configure 	(*api->usb_hub_configure)	

