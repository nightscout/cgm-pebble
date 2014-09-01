#include "pebble.h"

static Window *window;

static TextLayer *bg_layer = NULL;
static TextLayer *cgmtime_layer = NULL;
static BitmapLayer *icon_layer = NULL;
static BitmapLayer *cgmicon_layer = NULL;
static BitmapLayer *appicon_layer = NULL;
static BitmapLayer *batticon_layer = NULL;
static TextLayer *message_layer = NULL;    // BG DELTA & MESSAGE LAYER
static TextLayer *battlevel_layer = NULL;
static TextLayer *t1dname_layer = NULL;
static TextLayer *time_watch_layer = NULL;
static TextLayer *time_app_layer = NULL;
static TextLayer *date_app_layer = NULL;
static char time_watch_text[] = "00:00";
static char date_app_text[] = "Wed 13 ";
static GBitmap *icon_bitmap = NULL;
static GBitmap *appicon_bitmap = NULL;
static GBitmap *cgmicon_bitmap = NULL;
static GBitmap *specialvalue_bitmap = NULL;
static GBitmap *batticon_bitmap = NULL;

static AppSync sync;
static AppTimer *timer = NULL;

static uint8_t sync_buffer[256];
static char current_icon[124];

static char last_bg[124];
static int current_bg = 0;
static int current_isMMOL = 0;

static bool bluetooth_connected = true;

static char new_cgm_time[124];
static long current_cgm_time = 0;
static uint32_t current_cgm_timeago = 0;
static char formatted_cgm_timeago[10];
static int cgm_timeago_diff = 0;
static char cgm_label_buffer[5];

static char new_app_time[124];
static long current_app_time = 0;
static uint32_t current_app_timeago = 0;
static char formatted_app_timeago[10];
static int app_timeago_diff = 0;
static char app_label_buffer[5];

static uint8_t lastAlertTime = 0;

static bool specvalue_overwrite = false;
static bool hypolow_overwrite = false;
static bool biglow_overwrite = false;
static bool midlow_overwrite = false;
static bool low_overwrite = false;
static bool midhigh_overwrite = false;
static bool bighigh_overwrite = false;

static bool DoubleDownAlert = false;
static bool OfflineAlert = false;
static bool BluetoothAlert = false;

static time_t time_now = 0;

static const uint32_t const high[] = { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
static const uint32_t const low[] = { 1000,100,2000};

static const uint32_t const hypo[] = { 3200,200,3200 };
static const uint32_t const hyper[] = { 50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150 };

static const uint32_t const trend_high[] = { 200,200,1000,200,200,200,1000 };
static const uint32_t const trend_low[] = { 2000,200,1000 };

static const uint32_t const alert[] = { 500,200,1000 };

static const uint8_t const NO_ALERT = 0;
//static const uint8_t const LOW_ALERT = 1;
//static const uint8_t const MEDIUM_ALERT = 2;
static const uint8_t const HIGH_ALERT = 3;

static const uint32_t MINUTEAGO = 60;
static const uint32_t HOURAGO = 60*(60);
static const uint32_t DAYAGO = 24*(60*60);
static const uint32_t WEEKAGO = 7*(24*60*60);

// BG Ranges, MG/DL
static const uint8_t const SPECVALUE_BG_MGDL = 20;
static const uint8_t const HYPOLOW_BG_MGDL = 55;
static const uint8_t const BIGLOW_BG_MGDL = 60;
static const uint8_t const MIDLOW_BG_MGDL = 70;
static const uint8_t const LOW_BG_MGDL = 80;
static const uint16_t const HIGH_BG_MGDL = 180;
static const uint16_t const MIDHIGH_BG_MGDL = 240;
static const uint16_t const BIGHIGH_BG_MGDL = 300;

// BG Ranges, MMOL (IN INT, NOT WITH FLOATING POINT)
static const uint8_t const SPECVALUE_BG_MMOL = 11;
static const uint8_t const HYPOLOW_BG_MMOL = 30;
static const uint8_t const BIGLOW_BG_MMOL = 33;
static const uint8_t const MIDLOW_BG_MMOL = 39;
static const uint8_t const LOW_BG_MMOL = 44;
static const uint16_t const HIGH_BG_MMOL = 100;
static const uint16_t const MIDHIGH_BG_MMOL = 133;
static const uint16_t const BIGHIGH_BG_MMOL = 166;

// Snooze times
static const uint8_t const SNOOZE_5MIN = 5;
static const uint8_t const SNOOZE_10MIN = 10;
static const uint8_t const SNOOZE_15MIN = 15;
static const uint8_t const SNOOZE_30MIN = 30;
//static const uint8_t const SNOOZE_45MIN = 45;

enum CgmKey {
	CGM_ICON_KEY = 0x0,         // TUPLE_CSTRING
	CGM_BG_KEY = 0x1,           // TUPLE_CSTRING
	CGM_READTIME_KEY = 0x2,     // TUPLE_CSTRING
	CGM_ALERT_KEY = 0x3,        // TUPLE_INT
	CGM_TIME_NOW = 0x4,         // TUPLE_CSTRING
	CGM_DELTA_KEY = 0x5,        // TUPLE_CSTRING
	CGM_BATTLEVEL_KEY = 0x6,    // TUPLE_CSTRING
	CGM_T1DNAME_KEY = 0x7       // TUPLE_CSTRING
};

static const char NO_ARROW[] = "NONE";
static const char DOUBLEUP_ARROW[] = "DoubleUp";
static const char SINGLEUP_ARROW[] = "SingleUp";
static const char UP45_ARROW[] = "FortyFiveUp";
static const char FLAT_ARROW[] = "Flat";
static const char DOWN45_ARROW[] = "FortyFiveDown";
static const char SINGLEDOWN_ARROW[] = "SingleDown";
static const char DOUBLEDOWN_ARROW[] = "DoubleDown";
static const char NOTCOMPUTE_ICON[] = "NOT COMPUTABLE";
static const char OUTOFRANGE_ICON[] = "RATE OUT OF RANGE";

// mg/dl
static const uint8_t NO_ANTENNA_VALUE = 3;
static const uint8_t SENSOR_NOT_CALIBRATED_VALUE = 5;
static const uint8_t STOP_LIGHT_VALUE = 6;
static const uint8_t HOURGLASS_VALUE = 9;
static const uint8_t QUESTION_MARKS_VALUE = 10;
static const uint8_t BAD_RF_VALUE = 12;

//mmol
static const uint8_t MMOL_NO_ANTENNA_VALUE = 2;
static const uint8_t MMOL_SENSOR_NOT_CALIBRATED_VALUE = 3;
static const uint8_t MMOL_STOP_LIGHT_VALUE = 4;
static const uint8_t MMOL_HOURGLASS_VALUE = 5;
static const uint8_t MMOL_QUESTION_MARKS_VALUE = 6;
static const uint8_t MMOL_BAD_RF_VALUE = 7;


static const uint32_t SPECIAL_VALUE_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,             //0
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,   //1
	RESOURCE_ID_IMAGE_BLOOD_DROP,       //2
	RESOURCE_ID_IMAGE_STOP_LIGHT,       //3
	RESOURCE_ID_IMAGE_HOURGLASS,        //4
	RESOURCE_ID_IMAGE_QUESTION_MARKS    //5
};

