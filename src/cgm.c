#include "pebble.h"

// global window variables
// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

Window *window_cgm = NULL;

TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *message_layer = NULL;    // BG DELTA & MESSAGE LAYER
TextLayer *battlevel_layer = NULL;
TextLayer *t1dname_layer = NULL;
TextLayer *time_watch_layer = NULL;
TextLayer *time_app_layer = NULL;
TextLayer *date_app_layer = NULL;

BitmapLayer *icon_layer = NULL;
BitmapLayer *cgmicon_layer = NULL;
BitmapLayer *appicon_layer = NULL;
BitmapLayer *batticon_layer = NULL;

GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *cgmicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *batticon_bitmap = NULL;

InverterLayer *inv_battlevel_layer = NULL;

static char time_watch_text[] = "00:00";
static char date_app_text[] = "Wed 13 ";

// variables for AppSync
AppSync sync_cgm;
// CGM message is 57 bytes
// Pebble needs additional 62 Bytes?!? Pad with additional 20 bytes
static uint8_t sync_buffer_cgm[140];

// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
time_t time_now = 0;

// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;

// global variables for sync tuple functions
// buffers have to be static and hardcoded
static char current_icon[2];
static char last_bg[6];
static int current_bg = 0;
static bool currentBG_isMMOL = false;
static char last_battlevel[4];
static uint32_t current_cgm_time = 0;
static uint32_t current_app_time = 0;
static char current_bg_delta[10];
static int converted_bgDelta = 0;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static bool specvalue_alert = false;

// global overwrite variables for vibrating when hit a specific BG if already in a snooze
static bool specvalue_overwrite = false;
static bool hypolow_overwrite = false;
static bool biglow_overwrite = false;
static bool midlow_overwrite = false;
static bool low_overwrite = false;
static bool high_overwrite = false;
static bool midhigh_overwrite = false;
static bool bighigh_overwrite = false;

// global variables for vibrating in special conditions
static bool DoubleDownAlert = false;
static bool AppSyncErrAlert = false;
static bool AppMsgInDropAlert = false;
static bool AppMsgOutFailAlert = false;
static bool BluetoothAlert = false;
static bool BT_timer_pop = false;
static bool CGMOffAlert = false;
static bool PhoneOffAlert = false;
static bool LowBatteryAlert = false;

// global constants for time durations
static const uint8_t MINUTEAGO = 60;
static const uint16_t HOURAGO = 60*(60);
static const uint32_t DAYAGO = 24*(60*60);
static const uint32_t WEEKAGO = 7*(24*60*60);
static const uint16_t MS_IN_A_SECOND = 1000;

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string
static const uint8_t TIME_TEXTBUFF_SIZE = 6;
static const uint8_t DATE_TEXTBUFF_SIZE = 8;
static const uint8_t LABEL_BUFFER_SIZE = 6;
static const uint8_t TIMEAGO_BUFFER_SIZE = 10;

// ** START OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **
// ** FOR MMOL, ALL VALUES ARE STORED AS INTEGER; LAST DIGIT IS USED AS DECIMAL **
// ** BE EXTRA CAREFUL OF CHANGING SPECIAL VALUES OR TIMERS; DO NOT CHANGE WITHOUT EXPERT HELP **

// FOR BG RANGES
// DO NOT SET ANY BG RANGES EQUAL TO ANOTHER; LOW CAN NOT EQUAL MIDLOW
// LOW BG RANGES MUST BE IN ASCENDING ORDER; SPECVALUE < HYPOLOW < BIGLOW < MIDLOW < LOW
// HIGH BG RANGES MUST BE IN ASCENDING ORDER; HIGH < MIDHIGH < BIGHIGH
// DO NOT ADJUST SPECVALUE UNLESS YOU HAVE A VERY GOOD REASON
// DO NOT USE NEGATIVE NUMBERS OR DECIMAL POINTS OR ANYTHING OTHER THAN A NUMBER

// BG Ranges, MG/DL
static const uint16_t SPECVALUE_BG_MGDL = 20;
static const uint16_t SHOWLOW_BG_MGDL = 40;
static const uint16_t HYPOLOW_BG_MGDL = 55;
static const uint16_t BIGLOW_BG_MGDL = 60;
static const uint16_t MIDLOW_BG_MGDL = 70;
static const uint16_t LOW_BG_MGDL = 80;

static const uint16_t HIGH_BG_MGDL = 180;
static const uint16_t MIDHIGH_BG_MGDL = 240;
static const uint16_t BIGHIGH_BG_MGDL = 300;
static const uint16_t SHOWHIGH_BG_MGDL = 400;

// BG Ranges, MMOL
// VALUES ARE IN INT, NOT FLOATING POINT, LAST DIGIT IS DECIMAL
// FOR EXAMPLE : SPECVALUE IS 1.1, BIGHIGH IS 16.6
// ALWAYS USE ONE AND ONLY ONE DECIMAL POINT FOR LAST DIGIT
// GOOD : 5.0, 12.2 // BAD : 7 , 14.44
static const uint16_t SPECVALUE_BG_MMOL = 11;
static const uint16_t SHOWLOW_BG_MMOL = 22;
static const uint16_t HYPOLOW_BG_MMOL = 30;
static const uint16_t BIGLOW_BG_MMOL = 33;
static const uint16_t MIDLOW_BG_MMOL = 39;
static const uint16_t LOW_BG_MMOL = 44;

static const uint16_t HIGH_BG_MMOL = 100;
static const uint16_t MIDHIGH_BG_MMOL = 133;
static const uint16_t BIGHIGH_BG_MMOL = 166;
static const uint16_t SHOWHIGH_BG_MMOL = 222;

// BG Snooze Times, in Minutes; controls when vibrate again
// RANGE 0-240
static const uint8_t SPECVALUE_SNZ_MIN = 30;
static const uint8_t HYPOLOW_SNZ_MIN = 5;
static const uint8_t BIGLOW_SNZ_MIN = 5;
static const uint8_t MIDLOW_SNZ_MIN = 10;
static const uint8_t LOW_SNZ_MIN = 15;
static const uint8_t HIGH_SNZ_MIN = 30;
static const uint8_t MIDHIGH_SNZ_MIN = 30;
static const uint8_t BIGHIGH_SNZ_MIN = 30;
  
// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
static const uint8_t SPECVALUE_VIBE = 2;
static const uint8_t HYPOLOWBG_VIBE = 3;
static const uint8_t BIGLOWBG_VIBE = 3;
static const uint8_t LOWBG_VIBE = 3;
static const uint8_t HIGHBG_VIBE = 2;
static const uint8_t BIGHIGHBG_VIBE = 2;
static const uint8_t DOUBLEDOWN_VIBE = 3;
static const uint8_t APPSYNC_ERR_VIBE = 1;
static const uint8_t APPMSG_INDROP_VIBE = 1;
static const uint8_t APPMSG_OUTFAIL_VIBE = 1;
static const uint8_t BTOUT_VIBE = 1;
static const uint8_t CGMOUT_VIBE = 1;
static const uint8_t PHONEOUT_VIBE = 1;
static const uint8_t LOWBATTERY_VIBE = 1;

// Icon Cross Out & Vibrate Once Wait Times, in Minutes
// RANGE 0-240
// IF YOU WANT TO WAIT LONGER TO GET CONDITION, INCREASE NUMBER
static const uint8_t CGMOUT_WAIT_MIN = 15;
static const uint8_t PHONEOUT_WAIT_MIN = 10;

// Control Messages
// IF YOU DO NOT WANT A SPECIFIC MESSAGE, SET TO true
static const bool TurnOff_NOBLUETOOTH_Msg = false;
static const bool TurnOff_CHECKCGM_Msg = false;
static const bool TurnOff_CHECKPHONE_Msg = false;

// Control Vibrations
// IF YOU WANT NO VIBRATIONS, SET TO true
static const bool TurnOffAllVibrations = false;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO true
static const bool TurnOffStrongVibrations = false;

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WIL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
static const uint8_t BT_ALERT_WAIT_SECS = 45;

// ** END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **

// Message Timer Wait Times, in Seconds
static const uint8_t WATCH_MSGSEND_SECS = 60;
static const uint8_t LOADING_MSGSEND_SECS = 4;

enum CgmKey {
	CGM_ICON_KEY = 0x0,		// TUPLE_CSTRING, MAX 2 BYTES (10)
	CGM_BG_KEY = 0x1,		// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
	CGM_TCGM_KEY = 0x2,		// TUPLE_INT, 4 BYTES (CGM TIME)
	CGM_TAPP_KEY = 0x3,		// TUPLE_INT, 4 BYTES (APP / PHONE TIME)
	CGM_DLTA_KEY = 0x4,		// TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
	CGM_UBAT_KEY = 0x5,		// TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
	CGM_NAME_KEY = 0x6		// TUPLE_CSTRING, MAX 9 BYTES (Christine)
}; 
// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,             //0
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,   //1
	RESOURCE_ID_IMAGE_BLOOD_DROP,       //2
	RESOURCE_ID_IMAGE_STOP_LIGHT,       //3
	RESOURCE_ID_IMAGE_HOURGLASS,        //4
	RESOURCE_ID_IMAGE_QUESTION_MARKS,   //5
	RESOURCE_ID_IMAGE_LOGO,             //6
	RESOURCE_ID_IMAGE_ERR               //7
};
	
// INDEX FOR ARRAY OF SPECIAL VALUE ICONS
static const uint8_t NONE_SPECVALUE_ICON_INDX = 0;
static const uint8_t BROKEN_ANTENNA_ICON_INDX = 1;
static const uint8_t BLOOD_DROP_ICON_INDX = 2;
static const uint8_t STOP_LIGHT_ICON_INDX = 3;
static const uint8_t HOURGLASS_ICON_INDX = 4;
static const uint8_t QUESTION_MARKS_ICON_INDX = 5;
static const uint8_t LOGO_SPECVALUE_ICON_INDX = 6;
static const uint8_t ERR_SPECVALUE_ICON_INDX = 7;

