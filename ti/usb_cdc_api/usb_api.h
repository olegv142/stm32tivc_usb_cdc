#pragma once

/*
 * The API over USB implementation
 */

#include "api.h"

extern struct api g_api;

void usb_api_init(void);
void usb_api_process(void);