static const uint32_t ARROW_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,     //0
	RESOURCE_ID_IMAGE_UPUP,     //1
	RESOURCE_ID_IMAGE_UP,       //2
	RESOURCE_ID_IMAGE_UP45,     //3
	RESOURCE_ID_IMAGE_FLAT,     //4
	RESOURCE_ID_IMAGE_DOWN45,   //5
	RESOURCE_ID_IMAGE_DOWN,     //6
	RESOURCE_ID_IMAGE_DOWNDOWN, //7
	RESOURCE_ID_IMAGE_LOGO      //8
};

static const uint32_t TIMEAGO_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,       //0
	RESOURCE_ID_IMAGE_PHONEON,    //1
	RESOURCE_ID_IMAGE_PHONEOFF,   //2
	RESOURCE_ID_IMAGE_RCVRON,     //3
	RESOURCE_ID_IMAGE_RCVROFF     //4
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

static char *translate_app_error(AppMessageResult result) {
  switch (result) {
	case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "APP UNKNOWN ERROR";
  }
}

static char *translate_dict_error(DictionaryResult result) {
  switch (result) {
    case DICT_OK: return "DICT_OK";
    case DICT_NOT_ENOUGH_STORAGE: return "DICT_NOT_ENOUGH_STORAGE";
    case DICT_INVALID_ARGS: return "DICT_INVALID_ARGS";
    case DICT_INTERNAL_INCONSISTENCY: return "DICT_INTERNAL_INCONSISTENCY";
    case DICT_MALLOC_FAILED: return "DICT_MALLOC_FAILED";
    default: return "DICT UNKNOWN ERROR";
  }
}

int myAtoi(char *str)
{
    int res = 0; // Initialize result
 
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i) {
      
        res = res*10 + str[i] - '0';
      
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "RES IN INT: %i", res ); 
      
    }
    return res;
} // end myAtoi

int myBGAtoi(char *str)
{
    int res = 0; // Initialize result
 
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i) {
      
      if ( (str[i] == ('.')) ) {
        current_isMMOL = 1;
      } 
      else {
        res = res*10 + str[i] - '0';
      }
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "RES IN BG INT: %i", res );   
    }
    return res;
} // end myBGAtoi

static void alert_handler(uint8_t alertValue)
{

  // APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER");
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE: %d", alertValue);
  
	switch (alertValue){

	case 0:
    //No alert
    //Normal (new data, in range, trend okay)
		break;
    
	case 1:;
    //Low
		//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: LOW ALERT");
		vibes_double_pulse();
		break;

	case 2:;
    // Medium
		//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: MEDIUM ALERT");
		vibes_double_pulse();
		VibePattern lowpat = {
			.durations = low,
			.num_segments = ARRAY_LENGTH(low),
		};
		vibes_enqueue_custom_pattern(lowpat);
		//vibes_double_pulse(lowpat);
		break;

	case 3:;
    // High
		//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: HIGH ALERT");
		VibePattern highpat = {
			.durations = high,
			.num_segments = ARRAY_LENGTH(high),
		};
		vibes_enqueue_custom_pattern(highpat);
		break;

	}
}

static void handle_bluetooth(bool bt_connected)
{
  if (bt_connected == false)
  {
	// Flag no bluetooth in logs
    APP_LOG(APP_LOG_LEVEL_INFO, "NO BLUETOOTH");
  
	text_layer_set_text(message_layer, "NO BLUETOOTH");
    
    // erase cgm and app ago times
    text_layer_set_text(cgmtime_layer, "");
    text_layer_set_text(time_app_layer, "");
    
    // erase cgm icon
	if (cgmicon_bitmap) {
	  gbitmap_destroy(cgmicon_bitmap);
	}
    cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
    bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);
    
    // turn phone icon off
	if (appicon_bitmap) {
	  gbitmap_destroy(appicon_bitmap);
	}
    appicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[2]);
    bitmap_layer_set_bitmap(appicon_layer, appicon_bitmap);
    
    // check if need to vibrate
    if (!BluetoothAlert) {
      alert_handler(HIGH_ALERT);
      BluetoothAlert = true;
    } 
  }	
  else {
	// Bluetooth is on, reset BluetoothAlert
	BluetoothAlert = false;
  }
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "BluetoothAlert: %i", BluetoothAlert);
} // end handle_bluetooth