// ARRAY OF TIMEAGO ICONS
static const uint8_t TIMEAGO_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,       //0
	RESOURCE_ID_IMAGE_PHONEON,    //1
	RESOURCE_ID_IMAGE_PHONEOFF,   //2
	RESOURCE_ID_IMAGE_RCVRON,     //3
	RESOURCE_ID_IMAGE_RCVROFF     //4
};

// INDEX FOR ARRAY OF TIMEAGO ICONS
static const uint8_t NONE_TIMEAGO_ICON_INDX = 0;
static const uint8_t PHONEON_ICON_INDX = 1;
static const uint8_t PHONEOFF_ICON_INDX = 2;
static const uint8_t RCVRON_ICON_INDX = 3;
static const uint8_t RCVROFF_ICON_INDX = 4;

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

int myAtoi(char *str) {

	// VARIABLES
    int res = 0; // Initialize result
 
	// CODE START
	
  //APP_LOG(APP_LOG_LEVEL_INFO, "MYATOI: ENTER CODE");
  
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i) {
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, STRING IN: %s", &str[i] );
      
      if ( (str[i] >= ('0')) && (str[i] <= ('9')) ) {
        res = res*10 + str[i] - '0';
      }
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FOR RESULT OUT: %i", res );
    }
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FINAL RESULT OUT: %i", res );
    return res;
} // end myAtoi

int myBGAtoi(char *str) {

	// VARIABLES
    int res = 0; // Initialize result
	
	// CODE START
 
    // initialize currentBG_isMMOL flag
	currentBG_isMMOL = false;
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, START currentBG is MMOL: %i", currentBG_isMMOL );
	
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i) {
      
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, STRING IN: %s", &str[i] );
	  
        if (str[i] == ('.')) {
        currentBG_isMMOL = true;
      }
      else if ( (str[i] >= ('0')) && (str[i] <= ('9')) ) {
        res = res*10 + str[i] - '0';
      }
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FOR RESULT OUT: %i", res );
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, currentBG is MMOL: %i", currentBG_isMMOL );	  
    }
 //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FINAL RESULT OUT: %i", res );
    return res;
} // end myBGAtoi

static void destroy_null_GBitmap(GBitmap **GBmp_image) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: ENTER CODE");
  
  if (*GBmp_image != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, DESTROY BITMAP IMAGE");
      gbitmap_destroy(*GBmp_image);
      if (*GBmp_image != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, SET POINTER TO NULL");
        *GBmp_image = NULL;
      }
	}
  
   //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: EXIT CODE");
} // end destroy_null_GBitmap

static void destroy_null_BitmapLayer(BitmapLayer **bmp_layer) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: ENTER CODE");
	
	if (*bmp_layer != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, DESTROY BITMAP LAYER");
      bitmap_layer_destroy(*bmp_layer);
      if (*bmp_layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, SET POINTER TO NULL");
        *bmp_layer = NULL;
      }
	}

  //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: EXIT CODE");
} // end destroy_null_BitmapLayer

static void destroy_null_TextLayer(TextLayer **txt_layer) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: ENTER CODE");
	
	if (*txt_layer != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, DESTROY TEXT LAYER");
      text_layer_destroy(*txt_layer);
      if (*txt_layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, SET POINTER TO NULL");
        *txt_layer = NULL;
      }
	}
//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: EXIT CODE");
} // end destroy_null_TextLayer

static void destroy_null_InverterLayer(InverterLayer **inv_layer) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL INVERTER LAYER: ENTER CODE");
	
	if (*inv_layer != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL INVERTER LAYER: POINTER EXISTS, DESTROY INVERTER LAYER");
      inverter_layer_destroy(*inv_layer);
      if (*inv_layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL INVERTER LAYER: POINTER EXISTS, SET POINTER TO NULL");
        *inv_layer = NULL;
      }
	}
//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL INVERTER LAYER: EXIT CODE");
} // end destroy_null_InverterLayer

static void create_update_bitmap(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id) {
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: ENTER CODE");
  
	// if bitmap pointer exists, destroy and set to NULL
	destroy_null_GBitmap(bmp_image);
	
	// create bitmap and pointer
  //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: CREATE BITMAP");
	*bmp_image = gbitmap_create_with_resource(resource_id);
  
	if (*bmp_image == NULL) {
      // couldn't create bitmap, return so don't crash
    //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: COULDNT CREATE BITMAP, RETURN");
      return;
	}
	else {
      // set bitmap
    //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: SET BITMAP");
      bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: EXIT CODE");
} // end create_update_bitmap

static void alert_handler_cgm(uint8_t alertValue) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER");
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE: %d", alertValue);
	
	// CONSTANTS
	// constants for vibrations patterns; has to be uint32_t, measured in ms, maximum duration 10000ms
	// Vibe pattern: ON, OFF, ON, OFF; ON for 500ms, OFF for 100ms, ON for 100ms; 
	
	// CURRENT PATTERNS
	const uint32_t highalert_fast[] = { 300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300 };
	const uint32_t medalert_long[] = { 500,100,100,100,500,100,100,100,500,100,100,100,500,100,100,100,500 };
	const uint32_t lowalert_beebuzz[] = { 75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75 };
	
	// PATTERN DURATION
	const uint8_t HIGHALERT_FAST_STRONG = 33;
	const uint8_t HIGHALERT_FAST_SHORT = (33/2);
	const uint8_t MEDALERT_LONG_STRONG = 17;
	const uint8_t MEDALERT_LONG_SHORT = (17/2);
	const uint8_t LOWALERT_BEEBUZZ_STRONG = 25;
	const uint8_t LOWALERT_BEEBUZZ_SHORT = (25/2);
	
	// PAST PATTERNS
	//const uint32_t hypo[] = { 3200,200,3200 };
	//const uint32_t low[] = { 1000,100,2000 };
	//const uint32_t hyper[] = { 50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150,50,150 };
	//const uint32_t trend_high[] = { 200,200,1000,200,200,200,1000 };
	//const uint32_t trend_low[] = { 2000,200,1000 };
	//const uint32_t alert[] = { 500,200,1000 };
  
	// CODE START
	
	if (TurnOffAllVibrations) {
      //turn off all vibrations is set, return out here
      return;
	}
	
	switch (alertValue) {

	case 0:
      //No alert
      //Normal (new data, in range, trend okay)
      break;
    
	case 1:;
      //Low
      //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: LOW ALERT");
      VibePattern low_alert_pat = {
			.durations = lowalert_beebuzz,
			.num_segments = LOWALERT_BEEBUZZ_STRONG,
      };
	  if (TurnOffStrongVibrations) { low_alert_pat.num_segments = LOWALERT_BEEBUZZ_SHORT; };
      vibes_enqueue_custom_pattern(low_alert_pat);
      break;

	case 2:;
      // Medium Alert
      //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: MEDIUM ALERT");
      VibePattern med_alert_pat = {
			.durations = medalert_long,
			.num_segments = MEDALERT_LONG_STRONG,
      };
	  if (TurnOffStrongVibrations) { med_alert_pat.num_segments = MEDALERT_LONG_SHORT; };
      vibes_enqueue_custom_pattern(med_alert_pat);
      break;

	case 3:;
      // High Alert
      //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: HIGH ALERT");
      VibePattern high_alert_pat = {
			.durations = highalert_fast,
			.num_segments = HIGHALERT_FAST_STRONG,
      };
	  if (TurnOffStrongVibrations) { high_alert_pat.num_segments = HIGHALERT_FAST_SHORT; };
      vibes_enqueue_custom_pattern(high_alert_pat);
      break;
  
	} // switch alertValue
	
} // end alert_handler_cgm

void BT_timer_callback(void *data);

void handle_bluetooth_cgm(bool bt_connected) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: ENTER CODE");
  
  if (bt_connected == false)
  {
	
	// Check BluetoothAlert for extended Bluetooth outage; if so, do nothing
	if (BluetoothAlert) {
      //Already vibrated and set message; out
	  return;
	}
	
	// Check to see if the BT_timer needs to be set; if BT_timer is not null we're still waiting
	if (BT_timer == NULL) {
	  // check to see if timer has popped
	  if (!BT_timer_pop) {
	    //set timer
	    BT_timer = app_timer_register((BT_ALERT_WAIT_SECS*MS_IN_A_SECOND), BT_timer_callback, NULL);
		// have set timer; next time we come through we will see that the timer has popped
		return;
	  }
	}
	else {
	  // BT_timer is not null and we're still waiting
	  return;
    }
	
	// timer has popped
	// Vibrate; BluetoothAlert takes over until Bluetooth connection comes back on
	//APP_LOG(APP_LOG_LEVEL_INFO, "BT HANDLER: TIMER POP, NO BLUETOOTH");
    alert_handler_cgm(BTOUT_VIBE);
    BluetoothAlert = true;
	
	// Reset timer pop
	BT_timer_pop = false;
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "NO BLUETOOTH");
    if (!TurnOff_NOBLUETOOTH_Msg) {
	  text_layer_set_text(message_layer, "NO BLUETOOTH");
	}
    
    // erase cgm and app ago times
    text_layer_set_text(cgmtime_layer, "");
    text_layer_set_text(time_app_layer, "");
    
	// erase cgm icon
    create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
    // turn phone icon off
    create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);
  }
    
  else {
	// Bluetooth is on, reset BluetoothAlert
    //APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: BLUETOOTH ON");
	BluetoothAlert = false;
    if (BT_timer == NULL) {
      // no timer is set, so need to reset timer pop
      BT_timer_pop = false;
    }
  }
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "BluetoothAlert: %i", BluetoothAlert);
} // end handle_bluetooth_cgm

