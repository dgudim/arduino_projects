#pragma once

#include "ha_client.h"
#include <lvgl.h>

void dashboard_create(lv_display_t *disp);
void dashboard_update(const HaSnapshot &data);
