#include "pebble.h"

static Window *window;

static TextLayer *bg_layer;
static TextLayer *readtime_layer;
static TextLayer *datetime_layer;
static BitmapLayer *icon_layer;
static TextLayer *message_layer;
static TextLayer *date_layer;
static char date_text[] = "Wed 13 ";
static GBitmap *icon_bitmap = NULL;

static void draw_date() {

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(date_text, sizeof(date_text), "%a %d", t);

	text_layer_set_text(date_layer, date_text);
}

static AppSync sync;

static uint8_t sync_buffer[256];
static char new_time[124];
static char last_bg[124];

static AppTimer *timer;

static const uint32_t const high[] = { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
static const uint32_t const low[] = { 1000,100,2000};

static const uint32_t const hypo[] = { 3200,200,3200 };
static const uint32_t const hyper[] = { 50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150 };

static const uint32_t const trend_high[] = { 200,200,1000,200,200,200,1000 };
static const uint32_t const trend_low[] = { 2000,200,1000 };

static const uint32_t const alert[] = { 500,200,1000 };


enum CgmKey {
	CGM_ICON_KEY = 0x0,         // TUPLE_INT
	CGM_BG_KEY = 0x1,           // TUPLE_CSTRING
	CGM_READTIME_KEY = 0x2,     // TUPLE_CSTRING
	CGM_ALERT_KEY = 0x3,        // TUPLE_INT
	CGM_TIME_NOW = 0x4,         // TUPLE_CSTRING
	CGM_DELTA_KEY = 0x5
};

static const uint32_t CGM_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,     //0
	RESOURCE_ID_IMAGE_UPUP,     //1
	RESOURCE_ID_IMAGE_UP,       //2
	RESOURCE_ID_IMAGE_UP45,     //3
	RESOURCE_ID_IMAGE_FLAT,     //4
	RESOURCE_ID_IMAGE_DOWN45,   //5
	RESOURCE_ID_IMAGE_DOWN,     //6
	RESOURCE_ID_IMAGE_DOWNDOWN  //7
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	text_layer_set_text(datetime_layer, "--:--");
	text_layer_set_text(message_layer, "Data: OFFLINE");

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
   	APP_LOG(APP_LOG_LEVEL_INFO, "ICON");
		if (icon_bitmap) {
			gbitmap_destroy(icon_bitmap);
		}
		icon_bitmap = gbitmap_create_with_resource(CGM_ICONS[new_tuple->value->uint8]);
		bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
		break;

	case CGM_BG_KEY:
    
   	APP_LOG(APP_LOG_LEVEL_INFO, "BG");
		text_layer_set_text(bg_layer, new_tuple->value->cstring);
		strncpy(last_bg, new_tuple->value->cstring, 124);
		break;

	case CGM_READTIME_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "readtime");
		strncpy(new_time, new_tuple->value->cstring, 124);
		text_layer_set_text(readtime_layer, new_tuple->value->cstring);
		break;

	case CGM_TIME_NOW:
    
   	APP_LOG(APP_LOG_LEVEL_INFO, "cgm time");
		draw_date();
		text_layer_set_text(datetime_layer, new_tuple->value->cstring);
		break;

	case CGM_ALERT_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "alert");
		alert_handler(new_tuple->value->uint8);
		break;

	case CGM_DELTA_KEY:
   	APP_LOG(APP_LOG_LEVEL_INFO, "delta");
		text_layer_set_text(message_layer, new_tuple->value->cstring);
		break;
	}

}

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


	icon_layer = bitmap_layer_create(GRect(82, 0, 61, 61));

	layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

	bg_layer = text_layer_create(GRect(0, 5, 83, 47));
	text_layer_set_text_color(bg_layer, GColorWhite);
	text_layer_set_background_color(bg_layer, GColorClear);
	text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(bg_layer));

	readtime_layer = text_layer_create(GRect(0, 48, 144, 22));
	text_layer_set_text_color(readtime_layer, GColorWhite);
	text_layer_set_background_color(readtime_layer, GColorClear);
	text_layer_set_font(readtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(readtime_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(readtime_layer));

	message_layer = text_layer_create(GRect(0, 69, 144, 22));
	text_layer_set_text_color(message_layer, GColorWhite);
	text_layer_set_background_color(message_layer, GColorBlack);
	text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(message_layer));

	datetime_layer = text_layer_create(GRect(0, 93, 144, 38));
	text_layer_set_text_color(datetime_layer, GColorBlack);
	text_layer_set_background_color(datetime_layer, GColorWhite);
	text_layer_set_font(datetime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(datetime_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(datetime_layer));
	//Date

	date_layer = text_layer_create(GRect(0, 127, 144, 42));
	text_layer_set_text_color(date_layer, GColorBlack);
	text_layer_set_background_color(date_layer, GColorWhite);
	text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(date_layer));

	draw_date();

	Tuplet initial_values[] = {
		TupletInteger(CGM_ICON_KEY, (uint8_t)0),
		TupletCString(CGM_BG_KEY, ""),
		TupletCString(CGM_READTIME_KEY, ""),
		TupletInteger(CGM_ALERT_KEY, 0),
		TupletCString(CGM_TIME_NOW, "loading..."),
		TupletCString(CGM_DELTA_KEY, " ")
	};

	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);

	timer = app_timer_register(1000, timer_callback, NULL);
}

static void window_unload(Window *window) {
	app_sync_deinit(&sync);

	if (icon_bitmap) {
		gbitmap_destroy(icon_bitmap);
	}
	text_layer_destroy(datetime_layer);
	text_layer_destroy(readtime_layer);
	text_layer_destroy(bg_layer);
	text_layer_destroy(message_layer);
	bitmap_layer_destroy(icon_layer);
	//  bitmap_layer_destroy(date_layer);
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
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();

	app_event_loop();
	deinit();
}