void BT_timer_callback(void *data) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "BT TIMER CALLBACK: ENTER CODE");
	
	// reset timer pop and timer
	BT_timer_pop = true;
	if (BT_timer != NULL) {
	  BT_timer = NULL;
	}
	
	// check bluetooth and call handler
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
	handle_bluetooth_cgm(bluetooth_connected_cgm);
	
} // end BT_timer_callback

static void draw_date_from_app() {
 
  // VARIABLES
  time_t d_app = time(NULL);
  struct tm *current_d_app = localtime(&d_app);
  size_t draw_return = 0;

  // CODE START
  
  // format current date from app
  if (strcmp(time_watch_text, "00:00") == 0) {
    draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", current_d_app);
	if (draw_return != 0) {
      text_layer_set_text(time_watch_layer, time_watch_text);
	}
  }
  
  draw_return = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d", current_d_app);
  if (draw_return != 0) {
    text_layer_set_text(date_app_layer, date_app_text);
  }

} // end draw_date_from_app

void sync_error_callback_cgm(DictionaryResult appsync_dict_error, AppMessageResult appsync_error, void *context) {

  // VARIABLES
  DictionaryIterator *iter = NULL;
  AppMessageResult appsync_err_openerr = APP_MSG_OK;
  AppMessageResult appsync_err_senderr = APP_MSG_OK;
  
  // CODE START
  
  // APPSYNC ERROR debug logs
  APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC ERROR");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));

  bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_cgm) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  appsync_err_openerr = app_message_outbox_begin(&iter);
  
  if (appsync_err_openerr == APP_MSG_OK) {
    // reset AppSyncErrAlert to flag for vibrate
	AppSyncErrAlert = false;
	
	// send message
    appsync_err_senderr = app_message_outbox_send();
	if (appsync_err_senderr != APP_MSG_OK ) {
	  APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC SEND ERROR");
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
	} 
	else {
	  return;
	}
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC RESEND ERROR");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC RESEND ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppSyncErrAlert:  %i", AppSyncErrAlert);
    
  bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_cgm) {
    // bluetooth is out, BT message already set; return out
    return;
  }
    
  // set message to RESTART WATCH -> PHONE
  text_layer_set_text(message_layer, "RSTRT WCH/PH");
    
  // erase cgm and app ago times
  text_layer_set_text(cgmtime_layer, "");
  text_layer_set_text(time_app_layer, "");
    
  // erase cgm icon
  create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
    
  // turn phone icon off
  create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

  // check if need to vibrate
  if (!AppSyncErrAlert) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "APPSYNC ERROR: VIBRATE");
    alert_handler_cgm(APPSYNC_ERR_VIBE);
	AppSyncErrAlert = true;
  } 
    
} // end sync_error_callback_cgm

void inbox_dropped_handler_cgm(AppMessageResult appmsg_indrop_error, void *context) {
	// incoming appmessage send back from Pebble app dropped; no data received
	
	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_indrop_openerr = APP_MSG_OK;
	AppMessageResult appmsg_indrop_senderr = APP_MSG_OK;
  
	// CODE START
	
	// APPMSG IN DROP debug logs
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));

	bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
	if (!bluetooth_connected_cgm) {
      // bluetooth is out, BT message already set; return out
      return;
	}
  
	appmsg_indrop_openerr = app_message_outbox_begin(&iter);
  
	if (appmsg_indrop_openerr == APP_MSG_OK) {
      // reset AppMsgInDropAlert to flag for vibrate
	  AppMsgInDropAlert = false;
	
  	  // send message
      appmsg_indrop_senderr = app_message_outbox_send();
	  if (appmsg_indrop_senderr != APP_MSG_OK ) {
	    APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP SEND ERROR");
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP SEND ERR CODE: %i RES: %s", appmsg_indrop_senderr, translate_app_error(appmsg_indrop_senderr));
	  } 
	  else {
	    return;
	  }
  	}

	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP RESEND ERR CODE: %i RES: %s", appmsg_indrop_openerr, translate_app_error(appmsg_indrop_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgInDropAlert:  %i", AppMsgInDropAlert);
    
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
	if (!bluetooth_connected_cgm) {
      // bluetooth is out, BT message already set; return out
      return;
	}
    
	// set message to RESTART WATCH -> PHONE
	text_layer_set_text(message_layer, "RSTRT WCH/PHN");
    
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	text_layer_set_text(time_app_layer, "");
    
	// erase cgm icon
	create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
    
	// turn phone icon off
	create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppMsgInDropAlert) {
      //APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR: VIBRATE");
      alert_handler_cgm(APPMSG_INDROP_VIBE);
	  AppMsgInDropAlert = true;
	} 
    
} // end inbox_dropped_handler_cgm

void outbox_failed_handler_cgm(DictionaryIterator *failed, AppMessageResult appmsg_outfail_error, void *context) {
	// outgoing appmessage send failed to deliver to Pebble
	
	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_outfail_openerr = APP_MSG_OK;
	AppMessageResult appmsg_outfail_senderr = APP_MSG_OK;
  
	// CODE START
	
	// APPMSG OUT FAIL debug logs
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));

	bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
	if (!bluetooth_connected_cgm) {
      // bluetooth is out, BT message already set; return out
      return;
	}
  
	appmsg_outfail_openerr = app_message_outbox_begin(&iter);
  
	if (appmsg_outfail_openerr == APP_MSG_OK) {
      // reset AppMsgOutFailAlert to flag for vibrate
	  AppMsgOutFailAlert = false;
	
  	  // send message
      appmsg_outfail_senderr = app_message_outbox_send();
	  if (appmsg_outfail_senderr != APP_MSG_OK ) {
	    APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL SEND ERROR");
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL SEND ERR CODE: %i RES: %s", appmsg_outfail_senderr, translate_app_error(appmsg_outfail_senderr));
	  } 
	  else {
	    return;
	  }
  	}

	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL RESEND ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgOutFailAlert:  %i", AppMsgOutFailAlert);
    
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
	if (!bluetooth_connected_cgm) {
      // bluetooth is out, BT message already set; return out
      return;
	}
    
	// set message to RESTART WATCH -> PHONE
	text_layer_set_text(message_layer, "RSTRT WCH/PHN");
    
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	text_layer_set_text(time_app_layer, "");
    
	// erase cgm icon
	create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
    
	// turn phone icon off
	create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppMsgOutFailAlert) {
      //APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR: VIBRATE");
      alert_handler_cgm(APPMSG_OUTFAIL_VIBE);
	  AppMsgOutFailAlert = true;
	} 
	
} // end outbox_failed_handler_cgm

static void load_icon() {
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON ARROW FUNCTION START");
	
	// CONSTANTS
	
	// ICON ASSIGNMENTS OF ARROW DIRECTIONS
	const char NO_ARROW[] = "0";
	const char DOUBLEUP_ARROW[] = "1";
	const char SINGLEUP_ARROW[] = "2";
	const char UP45_ARROW[] = "3";
	const char FLAT_ARROW[] = "4";
	const char DOWN45_ARROW[] = "5";
	const char SINGLEDOWN_ARROW[] = "6";
	const char DOUBLEDOWN_ARROW[] = "7";
	const char NOTCOMPUTE_ICON[] = "8";
	const char OUTOFRANGE_ICON[] = "9";
	
	// ARRAY OF ARROW ICON IMAGES
	const uint8_t ARROW_ICONS[] = {
	  RESOURCE_ID_IMAGE_NONE,     //0
	  RESOURCE_ID_IMAGE_UPUP,     //1
	  RESOURCE_ID_IMAGE_UP,       //2
	  RESOURCE_ID_IMAGE_UP45,     //3
	  RESOURCE_ID_IMAGE_FLAT,     //4
	  RESOURCE_ID_IMAGE_DOWN45,   //5
	  RESOURCE_ID_IMAGE_DOWN,     //6
	  RESOURCE_ID_IMAGE_DOWNDOWN, //7
	  RESOURCE_ID_IMAGE_LOGO,     //8
	  RESOURCE_ID_IMAGE_ERR       //9
	};
    
	// INDEX FOR ARRAY OF ARROW ICON IMAGES
	const uint8_t NONE_ARROW_ICON_INDX = 0;
	const uint8_t UPUP_ICON_INDX = 1;
	const uint8_t UP_ICON_INDX = 2;
	const uint8_t UP45_ICON_INDX = 3;
	const uint8_t FLAT_ICON_INDX = 4;
	const uint8_t DOWN45_ICON_INDX = 5;
	const uint8_t DOWN_ICON_INDX = 6;
	const uint8_t DOWNDOWN_ICON_INDX = 7;
	const uint8_t LOGO_ARROW_ICON_INDX = 8;
	const uint8_t ERR_ARROW_ICON_INDX = 9;
	
  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ARROW ICON, BEFORE CHECK SPEC VALUE BITMAP");
	// check if special value set
	if (specvalue_alert == false) {
	
	  // no special value, set arrow
      // check for arrow direction, set proper arrow icon
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD ICON, CURRENT ICON: %s", current_icon);
      if ( (strcmp(current_icon, NO_ARROW) == 0) || (strcmp(current_icon, NOTCOMPUTE_ICON) == 0) || (strcmp(current_icon, OUTOFRANGE_ICON) == 0) ) {
	    create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[NONE_ARROW_ICON_INDX]);
	    DoubleDownAlert = false;
      } 
      else if (strcmp(current_icon, DOUBLEUP_ARROW) == 0) {
	    create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UPUP_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, SINGLEUP_ARROW) == 0) {
	    create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, UP45_ARROW) == 0) {
        create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP45_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, FLAT_ARROW) == 0) {
        create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[FLAT_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, DOWN45_ARROW) == 0) {
	    create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN45_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, SINGLEDOWN_ARROW) == 0) {
        create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN_ICON_INDX]);
	    DoubleDownAlert = false;
      }
      else if (strcmp(current_icon, DOUBLEDOWN_ARROW) == 0) {
	    if (!DoubleDownAlert) {
	      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, ICON ARROW: DOUBLE DOWN");
	      alert_handler_cgm(DOUBLEDOWN_VIBE);
	      DoubleDownAlert = true;
	    }
	    create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWNDOWN_ICON_INDX]);
      } 
      else {
	    // check for special cases and set icon accordingly
		// check bluetooth
	    bluetooth_connected_cgm = bluetooth_connection_service_peek();
        
	    // check to see if we are in the loading screen  
	    if (!bluetooth_connected_cgm) {
	      // Bluetooth is out; in the loading screen so set logo
	      create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[LOGO_ARROW_ICON_INDX]);
	    }
	    else {
		  // unexpected, set error icon
	      create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[ERR_ARROW_ICON_INDX]);
	    }
	    DoubleDownAlert = false;
	  }
	} // if specvalue_alert == false
	else { // this is just for log when need it
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, SPEC VALUE ALERT IS TRUE, DONE");
	} // else specvalue_alert == true
	
} // end load_icon

