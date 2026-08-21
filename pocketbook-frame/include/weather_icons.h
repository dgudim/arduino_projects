#pragma once

#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t weather_icon_sunny;
extern const lv_image_dsc_t weather_icon_clear_night;
extern const lv_image_dsc_t weather_icon_partlycloudy;
extern const lv_image_dsc_t weather_icon_partlycloudy_night;
extern const lv_image_dsc_t weather_icon_cloudy;
extern const lv_image_dsc_t weather_icon_fog;
extern const lv_image_dsc_t weather_icon_hail;
extern const lv_image_dsc_t weather_icon_lightning;
extern const lv_image_dsc_t weather_icon_lightning_rainy;
extern const lv_image_dsc_t weather_icon_pouring;
extern const lv_image_dsc_t weather_icon_rainy;
extern const lv_image_dsc_t weather_icon_snowy;
extern const lv_image_dsc_t weather_icon_snowy_rainy;
extern const lv_image_dsc_t weather_icon_windy;
extern const lv_image_dsc_t weather_icon_windy_variant;
extern const lv_image_dsc_t weather_icon_exceptional;

const lv_image_dsc_t *weather_icon_for_condition(const char *condition, bool night);

#ifdef __cplusplus
}
#endif
