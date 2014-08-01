#include "pebble.h"

static Window *window;

static TextLayer *bg_layer;
static TextLayer *readtime_layer;
static BitmapLayer *icon_layer;
static TextLayer *message_layer;    // BG DELTA & MESSAGE LAYER
static TextLayer *time_layer;
static BitmapLayer *batticon_layer;
static TextLayer *battlevel_layer;
static TextLayer *t1dname_layer;
static TextLayer *date_layer;
static char time_text[] = "00:00";
static char date_text[] = "Wed 13 ";
static GBitmap *icon_bitmap = NULL;
static GBitmap *specialvalue_bitmap = NULL;
static GBitmap *batticon_bitmap = NULL;

static void draw_date() {

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(time_text, sizeof(time_text), "%l:%M", t);
	strftime(date_text, sizeof(date_text), "%a %d", t);

	text_layer_set_text(time_layer, time_text);
	text_layer_set_text(date_layer, date_text);
}

static AppSync sync;
static AppTimer *timer;

static uint8_t sync_buffer[256];
static char new_time[124];
static char last_bg[124];
static uint8_t current_bg = 0;

static const uint32_t const high[] = { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
static const uint32_t const low[] = { 1000,100,2000};

static const uint32_t const hypo[] = { 3200,200,3200 };
static const uint32_t const hyper[] = { 50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150 };

static const uint32_t const trend_high[] = { 200,200,1000,200,200,200,1000 };
static const uint32_t const trend_low[] = { 2000,200,1000 };

static const uint32_t const alert[] = { 500,200,1000 };


enum CgmKey {
	CGM_ICON_KEY = 0x0,         // TUPLE_BYTE_ARRAY
	CGM_BG_KEY = 0x1,           // TUPLE_CSTRING
	CGM_READTIME_KEY = 0x2,     // TUPLE_CSTRING
	CGM_ALERT_KEY = 0x3,        // TUPLE_INT
	CGM_TIME_NOW = 0x4,         // TUPLE_CSTRING
	CGM_DELTA_KEY = 0x5,        // TUPLE_CSTRING
        CGM_BATTLEVEL_KEY = 0x6,    // TUPLE_CSTRING
        CGM_T1DNAME_KEY = 0x7       // TUPLE_CSTRING
};

static const uint8_t NO_ANTENNA_VALUE = 3;
static const uint8_t SENSOR_NOT_CALIBRATED_VALUE = 5;
static const uint8_t STOP_LIGHT_VALUE = 6;
static const uint8_t HOURGLASS_VALUE = 9;
static const uint8_t QUESTION_MARKS_VALUE = 10;
static const uint8_t BAD_RF_VALUE = 12;

static const uint32_t SPECIAL_VALUE_ICONS[] = {
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,   //0
	RESOURCE_ID_IMAGE_BLOOD_DROP,       //1
	RESOURCE_ID_IMAGE_STOP_LIGHT,       //2
	RESOURCE_ID_IMAGE_HOURGLASS,        //3
	RESOURCE_ID_IMAGE_QUESTION_MARKS    //4
};

static const uint32_t CGM_ICONS[] = {		// COMMUNITY Build Order
	RESOURCE_ID_IMAGE_NONE,     //0
	RESOURCE_ID_IMAGE_UPUP,     //1
	RESOURCE_ID_IMAGE_UP,       //2
	RESOURCE_ID_IMAGE_UP45,     //3
	RESOURCE_ID_IMAGE_FLAT,     //4
	RESOURCE_ID_IMAGE_DOWN45,   //5
	RESOURCE_ID_IMAGE_DOWN,     //6
	RESOURCE_ID_IMAGE_DOWNDOWN, //7
	RESOURCE_ID_IMAGE_NONE,     //8
	RESOURCE_ID_IMAGE_NONE,     //9
	RESOURCE_ID_IMAGE_LOGO      //10
};

static const uint32_t BATTLEVEL_ICONS[] = {
	RESOURCE_ID_IMAGE_BATTEMPTY,  //0
	RESOURCE_ID_IMAGE_BATT10,     //1
	RESOURCE_ID_IMAGE_BATT20,     //2
	RESOURCE_ID_IMAGE_BATT30,     //3
	RESOURCE_ID_IMAGE_BATT40,     //4
	RESOURCE_ID_IMAGE_BATT50,     //5
	RESOURCE_ID_IMAGE_BATT60,     //6
	RESOURCE_ID_IMAGE_BATT70,     //7
	RESOURCE_ID_IMAGE_BATT80,     //8
	RESOURCE_ID_IMAGE_BATT90,     //9
	RESOURCE_ID_IMAGE_BATTFULL,   //10
	RESOURCE_ID_IMAGE_BATTNONE    //11
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	text_layer_set_text(message_layer, "WATCH OFFLINE");

	//    VibePattern pat = {
	//        .durations = alert,
	//       .num_segments = ARRAY_LENGTH(alert),
	//    };
	vibes_double_pulse();
	//vibes_enqueue_custom_pattern(pat);
}

static void alert_handler(uint8_t alertValue)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Alert code: %d", alertValue);

	switch (alertValue){
		//No alert
	case 0:
		break;

		//Normal (new data, in range, trend okay)
	case 1:
		vibes_double_pulse();
		break;

		//Low
	case 2:;
		VibePattern lowpat = {
			.durations = low,
			.num_segments = ARRAY_LENGTH(low),
		};
		vibes_enqueue_custom_pattern(lowpat);
		//vibes_double_pulse(lowpat);
		break;

		//High
	case 3:;
		VibePattern highpat = {
			.durations = high,
			.num_segments = ARRAY_LENGTH(high),
		};
		vibes_enqueue_custom_pattern(highpat);
		break;

		//Hypo

		//Hyper

		//Trend Low

		//Trend High

		//Data Alert

	}

}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {

	APP_LOG(APP_LOG_LEVEL_INFO, "sync tuple");
	switch (key) {

	case CGM_ICON_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "ICON ARROW");
                // if SpecialValue already set, then break
                if (specialvalue_bitmap) {
                   break;
                }

                // no SpecialValue, so set regular icon
		if (icon_bitmap) {
		   gbitmap_destroy(icon_bitmap);
		}

        icon_bitmap = gbitmap_create_with_resource(CGM_ICONS[new_tuple->value->uint8]);

		bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
		break;

	case CGM_BG_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "BG CURRENT");
		if (specialvalue_bitmap) {
		   gbitmap_destroy(specialvalue_bitmap);
		}

                // get current BG
                strncpy(last_bg, new_tuple->value->cstring, 124);
                current_bg = atoi(last_bg);

                // check for special value, if special value, then replace icon and blank BG; else send current BG
                if ((current_bg == NO_ANTENNA_VALUE) || (current_bg == BAD_RF_VALUE)) {
                   text_layer_set_text(bg_layer, "");
                   specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[0]);
                   bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
                }
                else if (current_bg == SENSOR_NOT_CALIBRATED_VALUE) {
                   text_layer_set_text(bg_layer, "");
                   specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[1]);
                   bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
                }
                else if (current_bg == STOP_LIGHT_VALUE) {
                   text_layer_set_text(bg_layer, "");
                   specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[2]);
                   bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
                }
                else if (current_bg == HOURGLASS_VALUE) {
                   text_layer_set_text(bg_layer, "");
                   specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[3]);
                   bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
                }
                else if (current_bg == QUESTION_MARKS_VALUE) {
                   text_layer_set_text(bg_layer, "");
                   specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[4]);
                   bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
                }
                else {
		   text_layer_set_text(bg_layer, new_tuple->value->cstring);
                }

                break; // break for CGM_BG_KEY

	case CGM_READTIME_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "READ TIME AGO");
		strncpy(new_time, new_tuple->value->cstring, 124);
		text_layer_set_text(readtime_layer, new_tuple->value->cstring);
		break;

	case CGM_ALERT_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "ALERT VIBRATION");
		alert_handler(new_tuple->value->uint8);
		break;

	case CGM_TIME_NOW:
   	APP_LOG(APP_LOG_LEVEL_INFO, "CGM TIME NOW");
		draw_date();
		break;

	case CGM_DELTA_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "DELTA IN BG");
		text_layer_set_text(message_layer, new_tuple->value->cstring);
		break;

        case CGM_BATTLEVEL_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "BATTERY LEVEL");

                static uint8_t current_battlevel = 0;
                static char last_battlevel[4];
                static char battlevel_percent[6];

		if (batticon_bitmap) {
		   gbitmap_destroy(batticon_bitmap);
		}

                // get current battery level
                strncpy(last_battlevel, new_tuple->value->cstring, 4);
                current_battlevel = atoi(last_battlevel);

                // check for init code or Rajat build (will be 111 if Rajat build)
                if  ( (strcmp(last_battlevel, "") == 0) || (current_battlevel == 111) ) {
                   // Init code or Rajat build, can't do battery; set text layer & icon to empty value
                   text_layer_set_text(battlevel_layer, "");
                   batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[11]);
                   bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   break;
                }
                else {
                   // get current battery level and set battery level text with percent
                   snprintf(battlevel_percent, 6, "%s%%", last_battlevel);
		   text_layer_set_text(battlevel_layer, battlevel_percent);

                   // check battery level, set battery level icon
                   if ( (current_battlevel >= 90) && (current_battlevel <= 100) ) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[10]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 80) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[9]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 70) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[8]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 60) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[7]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 50) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[6]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 40) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[5]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 30) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[4]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 20) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[3]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 10) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[2]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else if (current_battlevel >= 5) {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[1]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                   else {
                      batticon_bitmap = gbitmap_create_with_resource(BATTLEVEL_ICONS[0]);
                      bitmap_layer_set_bitmap(batticon_layer, batticon_bitmap);
                   }
                } // end else Init code or Rajat build

		break; // break for CGM_BATTLEVEL_KEY

        case CGM_T1DNAME_KEY:
        APP_LOG(APP_LOG_LEVEL_INFO, "T1D NAME");
                text_layer_set_text(t1dname_layer, new_tuple->value->cstring);
                break;

	}  // end switch(key)

} // end sync_tuple_changed_callback()