static void load_bg() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION START");
	
	// CONSTANTS

	// ARRAY OF BG CONSTANTS; MGDL
	uint16_t BG_MGDL[] = {
	  SPECVALUE_BG_MGDL,	//0
	  SHOWLOW_BG_MGDL,		//1
	  HYPOLOW_BG_MGDL,		//2
	  BIGLOW_BG_MGDL,		//3
	  MIDLOW_BG_MGDL,		//4
	  LOW_BG_MGDL,			//5
	  HIGH_BG_MGDL,			//6
	  MIDHIGH_BG_MGDL,		//7
	  BIGHIGH_BG_MGDL,		//8
	  SHOWHIGH_BG_MGDL		//9
	};
	
	// ARRAY OF BG CONSTANTS; MMOL
	uint16_t BG_MMOL[] = {
	  SPECVALUE_BG_MMOL,	//0
	  SHOWLOW_BG_MMOL,		//1
	  HYPOLOW_BG_MMOL,		//2
	  BIGLOW_BG_MMOL,		//3
	  MIDLOW_BG_MMOL,		//4
	  LOW_BG_MMOL,			//5
	  HIGH_BG_MMOL,			//6
	  MIDHIGH_BG_MMOL,		//7
	  BIGHIGH_BG_MMOL,		//8
	  SHOWHIGH_BG_MMOL		//9
	};
    
	// INDEX FOR ARRAYS OF BG CONSTANTS
	const uint8_t SPECVALUE_BG_INDX = 0;
	const uint8_t SHOWLOW_BG_INDX = 1;
	const uint8_t HYPOLOW_BG_INDX = 2;
	const uint8_t BIGLOW_BG_INDX = 3;
	const uint8_t MIDLOW_BG_INDX = 4;
	const uint8_t LOW_BG_INDX = 5;
	const uint8_t HIGH_BG_INDX = 6;
	const uint8_t MIDHIGH_BG_INDX = 7;
	const uint8_t BIGHIGH_BG_INDX = 8;
	const uint8_t SHOWHIGH_BG_INDX = 9;
	
	// MG/DL SPECIAL VALUE CONSTANTS ACTUAL VALUES
	// mg/dL = mmol / .0555 OR mg/dL = mmol * 18.0182
	const uint8_t SENSOR_NOT_ACTIVE_VALUE_MGDL = 1;		// show stop light, ?SN
	const uint8_t MINIMAL_DEVIATION_VALUE_MGDL = 2; 		// show stop light, ?MD
	const uint8_t NO_ANTENNA_VALUE_MGDL = 3; 			// show broken antenna, ?NA 
	const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MGDL = 5;	// show blood drop, ?NC
	const uint8_t STOP_LIGHT_VALUE_MGDL = 6;				// show stop light, ?CD
	const uint8_t HOURGLASS_VALUE_MGDL = 9;				// show hourglass, hourglass
	const uint8_t QUESTION_MARKS_VALUE_MGDL = 10;		// show ???, ???
	const uint8_t BAD_RF_VALUE_MGDL = 12;				// show broken antenna, ?RF

	// MMOL SPECIAL VALUE CONSTANTS ACTUAL VALUES
	// mmol = mg/dL / 18.0182 OR mmol = mg/dL * .0555
	const uint8_t SENSOR_NOT_ACTIVE_VALUE_MMOL = 1;		// show stop light, ?SN (.06 -> .1)
	const uint8_t MINIMAL_DEVIATION_VALUE_MMOL = 1;		// show stop light, ?MD (.11 -> .1)
	const uint8_t NO_ANTENNA_VALUE_MMOL = 2;				// show broken antenna, ?NA (.17 -> .2)
	const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MMOL = 3;	// show blood drop, ?NC (.28 -> .3)
	const uint8_t STOP_LIGHT_VALUE_MMOL = 4;				// show stop light, ?CD (.33 -> .3, set to .4 here)
	const uint8_t HOURGLASS_VALUE_MMOL = 5;				// show hourglass, hourglass (.50 -> .5)
	const uint8_t QUESTION_MARKS_VALUE_MMOL = 6;			// show ???, ??? (.56 -> .6)
	const uint8_t BAD_RF_VALUE_MMOL = 7;					// show broken antenna, ?RF (.67 -> .7)
	
	// ARRAY OF SPECIAL VALUES CONSTANTS; MGDL
	uint8_t SPECVALUE_MGDL[] = {
	  SENSOR_NOT_ACTIVE_VALUE_MGDL,		//0	
	  MINIMAL_DEVIATION_VALUE_MGDL,		//1
	  NO_ANTENNA_VALUE_MGDL,			//2
	  SENSOR_NOT_CALIBRATED_VALUE_MGDL,	//3
	  STOP_LIGHT_VALUE_MGDL,			//4
	  HOURGLASS_VALUE_MGDL,				//5
	  QUESTION_MARKS_VALUE_MGDL,		//6
	  BAD_RF_VALUE_MGDL					//7
	};
	
	// ARRAY OF SPECIAL VALUES CONSTANTS; MMOL
	uint8_t SPECVALUE_MMOL[] = {
	  SENSOR_NOT_ACTIVE_VALUE_MMOL,		//0	
	  MINIMAL_DEVIATION_VALUE_MMOL,		//1
	  NO_ANTENNA_VALUE_MMOL,			//2
	  SENSOR_NOT_CALIBRATED_VALUE_MMOL,	//3
	  STOP_LIGHT_VALUE_MMOL,			//4
	  HOURGLASS_VALUE_MMOL,				//5
	  QUESTION_MARKS_VALUE_MMOL,		//6
	  BAD_RF_VALUE_MMOL					//7
	};
	
	// INDEX FOR ARRAYS OF SPECIAL VALUES CONSTANTS
	const uint8_t SENSOR_NOT_ACTIVE_VALUE_INDX = 0;
	const uint8_t MINIMAL_DEVIATION_VALUE_INDX = 1;
	const uint8_t NO_ANTENNA_VALUE_INDX = 2;
	const uint8_t SENSOR_NOT_CALIBRATED_VALUE_INDX = 3;
	const uint8_t STOP_LIGHT_VALUE_INDX = 4;
	const uint8_t HOURGLASS_VALUE_INDX = 5;
	const uint8_t QUESTION_MARKS_VALUE_INDX = 6;
	const uint8_t BAD_RF_VALUE_INDX = 7;
  
	// VARIABLES 
	
	// pointers to be used to MGDL or MMOL values for parsing
	uint16_t *bg_ptr = NULL;
	uint8_t *specvalue_ptr = NULL;
	
	// CODE START
	
	// if special value set, erase anything in the icon field
	if (specvalue_alert == true) {
	  create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
	}
	
	// set special value alert to false no matter what
	specvalue_alert = false;

    // see if we're doing MGDL or MMOL; get currentBG_isMMOL value in myBGAtoi
	// convert BG value from string to int
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, BGATOI IN, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
    current_bg = myBGAtoi(last_bg);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, BG ATOI OUT, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST BG: %s", last_bg);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT BG: %i", current_bg);
    
    if (!currentBG_isMMOL) {
	  bg_ptr = BG_MGDL;
	  specvalue_ptr = SPECVALUE_MGDL;
	}
	else {
	  bg_ptr = BG_MMOL;
	  specvalue_ptr = SPECVALUE_MMOL;
    }
	
    // BG parse, check snooze, and set text 
      
    // check for init code or error code
    if ((current_bg <= 0) || (last_bg[0] == '-')) {
      lastAlertTime = 0;
      
      // check bluetooth
      bluetooth_connected_cgm = bluetooth_connection_service_peek();
      
      if (!bluetooth_connected_cgm) {
	    // Bluetooth is out; set BT message
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BG INIT: NO BT, SET NO BT MESSAGE");
		if (!TurnOff_NOBLUETOOTH_Msg) {
		  text_layer_set_text(message_layer, "NO BLUETOOTH");
		} // if turnoff nobluetooth msg
      }// if !bluetooth connected
      else {
	    // if init code, we will set it right in message layer
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED BG: SET ERR ICON");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, UNEXP BG, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
        text_layer_set_text(bg_layer, "ERR");
        create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
		specvalue_alert = true;
      }
      
	} // if current_bg <= 0
	  
    else {
      // valid BG
	  // check for special value, if special value, then replace icon and blank BG; else send current BG  
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BEFORE CREATE SPEC VALUE BITMAP");
	  if ((current_bg == specvalue_ptr[NO_ANTENNA_VALUE_INDX]) || (current_bg == specvalue_ptr[BAD_RF_VALUE_INDX])) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BROKEN ANTENNA");
		text_layer_set_text(bg_layer, "");
		create_update_bitmap(&specialvalue_bitmap,icon_layer, SPECIAL_VALUE_ICONS[BROKEN_ANTENNA_ICON_INDX]);
		specvalue_alert = true;
	  }
	  else if (current_bg == specvalue_ptr[SENSOR_NOT_CALIBRATED_VALUE_INDX]) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BLOOD DROP");
		text_layer_set_text(bg_layer, "");
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[BLOOD_DROP_ICON_INDX]);
	    specvalue_alert = true;        
	  }
	  else if ((current_bg == specvalue_ptr[SENSOR_NOT_ACTIVE_VALUE_INDX]) || (current_bg == specvalue_ptr[MINIMAL_DEVIATION_VALUE_INDX]) 
	        || (current_bg == specvalue_ptr[STOP_LIGHT_VALUE_INDX])) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET STOP LIGHT");
		text_layer_set_text(bg_layer, "");
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[STOP_LIGHT_ICON_INDX]);
		specvalue_alert = true;
	  }
	  else if (current_bg == specvalue_ptr[HOURGLASS_VALUE_INDX]) {
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET HOUR GLASS");
	    text_layer_set_text(bg_layer, "");
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[HOURGLASS_ICON_INDX]);
		specvalue_alert = true;
	  }
	  else if (current_bg == specvalue_ptr[QUESTION_MARKS_VALUE_INDX]) {
		//PP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
        text_layer_set_text(bg_layer, "");
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, SET BITMAP");
        create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[QUESTION_MARKS_ICON_INDX]); 
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, DONE");
		specvalue_alert = true;
	  }
	  else if (current_bg < bg_ptr[SPECVALUE_BG_INDX]) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED SPECIAL VALUE: SET ERR ICON");
		text_layer_set_text(bg_layer, "");
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[ERR_SPECVALUE_ICON_INDX]);
		specvalue_alert = true;
	  } // end special value checks
		
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, AFTER CREATE SPEC VALUE BITMAP");
      
	  if (specvalue_alert == false) {
		// we didn't find a special value, so set BG instead
		// arrow icon already set separately
		if (current_bg < bg_ptr[SHOWLOW_BG_INDX]) {
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG: SET TO LO");
		  text_layer_set_text(bg_layer, "LO");
		}
		else if (current_bg > bg_ptr[SHOWHIGH_BG_INDX]) {
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG: SET TO HI");
		  text_layer_set_text(bg_layer, "HI");
		}
		else {
		  // else update with current BG
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, SET TO BG: %s ", last_bg);
		  text_layer_set_text(bg_layer, last_bg);
		}
	  } // end bg checks (if special_value_bitmap)
  
      // check BG and vibrate if needed
    
	  // check for SPECIAL VALUE
      if ( ( ((current_bg > 0) && (current_bg < bg_ptr[SPECVALUE_BG_INDX]))
          && ((lastAlertTime == 0) || (lastAlertTime == SPECVALUE_SNZ_MIN)) )
		    || ( ((current_bg > 0) && (current_bg <= bg_ptr[SPECVALUE_BG_INDX])) && (!specvalue_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime SPEC VALUE SNOOZE VALUE IN: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite IN: %i", specvalue_overwrite);
     
	    // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!specvalue_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: VIBRATE");
          alert_handler_cgm(SPECVALUE_VIBE);        
          // don't know where we are coming from, so reset last alert time no matter what
		  // set to 1 to prevent bouncing connection
          lastAlertTime = 1;
          if (!specvalue_overwrite) { specvalue_overwrite = true; }
        }
      
	    // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == SPECVALUE_SNZ_MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime SPEC VALUE SNOOZE VALUE OUT: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "specvalue_overwrite OUT: %i", specvalue_overwrite);
      } // else if SPECIAL VALUE BG
      
	  // check for HYPO LOW BG and not SPECIAL VALUE
      else if ( ( ((current_bg > bg_ptr[SPECVALUE_BG_INDX]) && (current_bg <= bg_ptr[HYPOLOW_BG_INDX])) 
             && ((lastAlertTime == 0) || (lastAlertTime == HYPOLOW_SNZ_MIN)) ) 
           || ( ((current_bg > bg_ptr[SPECVALUE_BG_INDX]) && (current_bg <= bg_ptr[HYPOLOW_BG_INDX])) && (!hypolow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HYPO LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HYPO LOW SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite IN: %i", hypolow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!hypolow_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HYPO LOW: VIBRATE");
          alert_handler_cgm(HYPOLOWBG_VIBE);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!hypolow_overwrite) { hypolow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == HYPOLOW_SNZ_MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HYPO LOW SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "hypolow_overwrite OUT: %i", hypolow_overwrite);
      } // else if HYPO LOW BG
	  
      // check for BIG LOW BG
      else if ( ( ((current_bg > bg_ptr[HYPOLOW_BG_INDX]) && (current_bg <= bg_ptr[BIGLOW_BG_INDX])) 
             && ((lastAlertTime == 0) || (lastAlertTime == BIGLOW_SNZ_MIN)) ) 
           || ( ((current_bg > bg_ptr[HYPOLOW_BG_INDX]) && (current_bg <= bg_ptr[BIGLOW_BG_INDX])) && (!biglow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BIG LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime BIG LOW SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite IN: %i", biglow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!biglow_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BIG LOW: VIBRATE");
          alert_handler_cgm(BIGLOWBG_VIBE);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!biglow_overwrite) { biglow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == BIGLOW_SNZ_MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime BIG LOW SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "biglow_overwrite OUT: %i", biglow_overwrite);
      } // else if BIG LOW BG
	 
      // check for MID LOW BG
      else if ( ( ((current_bg > bg_ptr[BIGLOW_BG_INDX]) && (current_bg <= bg_ptr[MIDLOW_BG_INDX])) 
             && ((lastAlertTime == 0) || (lastAlertTime == MIDLOW_SNZ_MIN)) ) 
           || ( ((current_bg > bg_ptr[BIGLOW_BG_INDX]) && (current_bg <= bg_ptr[MIDLOW_BG_INDX])) && (!midlow_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, MID LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime MID LOW SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite IN: %i", midlow_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midlow_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, MID LOW: VIBRATE");
          alert_handler_cgm(LOWBG_VIBE);        
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midlow_overwrite) { midlow_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == MIDLOW_SNZ_MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime MID LOW SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midlow_overwrite OUT: %i", midlow_overwrite);
      } // else if MID HIGH BG

      // check for LOW BG
      else if ( ( ((current_bg > bg_ptr[MIDLOW_BG_INDX]) && (current_bg <= bg_ptr[LOW_BG_INDX])) 
               && ((lastAlertTime == 0) || (lastAlertTime == LOW_SNZ_MIN)) )
             || ( ((current_bg > bg_ptr[MIDLOW_BG_INDX]) && (current_bg <= bg_ptr[LOW_BG_INDX])) && (!low_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime LOW SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite IN: %i", low_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!low_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, LOW: VIBRATE");
          alert_handler_cgm(LOWBG_VIBE); 
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!low_overwrite) { low_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == LOW_SNZ_MIN) {
          lastAlertTime = 0; 
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime LOW SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite OUT: %i", low_overwrite);
      }  // else if LOW BG
      
      // check for HIGH BG
      else if ( ( ((current_bg >= bg_ptr[HIGH_BG_INDX]) && (current_bg < bg_ptr[MIDHIGH_BG_INDX])) 
             && ((lastAlertTime == 0) || (lastAlertTime == HIGH_SNZ_MIN)) ) 
           || ( ((current_bg >= bg_ptr[HIGH_BG_INDX]) && (current_bg < bg_ptr[MIDHIGH_BG_INDX])) && (!high_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HIGH BG ALERT");   
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HIGH SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "high_overwrite IN: %i", high_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!high_overwrite)) {  
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HIGH: VIBRATE");
          alert_handler_cgm(HIGHBG_VIBE);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!high_overwrite) { high_overwrite = true; }
        }
       
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == HIGH_SNZ_MIN) {
          lastAlertTime = 0; 
          specvalue_overwrite = false;
          high_overwrite = false;
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
       
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HIGH SNOOZE VALUE OUT: %i", lastAlertTime);
      } // else if HIGH BG
    
      // check for MID HIGH BG
      else if ( ( ((current_bg >= bg_ptr[MIDHIGH_BG_INDX]) && (current_bg < bg_ptr[BIGHIGH_BG_INDX])) 
               && ((lastAlertTime == 0) || (lastAlertTime == MIDHIGH_SNZ_MIN)) )
             || ( ((current_bg >= bg_ptr[MIDHIGH_BG_INDX]) && (current_bg < bg_ptr[BIGHIGH_BG_INDX])) && (!midhigh_overwrite) ) ) {  
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, MID HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime MID HIGH SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite IN: %i", midhigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!midhigh_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, MID HIGH: VIBRATE");
          alert_handler_cgm(HIGHBG_VIBE);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!midhigh_overwrite) { midhigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == MIDHIGH_SNZ_MIN) { 
          lastAlertTime = 0; 
          specvalue_overwrite = false;
          high_overwrite = false;
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime MID HIGH SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "midhigh_overwrite OUT: %i", midhigh_overwrite);
      } // else if MID HIGH BG
  
      // check for BIG HIGH BG
      else if ( ( (current_bg >= bg_ptr[BIGHIGH_BG_INDX]) 
               && ((lastAlertTime == 0) || (lastAlertTime == BIGHIGH_SNZ_MIN)) )
               || ((current_bg >= bg_ptr[BIGHIGH_BG_INDX]) && (!bighigh_overwrite)) ) {   
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BIG HIGH BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime BIG HIGH SNOOZE VALUE IN: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite IN: %i", bighigh_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!bighigh_overwrite)) {
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BIG HIGH: VIBRATE");
          alert_handler_cgm(BIGHIGHBG_VIBE);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!bighigh_overwrite) { bighigh_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime == BIGHIGH_SNZ_MIN) { 
          lastAlertTime = 0;
          specvalue_overwrite = false;
          high_overwrite = false;
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime BIG HIGH SNOOZE VALUE OUT: %i", lastAlertTime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "bighigh_overwrite OUT: %i", bighigh_overwrite);
      } // else if BIG HIGH BG

      // else "normal" range or init code
      else if ( ((current_bg > bg_ptr[LOW_BG_INDX]) && (current_bg < bg_ptr[HIGH_BG_INDX])) 
              || (current_bg <= 0) ) {
      
        // do nothing; just reset snooze counter
        lastAlertTime = 0;
      } // else if "NORMAL RANGE" BG
	  
    } // else if current bg <= 0
      
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION OUT");
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, FUNCTION OUT, SNOOZE VALUE: %d", lastAlertTime);
	
} // end load_bg

