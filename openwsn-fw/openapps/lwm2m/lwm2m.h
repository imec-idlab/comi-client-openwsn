/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/
#ifndef __lwm2m_H
#define __lwm2m_H

/**
\addtogroup AppCoAP
\{
\addtogroup lwm2m
\{
*/

#include "opendefs.h"
#include "opentimers.h"
#include "opencoap.h"
#include "schedule.h"
#include "opensensors.h"
#include "sensors.h"

typedef char uri_t;

//=========================== define ==========================================
#define LESHANPERIOD  50000

//=========================== typedef =========================================
typedef enum{
	   TEMP_UNIT_C                      = 0,
	   TEMP_UNIT_F                      = 1,
	   TEMP_UNIT_K                      = 2,

}temp_units_t;

//// LWM2M Resources////
typedef struct {
   coap_resource_desc_t desc;
   float sensor_value;
} res_sensor_value;				//5700

typedef struct {
   coap_resource_desc_t desc;
   temp_units_t temp_unit;
} res_sensor_units;				//5701

typedef struct {
   coap_resource_desc_t desc;
   float min_range_value;
} res_min_range_value;			//5603

typedef struct {
   coap_resource_desc_t desc;
   float min_range_value;
} res_max_range_value;			//5604

typedef struct {
   coap_resource_desc_t desc;
   char  manufacturer[];
} res_manufacturer;				//0


typedef struct {
   coap_resource_desc_t desc;
   char  model_number[];
} res_model_number;				//1

typedef struct {
   coap_resource_desc_t desc;
   bool  onoff;
} res_onoff_number;				//5850

//// LWM2M Objects////

typedef struct {
   coap_resource_desc_t desc;
   res_sensor_value sensor_value;					//5700
   res_sensor_units sensor_units;					//5701
   res_min_range_value min_range_value;				//5603
   res_max_range_value max_range_value;				//5604
} IPSO_Temp_Object;									//3303

typedef struct {
   coap_resource_desc_t desc;
   res_sensor_value sensor_value;					//5700
   res_sensor_units sensor_units;					//5701
   res_min_range_value min_range_value;				//5603
   res_max_range_value max_range_value;				//5604
} IPSO_Hum_Object;									//3304

typedef struct {
   coap_resource_desc_t desc;
   res_onoff_number on_off_res;
} IPSO_Light_Object;									//3311

typedef struct {
   coap_resource_desc_t desc;
   res_manufacturer manufacturer;					// 0
   res_model_number model_number;					// 1
} OMA_Device_Object;								// 3

typedef struct {
   coap_resource_desc_t desc;
   opentimer_id_t timerId;
   uint8_t ipv6address[];
} leshan_t;


typedef struct {
   coap_resource_desc_t desc;
   IPSO_Temp_Object temp_objects;
   IPSO_Hum_Object hum_objects;
   IPSO_Light_Object led_objects;
   OMA_Device_Object device_object;
   leshan_t leshan;
} lwm2m_server_t;




//=========================== variables =======================================




//=========================== prototypes ======================================

void lwm2m_init(void);

/**
\}
\}
*/
void lwm2m_device_register(OMA_Device_Object* oma_device_object);
void lwm2m_temp_register(IPSO_Temp_Object* ipso_temp_object);
void lwm2m_hum_register(IPSO_Hum_Object* ipso_hum_object);
void lwm2m_led_register(IPSO_Light_Object* ipso_led_object);
void lwm2m_register_server_cb(opentimer_id_t id);
void lwm2m_register_server(void);

owerror_t lwm2m_device_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);

owerror_t lwm2m_temp_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);

owerror_t lwm2m_hum_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);

owerror_t lwm2m_led_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);

void    lwm2m_sendDone(
   OpenQueueEntry_t* msg,
   owerror_t         error
);
#endif
