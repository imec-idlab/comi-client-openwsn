/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/
#ifndef __lwm2m_dev_H
#define __lwm2m_dev_H

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

typedef char uri_t;

//=========================== define ==========================================


//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   char  manufacturer[];
} res_manufacturer;				//0


typedef struct {
   coap_resource_desc_t desc;
   char  model_number[];
} res_model_number;				//1

//// LWM2M Objects////

typedef struct {
   coap_resource_desc_t desc;
   res_manufacturer manufacturer;					// 0
   res_model_number model_number;					// 1
} OMA_Device_Object;								// 3

typedef struct {
   coap_resource_desc_t desc;
   OMA_Device_Object device_object;
} lwm2m_device_t;




//=========================== variables =======================================




//=========================== prototypes ======================================

void lwm2m_dev_init(void);

/**
\}
\}
*/
void lwm2m_device_register(OMA_Device_Object* oma_device_object);

owerror_t lwm2m_device_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);

void    lwm2m_dev_sendDone(
   OpenQueueEntry_t* msg,
   owerror_t         error
);
#endif