static void load_cgmtime() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME FUNCTION START");
	
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_cgm_timeago = 0;
	int cgm_timeago_diff = 0;
	static char formatted_cgm_timeago[10];
	static char cgm_label_buffer[6];	
    
	// CODE START
	
    // initialize label buffer
    strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);
       
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, NEW CGM TIME: %s", new_cgm_time);
    
    if (current_cgm_time == 0) {     
      // Init code or error code; set text layer & icon to empty value 
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
      text_layer_set_text(cgmtime_layer, "");
      create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);            
    }
    else {
	  // set rcvr on icon
      create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);
      
      time_now = time(NULL);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, TIME NOW IN CGM: %lu", time_now);
        
      current_cgm_timeago = abs(time_now - current_cgm_time);
        
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, GM TIME AGO LABEL IN: %s", cgm_label_buffer);
      
      if (current_cgm_timeago < MINUTEAGO) {
        cgm_timeago_diff = 0;
        strncpy (formatted_cgm_timeago, "now", TIMEAGO_BUFFER_SIZE);
      }
      else if (current_cgm_timeago < HOURAGO) {
        cgm_timeago_diff = (current_cgm_timeago / MINUTEAGO);
        snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "m", LABEL_BUFFER_SIZE);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else if (current_cgm_timeago < DAYAGO) {
        cgm_timeago_diff = (current_cgm_timeago / HOURAGO);
        snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "h", LABEL_BUFFER_SIZE);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else if (current_cgm_timeago < WEEKAGO) {
        cgm_timeago_diff = (current_cgm_timeago / DAYAGO);
        snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
        strncpy(cgm_label_buffer, "d", LABEL_BUFFER_SIZE);
        strcat(formatted_cgm_timeago, cgm_label_buffer);
      }
      else {
        strncpy (formatted_cgm_timeago, "ERR", TIMEAGO_BUFFER_SIZE);
        create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);        
      }
      
      text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
          
      // check to see if we need to show receiver off icon
      if ( (cgm_timeago_diff >= CGMOUT_WAIT_MIN) || ( (strcmp(cgm_label_buffer, "") != 0) && (strcmp(cgm_label_buffer, "m") != 0) ) ) {
	    // set receiver off icon
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, SET RCVR OFF ICON, CGM TIMEAGO DIFF: %d", cgm_timeago_diff);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, SET RCVR OFF ICON, LABEL: %s", cgm_label_buffer);
		create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVROFF_ICON_INDX]);
		
		// Vibrate if we need to
		if ((!CGMOffAlert) && (!PhoneOffAlert)) {
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME, CGM TIMEAGO: VIBRATE");
		  alert_handler_cgm(CGMOUT_VIBE);
		  CGMOffAlert = true;
		}
	  }
	  else {
	    // reset CGMOffAlert
        CGMOffAlert = false;
      }		
    } // else init code
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIMEAGO LABEL OUT: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_apptime(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, READ APP TIME FUNCTION START");
	
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_app_timeago = 0;
	int app_timeago_diff = 0;
	static char formatted_app_timeago[10];
	static char app_label_buffer[6];
	  
	// CODE START
	
	draw_date_from_app();
    
    // initialize label buffer and icon
    strncpy(app_label_buffer, "", LABEL_BUFFER_SIZE);    
        
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, NEW APP TIME: %lu", current_app_time);
    
    // check for init or error code
    if (current_app_time == 0) {   
      text_layer_set_text(time_app_layer, "");
      create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);          
    }
    else {
	  // set phone on icon
      create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEON_ICON_INDX]);       
       
      time_now = time(NULL);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, TIME NOW: %lu", time_now);
      
      current_app_timeago = abs(time_now - current_app_time);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, CURRENT APP TIMEAGO: %lu", current_app_timeago);
      
      if (current_app_timeago < (MINUTEAGO)) {
        app_timeago_diff = 0;
        strncpy (formatted_app_timeago, "OK", TIMEAGO_BUFFER_SIZE);
      }
      else if (current_app_timeago < HOURAGO) {
        app_timeago_diff = (current_app_timeago / MINUTEAGO);
        snprintf(formatted_app_timeago, TIMEAGO_BUFFER_SIZE, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "m", LABEL_BUFFER_SIZE);
        strcat(formatted_app_timeago, app_label_buffer);
      } 
      else if (current_app_timeago < DAYAGO) {
        app_timeago_diff = (current_app_timeago / HOURAGO);
        snprintf(formatted_app_timeago, TIMEAGO_BUFFER_SIZE, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "h", LABEL_BUFFER_SIZE);
        strcat(formatted_app_timeago, app_label_buffer);
      }
      else if (current_app_timeago < WEEKAGO) {
        app_timeago_diff = (current_app_timeago / DAYAGO);
        snprintf(formatted_app_timeago, TIMEAGO_BUFFER_SIZE, "%i", app_timeago_diff);
        strncpy(app_label_buffer, "d", LABEL_BUFFER_SIZE);
        strcat(formatted_app_timeago, app_label_buffer);
      }
      else {
        strncpy (formatted_app_timeago, "ERR", TIMEAGO_BUFFER_SIZE);
		create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
      }
	  
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, FORMATTED APP TIMEAGO STRING: %s", formatted_app_timeago);
      text_layer_set_text(time_app_layer, formatted_app_timeago);
      
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, CHECK FOR PHONE OFF ICON");
      // check to see if we need to set phone off icon
      if ( (app_timeago_diff >= PHONEOUT_WAIT_MIN) || ( (strcmp(app_label_buffer, "") != 0) && (strcmp(app_label_buffer, "m") != 0) ) ) {
	    // set phone off icon
        create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]); 
              
        // erase cgm ago times and cgm icon
        text_layer_set_text(cgmtime_layer, "");
        create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, CHECK IF HAVE TO VIBRATE");
		// Vibrate if we need to
		if (!PhoneOffAlert) {
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, READ APP TIMEAGO: VIBRATE");
		  alert_handler_cgm(PHONEOUT_VIBE);
		  PhoneOffAlert = true;
		}
	  }
	  else {
	    // reset PhoneOffAlert
        PhoneOffAlert = false;
      }		
    } // else init code 
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, FUNCTION OUT");
} // end load_apptime