static void send_cmd(void) {

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) {
		return;
	}
	static char *bgptr = last_bg;
	static char *timeptr = new_time;

	Tuplet alertval = TupletInteger(3, 0);
	Tuplet bgVal = TupletCString(1, bgptr);
	Tuplet lastTimeVal = TupletCString(2, timeptr);

	dict_write_tuplet(iter, &alertval);
	dict_write_tuplet(iter, &bgVal);
	dict_write_tuplet(iter, &lastTimeVal);

	dict_write_end(iter);

	app_message_outbox_send();

}

static void timer_callback(void *data) {

	send_cmd();
	timer = app_timer_register(60000, timer_callback, NULL);

}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);

        // DELTA BG
        message_layer = text_layer_create(GRect(0, 33, 144, 55));
        text_layer_set_text_color(message_layer, GColorBlack);
        text_layer_set_background_color(message_layer, GColorWhite);
        text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
        text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(message_layer));

        //ARROW OR SPECIAL VALUE
        icon_layer = bitmap_layer_create(GRect(85, -7, 78, 50));
        bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
        bitmap_layer_set_background_color(icon_layer, GColorClear);
        layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

        //BG
        bg_layer = text_layer_create(GRect(0, -5, 95, 47));
        text_layer_set_text_color(bg_layer, GColorBlack);
        text_layer_set_background_color(bg_layer, GColorWhite);
        text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
        text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(bg_layer));

        //READ TIME AGO
        readtime_layer = text_layer_create(GRect(0, 58, 144, 28));
        text_layer_set_text_color(readtime_layer, GColorBlack);
        text_layer_set_background_color(readtime_layer, GColorClear);
        text_layer_set_font(readtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(readtime_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(readtime_layer));

        // T1D NAME
        t1dname_layer = text_layer_create(GRect(0, 138, 69, 28));
        text_layer_set_text_color(t1dname_layer, GColorWhite);
        text_layer_set_background_color(t1dname_layer, GColorClear);
        text_layer_set_font(t1dname_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(t1dname_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(t1dname_layer));

        // BATTERY LEVEL ICON
        batticon_layer = bitmap_layer_create(GRect(80, 147, 28, 20));
        bitmap_layer_set_alignment(batticon_layer, GAlignLeft);
        bitmap_layer_set_background_color(batticon_layer, GColorBlack);
        layer_add_child(window_layer, bitmap_layer_get_layer(batticon_layer));

        // BATTERY LEVEL
        battlevel_layer = text_layer_create(GRect(110, 144, 32, 20));
        text_layer_set_text_color(battlevel_layer, GColorWhite);
        text_layer_set_background_color(battlevel_layer, GColorBlack);
        text_layer_set_font(battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
        text_layer_set_text_alignment(battlevel_layer, GTextAlignmentLeft);
        layer_add_child(window_layer, text_layer_get_layer(battlevel_layer));

        // CURRENT ACTUAL TIME
        time_layer = text_layer_create(GRect(0, 82, 144, 44));
        text_layer_set_text_color(time_layer, GColorWhite);
        text_layer_set_background_color(time_layer, GColorClear);
        text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
        text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(time_layer));

        // CURRENT ACTUAL DATE
        date_layer = text_layer_create(GRect(0, 120, 144, 25));
        text_layer_set_text_color(date_layer, GColorWhite);
        text_layer_set_background_color(date_layer, GColorClear);
        text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
        layer_add_child(window_layer, text_layer_get_layer(date_layer));
        draw_date();


	Tuplet initial_values[] = {
		TupletInteger(CGM_ICON_KEY, 10),
		TupletCString(CGM_BG_KEY, ""),
		TupletCString(CGM_READTIME_KEY, ""),
		TupletInteger(CGM_ALERT_KEY, 0),
		TupletCString(CGM_TIME_NOW, ""),
		TupletCString(CGM_DELTA_KEY, "LOADING..."),
		TupletCString(CGM_BATTLEVEL_KEY, ""),
		TupletCString(CGM_T1DNAME_KEY, "")
	};

	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);

	timer = app_timer_register(1000, timer_callback, NULL);
}

static void window_unload(Window *window) {
	app_sync_deinit(&sync);

	if (icon_bitmap) {
		gbitmap_destroy(icon_bitmap);
	}
        if (specialvalue_bitmap) {
		gbitmap_destroy(specialvalue_bitmap);
	}
        if (batticon_bitmap) {
		gbitmap_destroy(batticon_bitmap);
	}
	text_layer_destroy(readtime_layer);
	text_layer_destroy(bg_layer);
	text_layer_destroy(message_layer);
	bitmap_layer_destroy(icon_layer);
	bitmap_layer_destroy(batticon_layer);
        text_layer_destroy(battlevel_layer);
        text_layer_destroy(t1dname_layer);
        //  bitmap_layer_destroy(time_layer);
	//  bitmap_layer_destroy(date_layer);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Time flies!");
    draw_date();
}

static void init(void) {
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_set_fullscreen(window, true);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
			.unload = window_unload
	});

	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

	const bool animated = true;
	window_stack_push(window, animated);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();

	app_event_loop();
	deinit();
}