// format current date from app
static void draw_date_from_app() {
 
  time_t d_app = time(NULL);
  struct tm *current_d_app = localtime(&d_app);
  size_t draw_return = 0;

  if (strcmp(time_watch_text, "00:00") == 0) {
    draw_return = strftime(time_watch_text, sizeof(time_watch_text), "%l:%M", current_d_app);
	if (draw_return != 0) {
      text_layer_set_text(time_watch_layer, time_watch_text);
	}
  }
  
  draw_return = strftime(date_app_text, sizeof(date_app_text), "%a %d", current_d_app);
  if (draw_return != 0) {
    text_layer_set_text(date_app_layer, date_app_text);
  }

} // end draw_date_from_app

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {

  DictionaryIterator *iter;
  AppMessageResult watchoff_openerr = APP_MSG_OK;
  AppMessageResult watchoff_senderr = APP_MSG_OK;
  
   //  WATCH OFFLINE debug logs
  APP_LOG(APP_LOG_LEVEL_INFO, "WATCH OFFLINE ERROR");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "WO APP ERR CODE: %i RES: %s", app_message_error, translate_app_error(app_message_error));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "WO DICT ERR CODE: %i RES: %s", dict_error, translate_dict_error(dict_error));

  watchoff_openerr = app_message_outbox_begin(&iter);
  
  if (watchoff_openerr == APP_MSG_OK) {
    // reset Offline alert
	OfflineAlert = false;
	
	// send message
    watchoff_senderr = app_message_outbox_send();
	if (watchoff_senderr != APP_MSG_OK ) {
	  APP_LOG(APP_LOG_LEVEL_INFO, "WATCH OFFLINE SEND ERROR");
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "WO SEND ERR CODE: %i RES: %s", watchoff_senderr, translate_app_error(watchoff_senderr));
	} 
	else {
	  return;
	}
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "WATCH OFFLINE RESEND ERROR");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "WO RESEND ERR CODE: %i RES: %s", watchoff_openerr, translate_app_error(watchoff_openerr));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "OfflineAlert:  %i", OfflineAlert);
    
  bluetooth_connected = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected) {
	handle_bluetooth(bluetooth_connected);
    return;
  }
  else {
	// Bluetooth is on, reset BluetoothAlert
	BluetoothAlert = false;
  }
    
  // set message to WATCH OFFLINE
  text_layer_set_text(message_layer, "WATCH OFFLINE");
    
  // erase cgm and app ago times
  text_layer_set_text(cgmtime_layer, "");
  text_layer_set_text(time_app_layer, "");
    
  // erase cgm icon
  if (cgmicon_bitmap) {
    gbitmap_destroy(cgmicon_bitmap);
  }
  cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
  bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);
    
  // turn phone icon off
  if (appicon_bitmap) {
    gbitmap_destroy(appicon_bitmap);
  }
  appicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[2]);
  bitmap_layer_set_bitmap(appicon_layer, appicon_bitmap);

  // check if need to vibrate
  if (!OfflineAlert) {
    alert_handler(HIGH_ALERT);
	OfflineAlert = true;
  } 
    
} // end sync_error_callback

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {

	// APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");

	switch (key) {

	case CGM_ICON_KEY:
   	// APP_LOG(APP_LOG_LEVEL_INFO, "ICON ARROW");
    // if SpecialValue already set, then break
    if (specialvalue_bitmap) {
      break;
    }
   
    // no SpecialValue, so set regular icon
	if (icon_bitmap) {
		gbitmap_destroy(icon_bitmap);
	}

    // get current Arrow Direction
    strncpy(current_icon, new_tuple->value->cstring, 124);               
    
    // check for arrow direction, set proper arrow icon
    if ( (strcmp(current_icon, NO_ARROW) == 0) || (strcmp(current_icon, NOTCOMPUTE_ICON) == 0) || (strcmp(current_icon, OUTOFRANGE_ICON) == 0) ) {
	  icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[0]);
	  DoubleDownAlert = false;
    } 
    else if (strcmp(current_icon, DOUBLEUP_ARROW) == 0) {
	  icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[1]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, SINGLEUP_ARROW) == 0) {
	  icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[2]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, UP45_ARROW) == 0) {
      icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[3]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, FLAT_ARROW) == 0) {
      icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[4]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, DOWN45_ARROW) == 0) {
	  icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[5]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, SINGLEDOWN_ARROW) == 0) {
      icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[6]);
	  DoubleDownAlert = false;
    }
    else if (strcmp(current_icon, DOUBLEDOWN_ARROW) == 0) {
		if (!DoubleDownAlert) {
		  alert_handler(HIGH_ALERT);
		  DoubleDownAlert = true;
		}
		icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[7]);
    } 
    else {
      icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[8]);
	  DoubleDownAlert = false;
    }

    bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
    break; // break for CGM_ICON_KEY

	case CGM_BG_KEY:
  
    // APP_LOG(APP_LOG_LEVEL_INFO, "BG CURRENT");
  
    if (specialvalue_bitmap) {
      gbitmap_destroy(specialvalue_bitmap);
    }
    
    // get MMOL value in myBGAtoi
    current_isMMOL = 0;
    strncpy(last_bg, new_tuple->value->cstring, 124);
    current_bg = myBGAtoi(last_bg);
    
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "CurrentBG: %d", current_bg);
    
    // check MMOL value and set current BG correctly
    if ( current_isMMOL == 1 ) {
      // MMOL Value
      current_isMMOL = 1;
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "MMOL, FLAG SHOULD BE ONE: %i", current_isMMOL);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST MMOL: %s", last_bg);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT MMOL: %i", current_bg);
    }
    else {
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "MGDL, FLAG SHOULD BE ZERO: %i", current_isMMOL);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST BG: %s", last_bg);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT BG: %i", current_bg);
    }
    
    if (current_isMMOL == 0) {
      
      // MG/DL; BG parse, check snooze, and set text 
      
      // check for init code or error code
      if (current_bg <= 0) {
        lastAlertTime = 0;
        text_layer_set_text(bg_layer, "");
		if (icon_bitmap) {
		  gbitmap_destroy(icon_bitmap);
		}
        icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[8]);
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);                  
        break;
      }
    
      // check for special value, if special value, then replace icon and blank BG; else send current BG    
      if ((current_bg == NO_ANTENNA_VALUE) || (current_bg == BAD_RF_VALUE)) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[1]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == SENSOR_NOT_CALIBRATED_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[2]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == STOP_LIGHT_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[3]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == HOURGLASS_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[4]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == QUESTION_MARKS_VALUE) {
      text_layer_set_text(bg_layer, "");
      specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[5]);
      bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else {
        text_layer_set_text(bg_layer, new_tuple->value->cstring);
      }
  
      // check BG and vibrate if needed
    
	  // check for SPECIAL VALUE
      if ( ( ((current_bg > 0) && (current_bg < SPECVALUE_BG_MGDL))
          && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
		|| ( ((current_bg > 0) && (current_bg <= SPECVALUE_BG_MGDL)) && (!specvalue_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SPECIAL VALUE BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite IN:  %i", specvalue_overwrite);
     
	 // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!specvalue_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          // don't know where we are coming from, so reset last alert time no matter what
		  // set to 1 to prevent bouncing connection
          lastAlertTime = 1;
          if (!specvalue_overwrite) { specvalue_overwrite = true; }
        }
      
	  // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
		  midhigh_overwrite = false;
		  bighigh_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite OUT:  %i", specvalue_overwrite);
      } 
      
	  // check for HYPO LOW BG and not SPECIAL VALUE
      else if ( ( ((current_bg > SPECVALUE_BG_MGDL) && (current_bg <= HYPOLOW_BG_MGDL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_5MIN)) ) 
           || ( ((current_bg > SPECVALUE_BG_MGDL) && (current_bg <= HYPOLOW_BG_MGDL)) && (!hypolow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "HYPO LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite IN:  %i", hypolow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!hypolow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!hypolow_overwrite) { hypolow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_5MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite OUT:  %i", hypolow_overwrite);
      }
	  
      // check for BIG LOW BG
      else if ( ( ((current_bg > HYPOLOW_BG_MGDL) && (current_bg <= BIGLOW_BG_MGDL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_5MIN)) ) 
           || ( ((current_bg > HYPOLOW_BG_MGDL) && (current_bg <= BIGLOW_BG_MGDL)) && (!biglow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "BIG LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite IN:  %i", biglow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!biglow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!biglow_overwrite) { biglow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_5MIN) { 
          lastAlertTime = 0;
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite OUT:  %i", biglow_overwrite);
      }
	 
      // check for MID LOW BG
      else if ( ( ((current_bg > BIGLOW_BG_MGDL) && (current_bg <= MIDLOW_BG_MGDL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_10MIN)) ) 
           || ( ((current_bg > BIGLOW_BG_MGDL) && (current_bg <= MIDLOW_BG_MGDL)) && (!midlow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "MID LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite IN:  %i", midlow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midlow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midlow_overwrite) { midlow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_10MIN) { 
          lastAlertTime = 0;
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite OUT:  %i", midlow_overwrite);
      }

      // check for LOW BG
      else if ( ( ((current_bg > MIDLOW_BG_MGDL) && (current_bg <= LOW_BG_MGDL)) 
               && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_15MIN)) )
             || ( ((current_bg > MIDLOW_BG_MGDL) && (current_bg <= LOW_BG_MGDL)) && (!low_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite IN:  %i", low_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!low_overwrite)) { 
          alert_handler(HIGH_ALERT); 
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!low_overwrite) { low_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_15MIN) {
          lastAlertTime = 0; 
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite OUT:  %i", low_overwrite);
      }  
      
      // check for HIGH BG
      else if ( ((current_bg >= HIGH_BG_MGDL) && (current_bg < MIDHIGH_BG_MGDL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "HIGH BG ALERT");    
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);   
      
        // send alert and handle a bouncing connection
        if (lastAlertTime == 0) { 
          alert_handler(HIGH_ALERT);
          lastAlertTime = 1;
        }
       
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) {
          lastAlertTime = 0; 
        } 
       
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
      } 
    
      // check for MID HIGH BG
      else if ( ( ((current_bg >= MIDHIGH_BG_MGDL) && (current_bg < BIGHIGH_BG_MGDL)) 
               && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
             || ( ((current_bg >= MIDHIGH_BG_MGDL) && (current_bg < BIGHIGH_BG_MGDL)) && (!midhigh_overwrite) ) ) {  
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "MID HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite IN:  %i", midhigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midhigh_overwrite)) { 
          alert_handler(HIGH_ALERT);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midhigh_overwrite) { midhigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0; 
          specvalue_overwrite = false;
		  midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite OUT:  %i", midhigh_overwrite);
      } 
  
      // check for BIG HIGH BG
      else if ( ( (current_bg >= BIGHIGH_BG_MGDL) 
               && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
               || ((current_bg >= BIGHIGH_BG_MGDL) && (!bighigh_overwrite)) ) {   
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "BIG HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite OUT:  %i", bighigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!bighigh_overwrite)) {
          alert_handler(HIGH_ALERT);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!bighigh_overwrite) { bighigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;		  
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite OUT:  %i", bighigh_overwrite);
      } 

      // else "normal" range or init code
      else if ( ((current_bg > LOW_BG_MGDL) && (current_bg < HIGH_BG_MGDL)) 
              || (current_bg <= 0) ) {
      
       alert_handler(NO_ALERT);
        // reset snooze counter
        lastAlertTime = 0;
      }
      
    } // end if MG/DL
  
    else {
      
      // MMOL; BG parse, check snooze, and set text 
      
      // check for init code or error code
      if (current_bg <= 0) {
        lastAlertTime = 0;
        text_layer_set_text(bg_layer, "");
		if (icon_bitmap) {
		   gbitmap_destroy(icon_bitmap);
		}
        icon_bitmap = gbitmap_create_with_resource(ARROW_ICONS[8]);
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);                  
        break;
      }
    
      // check for special value, if special value, then replace icon and blank BG; else send current BG    
      if ((current_bg == MMOL_NO_ANTENNA_VALUE) || (current_bg == MMOL_BAD_RF_VALUE)) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[1]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == MMOL_SENSOR_NOT_CALIBRATED_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[2]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == MMOL_STOP_LIGHT_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[3]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == MMOL_HOURGLASS_VALUE) {
        text_layer_set_text(bg_layer, "");
        specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[4]);
        bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else if (current_bg == MMOL_QUESTION_MARKS_VALUE) {
      text_layer_set_text(bg_layer, "");
      specialvalue_bitmap = gbitmap_create_with_resource(SPECIAL_VALUE_ICONS[5]);
      bitmap_layer_set_bitmap(icon_layer, specialvalue_bitmap);
      }
      else {
        text_layer_set_text(bg_layer, new_tuple->value->cstring);
      }
    
      // check BG and vibrate if needed
    
      // check for SPECIAL VALUE
      if ( ( ((current_bg > 0) && (current_bg < SPECVALUE_BG_MMOL))
          && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
		|| ( ((current_bg > 0) && (current_bg <= SPECVALUE_BG_MMOL)) && (!specvalue_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SPECIAL VALUE BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite IN:  %i", specvalue_overwrite);
     
	 // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!specvalue_overwrite)) { 
          alert_handler(HIGH_ALERT); 
		  // don't know where we are coming from, so reset last alert time no matter what
		  // set to 1 to prevent bouncing connection
          lastAlertTime = 1;
          if (!specvalue_overwrite) { specvalue_overwrite = true; }
        }
      
	  // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
		  midhigh_overwrite = false;
		  bighigh_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite OUT:  %i", specvalue_overwrite);
      } 
	  
	  // check for HYPO LOW BG and not SPECIAL VALUE
      else if ( ( ((current_bg > SPECVALUE_BG_MMOL) && (current_bg <= HYPOLOW_BG_MMOL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_5MIN)) ) 
           || ( ((current_bg > SPECVALUE_BG_MMOL) && (current_bg <= HYPOLOW_BG_MMOL)) && (!hypolow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "HYPO LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite IN:  %i", hypolow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!hypolow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!hypolow_overwrite) { hypolow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_5MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite OUT:  %i", hypolow_overwrite);
      }
	  
      // check for BIG LOW BG
      else if ( ( ((current_bg > HYPOLOW_BG_MMOL) && (current_bg <= BIGLOW_BG_MMOL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_5MIN)) ) 
           || ( ((current_bg > HYPOLOW_BG_MMOL) && (current_bg <= BIGLOW_BG_MMOL)) && (!biglow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "BIG LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite IN:  %i", biglow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!biglow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!biglow_overwrite) { biglow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_5MIN) { 
          lastAlertTime = 0;
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite OUT:  %i", biglow_overwrite);
      }
	 
      // check for MID LOW BG
      else if ( ( ((current_bg > BIGLOW_BG_MMOL) && (current_bg <= MIDLOW_BG_MMOL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_10MIN)) ) 
           || ( ((current_bg > BIGLOW_BG_MMOL) && (current_bg <= MIDLOW_BG_MMOL)) && (!midlow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "MID LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite IN:  %i", midlow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midlow_overwrite)) { 
          alert_handler(HIGH_ALERT);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midlow_overwrite) { midlow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_10MIN) { 
          lastAlertTime = 0;
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite OUT:  %i", midlow_overwrite);
      }

	 
      // check for LOW BG
      else if ( ( ((current_bg > MIDLOW_BG_MMOL) && (current_bg <= LOW_BG_MMOL)) 
               && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_15MIN)) )
             || ( ((current_bg > MIDLOW_BG_MMOL) && (current_bg <= LOW_BG_MMOL)) && (!low_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite IN:  %i", low_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!low_overwrite)) { 
          alert_handler(HIGH_ALERT); 
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!low_overwrite) { low_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_15MIN) {
          lastAlertTime = 0; 
		  specvalue_overwrite = false;
		  hypolow_overwrite = false;
		  biglow_overwrite = false;
		  midlow_overwrite = false;
		  low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite OUT:  %i", low_overwrite);
      }
      
      // check for HIGH BG
      else if ( ((current_bg >= HIGH_BG_MMOL) && (current_bg < MIDHIGH_BG_MMOL)) 
             && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) ) {
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "HIGH BG ALERT");    
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);   
      
        // send alert and handle a bouncing connection
        if (lastAlertTime == 0) { 
          alert_handler(HIGH_ALERT);
          lastAlertTime = 1;
        }
       
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) {
          lastAlertTime = 0; 
        } 
       
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
      } 
    
      // check for MID HIGH BG
      else if ( ( ((current_bg >= MIDHIGH_BG_MMOL) && (current_bg < BIGHIGH_BG_MMOL)) 
               && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
             || ( ((current_bg >= MIDHIGH_BG_MMOL) && (current_bg < BIGHIGH_BG_MMOL)) && (!midhigh_overwrite) ) ) {  
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "MID HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite IN:  %i", midhigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midhigh_overwrite)) { 
          alert_handler(HIGH_ALERT);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midhigh_overwrite) { midhigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0; 
          specvalue_overwrite = false;
		  midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite OUT:  %i", midhigh_overwrite);
      } 
  
      // check for BIG HIGH BG
      else if ( ( (current_bg >= BIGHIGH_BG_MMOL) 
              && ((lastAlertTime == 0) || (lastAlertTime == SNOOZE_30MIN)) )
              || ((current_bg >= BIGHIGH_BG_MMOL) && (!bighigh_overwrite)) ) {   
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "BIG HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite OUT:  %i", bighigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!bighigh_overwrite)) {
          alert_handler(HIGH_ALERT);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!bighigh_overwrite) { bighigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SNOOZE_30MIN) { 
          lastAlertTime = 0; 
          specvalue_overwrite = false;
		  midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite OUT:  %i", bighigh_overwrite);
      } 

      // else "normal" range or init code
      else if ( ((current_bg > LOW_BG_MMOL) && (current_bg < HIGH_BG_MMOL)) 
              || (current_bg <= 0) ) {
      
        alert_handler(NO_ALERT);
        // reset snooze counter
        lastAlertTime = 0;
      }      
    } // end else MMOL
      
    // APP_LOG(APP_LOG_LEVEL_INFO, "ALERT FROM BG");
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE FROM BG: %d", lastAlertTime);
    
    break; // break for CGM_BG_KEY

	case CGM_READTIME_KEY:
   	// APP_LOG(APP_LOG_LEVEL_INFO, "READ CGM TIME");
    
    if (cgmicon_bitmap) {
	  gbitmap_destroy(cgmicon_bitmap);
	}
    
    // initialize label buffer
    strncpy(cgm_label_buffer, "", 5);
    
    strncpy(new_cgm_time, new_tuple->value->cstring, 124);
    
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "NEW CGM TIME: %s", new_cgm_time);
    
    if (strcmp(new_cgm_time, "") == 0) {     
      // Init code or error code; set text layer & icon to empty value 
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CGM TIME AGO INIT OR ERROR CODE: %s", label_buffer);
      text_layer_set_text(cgmtime_layer, "");
      cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
      bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);                   
      break;
    }
    else {
      cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[3]);
      bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);
      
      current_cgm_time = atol(new_cgm_time);
      time_now = time(NULL);
      
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT CGM TIME: %lu", current_cgm_time);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "TIME NOW IN CGM: %lu", time_now);
        
      current_cgm_timeago = abs(time_now - current_cgm_time);
        
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);
      
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CGM TIME AGO LABEL IN: %s", label_buffer);
      
      if (current_cgm_timeago < MINUTEAGO) {
        cgm_timeago_diff = 0;
        strncpy (formatted_cgm_timeago, "now", 10);
      }
      else if (current_cgm_timeago < HOURAGO) {
        cgm_timeago_diff = (current_cgm_timeago / MINUTEAGO);
        snprintf(formatted_cgm_timeago, 10, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "m", 5);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else if (current_cgm_timeago < DAYAGO) {
        cgm_timeago_diff = (current_cgm_timeago / HOURAGO);
        snprintf(formatted_cgm_timeago, 10, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "h", 5);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else if (current_cgm_timeago < WEEKAGO) {
        cgm_timeago_diff = (current_cgm_timeago / DAYAGO);
        snprintf(formatted_cgm_timeago, 10, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "d", 5);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else {
        strncpy (formatted_cgm_timeago, "", 10);
		if (cgmicon_bitmap) {
		  gbitmap_destroy(cgmicon_bitmap);
		}
        cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
        bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);   
      }
      
      text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
          
      // check to see if we need to show receiver off icon
      if ( (cgm_timeago_diff > 6) || ( (strcmp(cgm_label_buffer, "") != 0) && (strcmp(cgm_label_buffer, "m") != 0) ) ) {
        // APP_LOG(APP_LOG_LEVEL_DEBUG, "SET PHONE OFF ICON: %s", label_buffer);
		if (cgmicon_bitmap) {
		  gbitmap_destroy(cgmicon_bitmap);
		}
        cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[4]);
        bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);
      }
    }
    
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "CGM TIME AGO LABEL OUT: %s", label_buffer);
    
    break;

	case CGM_ALERT_KEY:
	// APP_LOG(APP_LOG_LEVEL_INFO, "ALERT FROM PEBBLE");
	
	alert_handler(new_tuple->value->uint8);
	break;

	case CGM_TIME_NOW:;
	// APP_LOG(APP_LOG_LEVEL_INFO, "READ APP TIME NOW");
	  
	draw_date_from_app();
    
	if (appicon_bitmap) {
	  gbitmap_destroy(appicon_bitmap);
	}
    
    // initialize label buffer and icon
    strncpy(app_label_buffer, "", 5);    
	
    strncpy(new_app_time, new_tuple->value->cstring, 124);
    
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "NEW APP TIME: %s", new_app_time);
    
    // check for init or error code
    if (strcmp(new_app_time, "") == 0) {   
      text_layer_set_text(time_app_layer, "");
      appicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
      bitmap_layer_set_bitmap(appicon_layer, appicon_bitmap);                   
      break;
    }
    else {
      appicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[1]);
      bitmap_layer_set_bitmap(appicon_layer, appicon_bitmap);              
    
      current_app_time = atol(new_app_time);    
      time_now = time(NULL);
      
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT APP TIME: %lu", current_app_time);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "TIME NOW IN APP: %lu", time_now);
      
      current_app_timeago = abs(time_now - current_app_time);
      
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT APP TIMEAGO: %lu", current_app_timeago);
      
      if (current_app_timeago < (MINUTEAGO)) {
        app_timeago_diff = 0;
        strncpy (formatted_app_timeago, "OK", 10);
      }
      else if (current_app_timeago < HOURAGO) {
        app_timeago_diff = (current_app_timeago / MINUTEAGO);
        snprintf(formatted_app_timeago, 10, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "m", 5);
        strcat(formatted_app_timeago, app_label_buffer);
      } 
      else if (current_app_timeago < DAYAGO) {
        app_timeago_diff = (current_app_timeago / HOURAGO);
        snprintf(formatted_app_timeago, 10, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "h", 5);
        strcat(formatted_app_timeago, app_label_buffer);
      }
      else if (current_app_timeago < WEEKAGO) {
        app_timeago_diff = (current_app_timeago / DAYAGO);
        snprintf(formatted_app_timeago, 10, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "d", 5);
        strcat(formatted_app_timeago, app_label_buffer);
      }
      else {
        strncpy (formatted_app_timeago, "", 10);
      }
    
      text_layer_set_text(time_app_layer, formatted_app_timeago);
      
      // check to see if we need to set phone off icon
      if ( (app_timeago_diff > 1) || ( (strcmp(app_label_buffer, "") != 0) && (strcmp(app_label_buffer, "m") != 0) ) ) {
		if (appicon_bitmap) {
		  gbitmap_destroy(appicon_bitmap);
		}	  
        appicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[2]);
        bitmap_layer_set_bitmap(appicon_layer, appicon_bitmap);
        
        // erase cgm ago times
        text_layer_set_text(cgmtime_layer, "");
        
        // erase cgm icon
		if (cgmicon_bitmap) {
		  gbitmap_destroy(cgmicon_bitmap);
		}	  
        cgmicon_bitmap = gbitmap_create_with_resource(TIMEAGO_ICONS[0]);
        bitmap_layer_set_bitmap(cgmicon_layer, cgmicon_bitmap);
      }
   
    }  
    
    break;

	case CGM_DELTA_KEY:
   	// APP_LOG(APP_LOG_LEVEL_INFO, "DELTA IN BG");
	text_layer_set_text(message_layer, new_tuple->value->cstring);
    
    // If no bluetooth, set that message instead
    handle_bluetooth(bluetooth_connection_service_peek());
	break;
    
    case CGM_BATTLEVEL_KEY:;
   	// APP_LOG(APP_LOG_LEVEL_INFO, "BATTERY LEVEL");

    static uint8_t current_battlevel = 0;
    static char last_battlevel[4];
    static char battlevel_percent[6];
    
    if (batticon_bitmap) {
	  gbitmap_destroy(batticon_bitmap);
	}
    
    // get current battery level
    strncpy(last_battlevel, new_tuple->value->cstring, 4);
    current_battlevel = myAtoi(last_battlevel);
      
    // check for init code or no battery
    if (strcmp(last_battlevel, "") == 0) {
      // Init code or no battery, can't do battery; set text layer & icon to empty value 
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
    } // end else battlevel check

    break; // break for CGM_BATTLEVEL_KEY

    case CGM_T1DNAME_KEY:
    // APP_LOG(APP_LOG_LEVEL_INFO, "T1D NAME");
    text_layer_set_text(t1dname_layer, new_tuple->value->cstring);
    break;

  }  // end switch(key)

} // end sync_tuple_changed_callback()