static void load_bg_delta() {
	//APP_LOG(APP_LOG_LEVEL_INFO, "BG DELTA FUNCTION START");
	
	// CONSTANTS
	const uint8_t MSGLAYER_BUFFER_SIZE = 14;
	const uint8_t BGDELTA_LABEL_SIZE = 14;
	const uint8_t BGDELTA_FORMATTED_SIZE = 14;
	
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	static char delta_label_buffer[14];
	static char formatted_bg_delta[14];
	
	// CODE START
	
	// check bluetooth connection
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
    
    if (!bluetooth_connected_cgm) {
	  // Bluetooth is out; BT message already set, so return
	  return;
    }
    
	// check for CHECK PHONE condition, if true set message
	if ((PhoneOffAlert) && (!TurnOff_CHECKPHONE_Msg)) {
	  text_layer_set_text(message_layer, "CHECK PHONE");
      return;	
	}
	
	// check for CHECK CGM condition, if true set message
	if ((CGMOffAlert) && (!TurnOff_CHECKCGM_Msg)) {
	  text_layer_set_text(message_layer, "CHECK CGM");
    return;	
	}
	
	// check for special messages; if no string, set no message
	if (strcmp(current_bg_delta, "") == 0) {
      strncpy(formatted_bg_delta, "", MSGLAYER_BUFFER_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      return;	
    }
	
  	// check for NO ENDPOINT condition, if true set message
	// put " " (space) in bg field so logo continues to show
	if (strcmp(current_bg_delta, "NOEP") == 0) {
      strncpy(formatted_bg_delta, "NO ENDPOINT", MSGLAYER_BUFFER_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      text_layer_set_text(bg_layer, " ");
	  create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGO_SPECVALUE_ICON_INDX]);
      specvalue_alert = false;
      return;	
	}

  	// check for DATA OFFLINE condition, if true set message to fix condition	
    if (strcmp(current_bg_delta, "ERR") == 0) {
      strncpy(formatted_bg_delta, "RSTRT PHONE", MSGLAYER_BUFFER_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      text_layer_set_text(bg_layer, "");
      return;	
    }
  
  	// check if LOADING.., if true set message
	// put " " (space) in bg field so logo continues to show
    if (strcmp(current_bg_delta, "LOAD") == 0) {
      strncpy(formatted_bg_delta, "LOADING...", MSGLAYER_BUFFER_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      text_layer_set_text(bg_layer, " ");
      create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGO_SPECVALUE_ICON_INDX]);
      specvalue_alert = false;
      return;	
    }
 
	// check for zero delta here; if get later then we know we have an error instead
    if (strcmp(current_bg_delta, "0") == 0) {
      strncpy(formatted_bg_delta, "0", BGDELTA_FORMATTED_SIZE);
      strncpy(delta_label_buffer, " mg/dL", BGDELTA_LABEL_SIZE);
	  strcat(formatted_bg_delta, delta_label_buffer);
      text_layer_set_text(message_layer, formatted_bg_delta);
      return;	
    }
  
    if (strcmp(current_bg_delta, "0.0") == 0) {
      strncpy(formatted_bg_delta, "0.0", BGDELTA_FORMATTED_SIZE);
      strncpy(delta_label_buffer, " mmol", BGDELTA_LABEL_SIZE);
	  strcat(formatted_bg_delta, delta_label_buffer);
      text_layer_set_text(message_layer, formatted_bg_delta);
      return;	
    }
  
	// check to see if we have MG/DL or MMOL
	// get currentBG_isMMOL in myBGAtoi
	converted_bgDelta = myBGAtoi(current_bg_delta);
 
	// Bluetooth is good, Phone is good, CGM connection is good, no special message 
	// set delta BG message
	
	// zero here, means we have an error instead; set error message
    if (converted_bgDelta == 0) {
      strncpy(formatted_bg_delta, "BG DELTA ERR", BGDELTA_FORMATTED_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      return;
    }
  
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG DELTA, DELTA STRING: %s", &current_bg_delta[i]);
    if (!currentBG_isMMOL) {
	  // set mg/dL string
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG DELTA: FOUND MG/DL, SET STRING");
	    if (converted_bgDelta >= 100) {
		  // bg delta too big, set zero instead
	      strncpy(formatted_bg_delta, "0", BGDELTA_FORMATTED_SIZE);
	    }
	    else {
	      strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
	    }
	    strncpy(delta_label_buffer, " mg/dL", BGDELTA_LABEL_SIZE);
	    strcat(formatted_bg_delta, delta_label_buffer);
	}
	else {
	  // set mmol string
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG DELTA: FOUND MMOL, SET STRING");
	  if (converted_bgDelta >= 55) {
	    // bg delta too big, set zero instead
	    strncpy(formatted_bg_delta, "0.0", BGDELTA_FORMATTED_SIZE);
	  }
	  else {
	    strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
	  }
	  strncpy(delta_label_buffer, " mmol", BGDELTA_LABEL_SIZE);
	  strcat(formatted_bg_delta, delta_label_buffer);
	}
	
	text_layer_set_text(message_layer, formatted_bg_delta);
	
} // end load_bg_delta

static void load_battlevel() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, FUNCTION START");

	// CONSTANTS
	const uint8_t BATTLEVEL_FORMATTED_SIZE = 6;
	
	// ARRAY OF BATTERY LEVEL ICONS
	const uint32_t BATTLEVEL_ICONS[] = {
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
    
	// INDEX FOR ARRAY OF BATTERY LEVEL ICONS
	const uint8_t BATTEMPTY_ICON_INDX = 0;
	const uint8_t BATT10_ICON_INDX = 1;
	const uint8_t BATT20_ICON_INDX = 2;
	const uint8_t BATT30_ICON_INDX = 3;
	const uint8_t BATT40_ICON_INDX = 4;
	const uint8_t BATT50_ICON_INDX = 5;
	const uint8_t BATT60_ICON_INDX = 6;
	const uint8_t BATT70_ICON_INDX = 7;
	const uint8_t BATT80_ICON_INDX = 8;
	const uint8_t BATT90_ICON_INDX = 9;
	const uint8_t BATTFULL_ICON_INDX = 10;
	const uint8_t BATTNONE_ICON_INDX = 11;
	
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	int current_battlevel = 0;
	static char battlevel_percent[6];
	
	// CODE START
	
	// initialize inverter layer to hide
	layer_set_hidden((Layer *)inv_battlevel_layer, true);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, LAST BATTLEVEL: %s", last_battlevel);
  
	if (strcmp(last_battlevel, " ") == 0) {
      // Init code or no battery, can't do battery; set text layer & icon to empty value 
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, NO BATTERY");
      text_layer_set_text(battlevel_layer, "");
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATTNONE_ICON_INDX]); 
      LowBatteryAlert = false;	  
      return;
    }
  
	if (strcmp(last_battlevel, "0") == 0) {
      // Zero battery level; set here, so if we get zero later we know we have an error instead
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, SET STRING");
      text_layer_set_text(battlevel_layer, "0%");
      layer_set_hidden((Layer *)inv_battlevel_layer, false);
      if (!LowBatteryAlert) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, VIBRATE");
		alert_handler_cgm(LOWBATTERY_VIBE);
		LowBatteryAlert = true;
      }	  
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, SET ICON");
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATTEMPTY_ICON_INDX]);
      return;
    }
  
	current_battlevel = myAtoi(last_battlevel);
  
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, CURRENT BATTLEVEL: %i", current_battlevel);
  
	if ((current_battlevel <= 0) || (current_battlevel > 100) || (last_battlevel[0] == '-')) { 
      // got a negative or out of bounds or error battery level
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, UNKNOWN, ERROR BATTERY");
	  text_layer_set_text(battlevel_layer, "ERR");
	  layer_set_hidden((Layer *)inv_battlevel_layer, false);
	  create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATTNONE_ICON_INDX]);
    return;
	}
      
    // get current battery level and set battery level text with percent
    snprintf(battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "%i%%", current_battlevel);
    text_layer_set_text(battlevel_layer, battlevel_percent);

    // check battery level, set battery level icon
    if ( (current_battlevel > 90) && (current_battlevel <= 100) ) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATTFULL_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 80) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT90_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 70) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT80_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 60) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT70_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 50) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT60_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 40) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT50_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 30) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT40_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 20) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT30_ICON_INDX]);
	  LowBatteryAlert = false;
    }
    else if (current_battlevel > 10) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT20_ICON_INDX]);
      layer_set_hidden((Layer *)inv_battlevel_layer, false);
      if (!LowBatteryAlert) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 20 OR LESS, VIBRATE");
		alert_handler_cgm(LOWBATTERY_VIBE);
		LowBatteryAlert = true;
      }
    }
    else if (current_battlevel > 5) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATT10_ICON_INDX]);
      layer_set_hidden((Layer *)inv_battlevel_layer, false);
      if (!LowBatteryAlert) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 10 OR LESS, VIBRATE");
		alert_handler_cgm(LOWBATTERY_VIBE);
		LowBatteryAlert = true;
      }
    }
    else if ( (current_battlevel > 0) && (current_battlevel <= 5) ) {
      create_update_bitmap(&batticon_bitmap,batticon_layer,BATTLEVEL_ICONS[BATTEMPTY_ICON_INDX]);
      layer_set_hidden((Layer *)inv_battlevel_layer, false);
      if (!LowBatteryAlert) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 5 OR LESS, VIBRATE");
		alert_handler_cgm(LOWBATTERY_VIBE);
		LowBatteryAlert = true;
      }	  
    }
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, END FUNCTION");
} // end load_battlevel

void sync_tuple_changed_callback_cgm(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");
	
	// CONSTANTS
	const uint8_t ICON_MSGSTR_SIZE = 4;
	const uint8_t BG_MSGSTR_SIZE = 6;
	const uint8_t BGDELTA_MSGSTR_SIZE = 6;
	const uint8_t BATTLEVEL_MSGSTR_SIZE = 4;

	// CODE START
	
	switch (key) {

	case CGM_ICON_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: ICON ARROW");
      strncpy(current_icon, new_tuple->value->cstring, ICON_MSGSTR_SIZE);
	  load_icon();
      break; // break for CGM_ICON_KEY

	case CGM_BG_KEY:;
	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG CURRENT");
      strncpy(last_bg, new_tuple->value->cstring, BG_MSGSTR_SIZE);
      load_bg();
      break; // break for CGM_BG_KEY

	case CGM_TCGM_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME");
	  current_cgm_time = new_tuple->value->uint32;
      load_cgmtime();
      break; // break for CGM_TCGM_KEY

	case CGM_TAPP_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ APP TIME NOW");
	  current_app_time = new_tuple->value->uint32;
      load_apptime();    
      break; // break for CGM_TAPP_KEY

	case CGM_DLTA_KEY:;
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG DELTA");
	  strncpy(current_bg_delta, new_tuple->value->cstring, BGDELTA_MSGSTR_SIZE);
	  load_bg_delta();
	  break; // break for CGM_DLTA_KEY
	
	case CGM_UBAT_KEY:;
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: UPLOADER BATTERY LEVEL");
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL IN, COPY LAST BATTLEVEL");
      strncpy(last_battlevel, new_tuple->value->cstring, BATTLEVEL_MSGSTR_SIZE);
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL, CALL LOAD BATTLEVEL");
      load_battlevel();
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL OUT");
      break; // break for CGM_UBAT_KEY

	case CGM_NAME_KEY:
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: T1D NAME");
      text_layer_set_text(t1dname_layer, new_tuple->value->cstring);
      break; // break for CGM_NAME_KEY
	  
  }  // end switch(key)

} // end sync_tuple_changed_callback_cgm()

static void send_cmd_cgm(void) {
  
  DictionaryIterator *iter = NULL;
  AppMessageResult sendcmd_openerr = APP_MSG_OK;
  AppMessageResult sendcmd_senderr = APP_MSG_OK;
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD IN, ABOUT TO OPEN APP MSG OUTBOX");
  sendcmd_openerr = app_message_outbox_begin(&iter);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, CHECK FOR ERROR");
  if (sendcmd_openerr != APP_MSG_OK) {
     APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD OPEN ERROR");
     APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD OPEN ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));

    return;
  }

  //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, NO ERROR, ABOUT TO SEND MSG TO APP");
  sendcmd_senderr = app_message_outbox_send();
  
  if (sendcmd_senderr != APP_MSG_OK) {
     APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD SEND ERROR");
     APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD SEND ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
  }

  //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD OUT, SENT MSG TO APP");
  
} // end send_cmd_cgm