static void send_cmd(void) {
  
  DictionaryIterator *iter;
  static AppMessageResult sendcmd_openerr = APP_MSG_OK;
  static AppMessageResult sendcmd_senderr = APP_MSG_OK;
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD IN, ABOUT TO OPEN APP MSG OUTBOX");
  
  sendcmd_openerr = app_message_outbox_begin(&iter);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, CHECK FOR ERROR");
          
  if (sendcmd_openerr != APP_MSG_OK) {
     APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD OPEN ERROR");
     APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD OPEN ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));

    return;
  }

  // APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, NO ERROR, ABOUT TO SEND MSG TO APP");
  
  sendcmd_senderr = app_message_outbox_send();
  
  if (sendcmd_senderr != APP_MSG_OK) {
     APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD SEND ERROR");
     APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD SEND ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
  }

  // APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD OUT, SENT MSG TO APP");
  
} // end send_cmd

static void timer_callback(void *data) {

  // APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK IN, TIMER POP, ABOUT TO CALL SEND CMD");
  
  send_cmd();
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, SEND CMD DONE, ABOUT TO REGISTER TIMER");
  
  timer = app_timer_register(60000, timer_callback, NULL);

  // APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, REGISTER TIMER DONE");
  
} // end timer_callback