void timer_callback_cgm(void *data) {

  //APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK IN, TIMER POP, ABOUT TO CALL SEND CMD");
  // reset msg timer to NULL
  if (timer_cgm != NULL) {
    timer_cgm = NULL;
  }
  
  // send message
  send_cmd_cgm();
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, SEND CMD DONE, ABOUT TO REGISTER TIMER");
  // set msg timer
  timer_cgm = app_timer_register((WATCH_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);

  //APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, REGISTER TIMER DONE");
  
} // end timer_callback_cgm

// format current time from watch

void handle_minute_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm) {
  
  // VARIABLES
  size_t tick_return_cgm = 0;
  
  // CODE START
  
  if (units_changed_cgm & MINUTE_UNIT) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME MINUTE CODE");
    tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", tick_time_cgm);
	if (tick_return_cgm != 0) {
      text_layer_set_text(time_watch_layer, time_watch_text);
	}
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
	// increment BG snooze
    ++lastAlertTime;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
	
  } 
  else if (units_changed_cgm & DAY_UNIT) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME DAY CODE");
    tick_return_cgm = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d", tick_time_cgm);
	if (tick_return_cgm != 0) {
      text_layer_set_text(date_app_layer, date_app_text);
	}
  }
  
} // end handle_minute_tick_cgm

void window_load_cgm(Window *window_cgm) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");
  
  // VARIABLES
  Layer *window_layer_cgm = NULL;
  
  // CODE START

  window_layer_cgm = window_get_root_layer(window_cgm);
  
  // DELTA BG
  message_layer = text_layer_create(GRect(0, 33, 144, 55));
  text_layer_set_text_color(message_layer, GColorBlack);
  text_layer_set_background_color(message_layer, GColorWhite);
  text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));

  // ARROW OR SPECIAL VALUE
  icon_layer = bitmap_layer_create(GRect(85, -7, 78, 50));
  bitmap_layer_set_alignment(icon_layer, GAlignTopLeft);
  bitmap_layer_set_background_color(icon_layer, GColorWhite);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(icon_layer));

  // APP TIME AGO ICON
  appicon_layer = bitmap_layer_create(GRect(118, 63, 40, 24));
  bitmap_layer_set_alignment(appicon_layer, GAlignLeft);
  bitmap_layer_set_background_color(appicon_layer, GColorWhite);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(appicon_layer));  

  // APP TIME AGO READING
  time_app_layer = text_layer_create(GRect(77, 58, 40, 24));
  text_layer_set_text_color(time_app_layer, GColorBlack);
  text_layer_set_background_color(time_app_layer, GColorClear);
  text_layer_set_font(time_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(time_app_layer, GTextAlignmentRight);
  layer_add_child(window_layer_cgm, text_layer_get_layer(time_app_layer));
  
  // BG
  bg_layer = text_layer_create(GRect(0, -5, 95, 47));
  text_layer_set_text_color(bg_layer, GColorBlack);
  text_layer_set_background_color(bg_layer, GColorWhite);
  text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(bg_layer));

  // CGM TIME AGO ICON
  cgmicon_layer = bitmap_layer_create(GRect(5, 63, 40, 24));
  bitmap_layer_set_alignment(cgmicon_layer, GAlignLeft);
  bitmap_layer_set_background_color(cgmicon_layer, GColorWhite);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(cgmicon_layer));  
  
  // CGM TIME AGO READING
  cgmtime_layer = text_layer_create(GRect(28, 58, 40, 24));
  text_layer_set_text_color(cgmtime_layer, GColorBlack);
  text_layer_set_background_color(cgmtime_layer, GColorClear);
  text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(cgmtime_layer));

  // T1D NAME
  t1dname_layer = text_layer_create(GRect(5, 138, 69, 28));
  text_layer_set_text_color(t1dname_layer, GColorWhite);
  text_layer_set_background_color(t1dname_layer, GColorClear);
  text_layer_set_font(t1dname_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(t1dname_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(t1dname_layer));

  // BATTERY LEVEL ICON
  batticon_layer = bitmap_layer_create(GRect(80, 147, 28, 20));
  bitmap_layer_set_alignment(batticon_layer, GAlignLeft);
  bitmap_layer_set_background_color(batticon_layer, GColorBlack);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(batticon_layer));

  // BATTERY LEVEL
  battlevel_layer = text_layer_create(GRect(110, 144, 38, 20));
  text_layer_set_text_color(battlevel_layer, GColorWhite);
  text_layer_set_background_color(battlevel_layer, GColorBlack);
  text_layer_set_font(battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(battlevel_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(battlevel_layer));
  
  // INVERTER BATTERY LAYER
  inv_battlevel_layer = inverter_layer_create(GRect(110, 149, 38, 15));
  layer_add_child(window_get_root_layer(window_cgm), inverter_layer_get_layer(inv_battlevel_layer));

  // CURRENT ACTUAL TIME FROM WATCH
  time_watch_layer = text_layer_create(GRect(0, 82, 144, 44));
  text_layer_set_text_color(time_watch_layer, GColorWhite);
  text_layer_set_background_color(time_watch_layer, GColorClear);
  text_layer_set_font(time_watch_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(time_watch_layer));
  
  // CURRENT ACTUAL DATE FROM APP
  date_app_layer = text_layer_create(GRect(0, 120, 144, 25));
  text_layer_set_text_color(date_app_layer, GColorWhite);
  text_layer_set_background_color(date_app_layer, GColorClear);
  text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
  draw_date_from_app();
  layer_add_child(window_layer_cgm, text_layer_get_layer(date_app_layer));
  
  // put " " (space) in bg field so logo continues to show
  // " " (space) also shows these are init values, not bad or null values
  Tuplet initial_values_cgm[] = {
    TupletCString(CGM_ICON_KEY, " "),
	TupletCString(CGM_BG_KEY, " "),
	TupletInteger(CGM_TCGM_KEY, 0),
	TupletInteger(CGM_TAPP_KEY, 0),
	TupletCString(CGM_DLTA_KEY, "LOAD"),
	TupletCString(CGM_UBAT_KEY, " "),
	TupletCString(CGM_NAME_KEY, " ")
  };
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, ABOUT TO CALL APP SYNC INIT");
  app_sync_init(&sync_cgm, sync_buffer_cgm, sizeof(sync_buffer_cgm), initial_values_cgm, ARRAY_LENGTH(initial_values_cgm), sync_tuple_changed_callback_cgm, sync_error_callback_cgm, NULL);
  
  // init timer to null if needed, and register timer
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, APP INIT DONE, ABOUT TO REGISTER TIMER");  
  if (timer_cgm != NULL) {
    timer_cgm = NULL;
  }
  timer_cgm = app_timer_register((LOADING_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, TIMER REGISTER DONE");
  
} // end window_load_cgm

void window_unload_cgm(Window *window_cgm) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD IN");
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, APP SYNC DEINIT");
  app_sync_deinit(&sync_cgm);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY GBITMAPS IF EXIST");
  destroy_null_GBitmap(&icon_bitmap);
  destroy_null_GBitmap(&appicon_bitmap);
  destroy_null_GBitmap(&cgmicon_bitmap);
  destroy_null_GBitmap(&specialvalue_bitmap);
  destroy_null_GBitmap(&batticon_bitmap);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY BITMAPS IF EXIST");  
  destroy_null_BitmapLayer(&icon_layer);
  destroy_null_BitmapLayer(&cgmicon_layer);
  destroy_null_BitmapLayer(&appicon_layer);
  destroy_null_BitmapLayer(&batticon_layer);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY TEXT LAYERS IF EXIST");  
  destroy_null_TextLayer(&bg_layer);
  destroy_null_TextLayer(&cgmtime_layer);
  destroy_null_TextLayer(&message_layer);
  destroy_null_TextLayer(&battlevel_layer);
  destroy_null_TextLayer(&t1dname_layer);
  destroy_null_TextLayer(&time_watch_layer);
  destroy_null_TextLayer(&time_app_layer);
  destroy_null_TextLayer(&date_app_layer);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY INVERTER LAYERS IF EXIST");  
  destroy_null_InverterLayer(&inv_battlevel_layer);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD OUT");
} // end window_unload_cgm

static void init_cgm(void) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");
  
  // subscribe to the tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);

  // subscribe to the bluetooth connection service
  bluetooth_connection_service_subscribe(handle_bluetooth_cgm);
  
  // init the window pointer to NULL if it needs it
  if (window_cgm != NULL) {
    window_cgm = NULL;
  }
  
  // create the windows
  window_cgm = window_create();
  window_set_background_color(window_cgm, GColorBlack);
  window_set_fullscreen(window_cgm, true);
  window_set_window_handlers(window_cgm, (WindowHandlers) {
    .load = window_load_cgm,
	.unload = window_unload_cgm  
  });

  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS"); 
  app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
  app_message_register_outbox_failed(outbox_failed_handler_cgm);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, ABOUT TO CALL APP MSG OPEN"); 
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, APP MSG OPEN DONE");
  
  const bool animated_cgm = true;
  window_stack_push(window_cgm, animated_cgm);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE OUT"); 
}  // end init_cgm

static void deinit_cgm(void) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE IN");
  
  // unsubscribe to the tick timer service
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE TICK TIMER");
  tick_timer_service_unsubscribe();

  // unsubscribe to the bluetooth connection service
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE BLUETOOTH");
  bluetooth_connection_service_unsubscribe();
  
  // cancel timers if they exist
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL APP TIMER");
  if (timer_cgm != NULL) {
    app_timer_cancel(timer_cgm);
    timer_cgm = NULL;
  }
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL BLUETOOTH TIMER");
  if (BT_timer != NULL) {
    app_timer_cancel(BT_timer);
    BT_timer = NULL;
  }
  
  // destroy the window if it exists
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR DESTROY");
  if (window_cgm != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, DESTROY");
    window_destroy(window_cgm);
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR NULL");
  if (window_cgm != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, SET TO NULL");
    window_cgm = NULL;
  }
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE OUT");
} // end deinit_cgm

int main(void) {

  init_cgm();

  app_event_loop();
  deinit_cgm();
  
} // end main