// format current time from watch

static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  
  size_t tick_return = 0;
  
  if (units_changed & MINUTE_UNIT) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME MINUTE CODE");
    tick_return = strftime(time_watch_text, sizeof(time_watch_text), "%l:%M", tick_time);
	if (tick_return != 0) {
      text_layer_set_text(time_watch_layer, time_watch_text);
	}
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
	// increment BG snooze
    ++lastAlertTime;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
	
  } 
  else if (units_changed & DAY_UNIT) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME DAY CODE");
    tick_return = strftime(date_app_text, sizeof(date_app_text), "%a %d", tick_time);
	if (tick_return != 0) {
      text_layer_set_text(date_app_layer, date_app_text);
	}
  }
  
} // end handle_minute_tick

static void window_load(Window *window) {
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");
  
  Layer *window_layer = window_get_root_layer(window);
  
  // DELTA BG
  message_layer = text_layer_create(GRect(0, 33, 144, 55));
  text_layer_set_text_color(message_layer, GColorBlack);
  text_layer_set_background_color(message_layer, GColorWhite);
  text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(message_layer));

  // ARROW OR SPECIAL VALUE
  icon_layer = bitmap_layer_create(GRect(85, -7, 78, 50));
  bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
  bitmap_layer_set_background_color(icon_layer, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

  // APP TIME AGO ICON
  appicon_layer = bitmap_layer_create(GRect(118, 63, 40, 24));
  bitmap_layer_set_alignment(appicon_layer, GAlignLeft);
  bitmap_layer_set_background_color(appicon_layer, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(appicon_layer));  

  // APP TIME AGO READING
  time_app_layer = text_layer_create(GRect(77, 58, 40, 24));
  text_layer_set_text_color(time_app_layer, GColorBlack);
  text_layer_set_background_color(time_app_layer, GColorClear);
  text_layer_set_font(time_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(time_app_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(time_app_layer));
  
  // BG
  bg_layer = text_layer_create(GRect(0, -5, 95, 47));
  text_layer_set_text_color(bg_layer, GColorBlack);
  text_layer_set_background_color(bg_layer, GColorWhite);
  text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(bg_layer));

  // CGM TIME AGO ICON
  cgmicon_layer = bitmap_layer_create(GRect(5, 63, 40, 24));
  bitmap_layer_set_alignment(cgmicon_layer, GAlignLeft);
  bitmap_layer_set_background_color(cgmicon_layer, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(cgmicon_layer));  
  
  // CGM TIME AGO READING
  cgmtime_layer = text_layer_create(GRect(28, 58, 40, 24));
  text_layer_set_text_color(cgmtime_layer, GColorBlack);
  text_layer_set_background_color(cgmtime_layer, GColorClear);
  text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(cgmtime_layer));

  // T1D NAME
  t1dname_layer = text_layer_create(GRect(5, 138, 69, 28));
  text_layer_set_text_color(t1dname_layer, GColorWhite);
  text_layer_set_background_color(t1dname_layer, GColorClear);
  text_layer_set_font(t1dname_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(t1dname_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(t1dname_layer));

  // BATTERY LEVEL ICON
  batticon_layer = bitmap_layer_create(GRect(80, 147, 28, 20));
  bitmap_layer_set_alignment(batticon_layer, GAlignLeft);
  bitmap_layer_set_background_color(batticon_layer, GColorBlack);
  layer_add_child(window_layer, bitmap_layer_get_layer(batticon_layer));

  // BATTERY LEVEL
  battlevel_layer = text_layer_create(GRect(110, 144, 38, 20));
  text_layer_set_text_color(battlevel_layer, GColorWhite);
  text_layer_set_background_color(battlevel_layer, GColorBlack);
  text_layer_set_font(battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(battlevel_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(battlevel_layer));

  // CURRENT ACTUAL TIME FROM WATCH
  time_watch_layer = text_layer_create(GRect(0, 82, 144, 44));
  text_layer_set_text_color(time_watch_layer, GColorWhite);
  text_layer_set_background_color(time_watch_layer, GColorClear);
  text_layer_set_font(time_watch_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_watch_layer));
  
  // CURRENT ACTUAL DATE FROM APP
  date_app_layer = text_layer_create(GRect(0, 120, 144, 25));
  text_layer_set_text_color(date_app_layer, GColorWhite);
  text_layer_set_background_color(date_app_layer, GColorClear);
  text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
  draw_date_from_app();
  layer_add_child(window_layer, text_layer_get_layer(date_app_layer));

  Tuplet initial_values[] = {
    TupletCString(CGM_ICON_KEY, ""),
	TupletCString(CGM_BG_KEY, " "),
	TupletCString(CGM_READTIME_KEY, ""),
	TupletInteger(CGM_ALERT_KEY, 0),
	TupletCString(CGM_TIME_NOW, ""),
	TupletCString(CGM_DELTA_KEY, "LOADING..."),
	TupletCString(CGM_BATTLEVEL_KEY, ""),
	TupletCString(CGM_T1DNAME_KEY, "")
  };
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, ABOUT TO CALL APP SYNC INIT");
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, APP INIT DONE, ABOUT TO REGISTER TIMER");
  
  timer = app_timer_register(1000, timer_callback, NULL);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, TIMER REGISTER DONE");
  
} // end window_load

static void window_unload(Window *window) {
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD IN");
  
  app_sync_deinit(&sync);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }
  
  if (appicon_bitmap) {
    gbitmap_destroy(appicon_bitmap);
  }
  
  if (cgmicon_bitmap) {
    gbitmap_destroy(cgmicon_bitmap);
  }
  
  if (specialvalue_bitmap) {
    gbitmap_destroy(specialvalue_bitmap);
  }
  
  if (batticon_bitmap) {
    gbitmap_destroy(batticon_bitmap);
  }
  
  bitmap_layer_destroy(icon_layer);
  bitmap_layer_destroy(cgmicon_layer);
  bitmap_layer_destroy(appicon_layer);
  bitmap_layer_destroy(batticon_layer);
	
  text_layer_destroy(bg_layer);

  text_layer_destroy(cgmtime_layer);

  text_layer_destroy(message_layer);
  text_layer_destroy(battlevel_layer);
  text_layer_destroy(t1dname_layer);

  text_layer_destroy(time_watch_layer);
  text_layer_destroy(time_app_layer);

  text_layer_destroy(date_app_layer);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD OUT");
  
} // end window_unload

static void init(void) {
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");
  
  // subscribe to the tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

  // subscribe to the bluetooth connection service
  bluetooth_connection_service_subscribe(handle_bluetooth);
  
  // create the windows
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
	.unload = window_unload  
  });

  // APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, ABOUT TO CALL APP MSG OPEN");
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, APP MSG OPEN DONE");
  
  const bool animated = true;
  window_stack_push(window, animated);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE OUT");
  
}  // end init

static void deinit(void) {
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "DE-INIT CODE IN");
  
  // unsubscribe to the tick timer service
  tick_timer_service_unsubscribe();

  // unsubscribe to the bluetooth connection service
  bluetooth_connection_service_unsubscribe();
  
  // destroy the window
  window_destroy(window);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "DE-INIT CODE OUT");
  
} // end deinit

int main(void) {

  init();

  app_event_loop();
  deinit();
  
} // end main








