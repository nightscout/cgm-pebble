#include "pebble.h"
#include "stddef.h"
#include "string.h"

// Global constants
#define NO_NOISE 0
#define CLEAN_NOISE 1
#define LIGHT_NOISE 2
#define MEDIUM_NOISE 3
#define HEAVY_NOISE 4
#define WARMUP_NOISE 5
#define OTHER_NOISE 6
  
// global window variables
// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

Window *window_cgm = NULL;

TextLayer *tophalf_layer = NULL;
TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *message_layer = NULL;    // BG DELTA & MESSAGE LAYER
TextLayer *rig_battlevel_layer = NULL;
TextLayer *watch_battlevel_layer = NULL;
TextLayer *t1dname_layer = NULL;
TextLayer *time_watch_layer = NULL;
TextLayer *date_app_layer = NULL;
TextLayer *happymsg_layer = NULL;
TextLayer *raw_calc_layer = NULL;
TextLayer *raw_unfilt_layer = NULL;
TextLayer *noise_layer = NULL;
TextLayer *calcraw_last1_layer = NULL;
TextLayer *calcraw_last2_layer = NULL;
TextLayer *calcraw_last3_layer = NULL;

BitmapLayer *icon_layer = NULL;
BitmapLayer *cgmicon_layer = NULL;
BitmapLayer *perfectbg_layer = NULL;

GBitmap *icon_bitmap = NULL;
GBitmap *cgmicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *perfectbg_bitmap = NULL;

InverterLayer *inv_rig_battlevel_layer = NULL;

PropertyAnimation *perfectbg_animation = NULL;
PropertyAnimation *happymsg_animation = NULL;

static char time_watch_text[] = "00:00";
static char date_app_text[] = "Wed 13 ";

// variables for AppSync
AppSync sync_cgm;
// CGM message is 110 bytes
// Pebble needs additional 62 Bytes?!? Pad with additional 60 bytes
static uint8_t sync_buffer_cgm[212];

// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
time_t cgm_time_now = 0;
time_t app_time_now = 0;
int timeformat = 0;

// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;

// global variables for sync tuple functions
char current_icon[2] = {0};
char last_bg[6] = {0};
char last_battlevel[4] = {0};
char battlevel_percent[6] = {0};
uint32_t current_cgm_time = 0;
char formatted_cgm_timeago[10] = {0};
char cgm_label_buffer[6] = {0};	
uint32_t current_app_time = 0;
char current_bg_delta[10] = {0};
char delta_label_buffer[14] = {0};
char formatted_bg_delta[14] = {0};
char last_calc_raw[6] = {0};
char last_raw_unfilt[6] = {0};
uint8_t current_noise_value = 0;
char formatted_noise[8] = {0};
char last_calc_raw1[6] = {0};
char last_calc_raw2[6] = {0};
char last_calc_raw3[6] = {0};
char formatted_battlevel[10] = {0};

int current_bg = 0;
int current_calc_raw = 0;
int current_calc_raw1 = 0;
bool currentBG_isMMOL = false;
int converted_bgDelta = 0;
char current_values[25] = {0};
bool HaveCalcRaw = false;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static bool specvalue_alert = false;

// global retries counters for timeout problems
static uint8_t appsyncandmsg_retries_counter = 0;
static uint8_t dataoffline_retries_counter = 0;

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
static bool DataOfflineAlert = false;

// global constants for time durations; seconds
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
static const uint8_t HAPPYMSG_BUFFER_SIZE = 30;
static const uint8_t BG_BUFFER_SIZE = 6;
static const uint8_t BATTLEVEL_FORMAT_SIZE = 12;

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
uint16_t HYPOLOW_BG_MGDL = 55;
uint16_t BIGLOW_BG_MGDL = 60;
uint16_t MIDLOW_BG_MGDL = 70;
uint16_t LOW_BG_MGDL = 80;

uint16_t HIGH_BG_MGDL = 180;
uint16_t MIDHIGH_BG_MGDL = 240;
uint16_t BIGHIGH_BG_MGDL = 300;
static const uint16_t SHOWHIGH_BG_MGDL = 400;

// BG Ranges, MMOL
// VALUES ARE IN INT, NOT FLOATING POINT, LAST DIGIT IS DECIMAL
// FOR EXAMPLE : SPECVALUE IS 1.1, BIGHIGH IS 16.6
// ALWAYS USE ONE AND ONLY ONE DECIMAL POINT FOR LAST DIGIT
// GOOD : 5.0, 12.2 // BAD : 7 , 14.44
static const uint16_t SPECVALUE_BG_MMOL = 11;
static const uint16_t SHOWLOW_BG_MMOL = 23;
uint16_t HYPOLOW_BG_MMOL = 30;
uint16_t BIGLOW_BG_MMOL = 33;
uint16_t MIDLOW_BG_MMOL = 39;
uint16_t LOW_BG_MMOL = 44;

uint16_t HIGH_BG_MMOL = 100;
uint16_t MIDHIGH_BG_MMOL = 133;
uint16_t BIGHIGH_BG_MMOL = 166;
static const uint16_t SHOWHIGH_BG_MMOL = 222;

// BG Snooze Times, in Minutes; controls when vibrate again
// RANGE 0-240
static const uint8_t SPECVALUE_SNZ_MIN = 30;
static const uint8_t HYPOLOW_SNZ_MIN = 5;
static const uint8_t BIGLOW_SNZ_MIN = 5;
static const uint8_t MIDLOW_SNZ_MIN = 10;
uint8_t LOW_SNZ_MIN = 15;
uint8_t HIGH_SNZ_MIN = 30;
static const uint8_t MIDHIGH_SNZ_MIN = 30;
static const uint8_t BIGHIGH_SNZ_MIN = 30;
  
// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
static const uint8_t SPECVALUE_VIBE = 2;
static const uint8_t HYPOLOWBG_VIBE = 3;
static const uint8_t BIGLOWBG_VIBE = 3;
uint8_t LOWBG_VIBE = 3;
uint8_t HIGHBG_VIBE = 2;
static const uint8_t BIGHIGHBG_VIBE = 2;
static const uint8_t DOUBLEDOWN_VIBE = 3;
static const uint8_t APPSYNC_ERR_VIBE = 1;
static const uint8_t APPMSG_INDROP_VIBE = 1;
static const uint8_t APPMSG_OUTFAIL_VIBE = 1;
static const uint8_t BTOUT_VIBE = 1;
static const uint8_t CGMOUT_VIBE = 1;
static const uint8_t PHONEOUT_VIBE = 1;
static const uint8_t LOWBATTERY_VIBE = 1;
static const uint8_t DATAOFFLINE_VIBE = 1;

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
// SPECIAL BOOL TO HARD CODE VIBRATIONS OFF; If you want no vibrations, SET TO true
// Use for Sleep Face or anyone else for a custom load
bool HardCodeNoVibrations = false;

// Control Animations
// SPECIAL BOOL TO HARD CODE ANIMATIONS OFF; If you want no animations, SET TO true
// SPECIAL BOOL TO HARD CODE ANIMATIONS ALL ON; If you want all animations, SET TO true
// This is for people who want old ones too
// Use for a custom load
bool HardCodeNoAnimations = false;
bool HardCodeAllAnimations = false;

// Control Vibrations for Config File
// IF YOU WANT NO VIBRATIONS, SET TO true
bool TurnOffAllVibrations = false;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO true
bool TurnOffStrongVibrations = false;

// Control Raw data
// If you want to turn off vibrations for calculated raw, set to true
bool TurnOffVibrationsCalcRaw = false;
// If you want to see unfiltered raw, set to true
bool TurnOnUnfilteredRaw = false;

// ** END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WIL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
static const uint8_t BT_ALERT_WAIT_SECS = 45;

// Message Timer & Animate Wait Times, in Seconds
static const uint8_t WATCH_MSGSEND_SECS = 60;
static const uint8_t LOADING_MSGSEND_SECS = 6;
static const uint8_t PERFECTBG_ANIMATE_SECS = 10;
static const uint8_t HAPPYMSG_ANIMATE_SECS = 10;

// App Sync / Message retries, for timeout / busy problems
// Change to see if there is a temp or long term problem
// This is approximately number of seconds, so if set to 50, timeout is at 50 seconds
// However, this can vary widely - can be up to 6 seconds .... for 50, timeout can be up to 3 minutes
static const uint8_t APPSYNCANDMSG_RETRIES_MAX = 50;

// HTML Request retries, for timeout / busy problems
// Change to see if there is a temp or long term problem
// This is number of minutes, so if set to 11 timeout is at 11 minutes
static const uint8_t DATAOFFLINE_RETRIES_MAX = 14;

enum CgmKey {
	CGM_ICON_KEY = 0x0,		// TUPLE_CSTRING, MAX 2 BYTES (10)
	CGM_BG_KEY = 0x1,		// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
	CGM_TCGM_KEY = 0x2,		// TUPLE_INT, 4 BYTES (CGM TIME)
	CGM_TAPP_KEY = 0x3,		// TUPLE_INT, 4 BYTES (APP / PHONE TIME)
	CGM_DLTA_KEY = 0x4,		// TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
	CGM_UBAT_KEY = 0x5,		// TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
	CGM_NAME_KEY = 0x6,		// TUPLE_CSTRING, MAX 9 BYTES (Christine)
	CGM_VALS_KEY = 0x7,   // TUPLE_CSTRING, MAX 25 BYTES (0,000,000,000,000,0,0,0,0)
	CGM_CLRW_KEY = 0x8,   // TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
	CGM_RWUF_KEY = 0x9,   // TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
	CGM_NOIZ_KEY = 0xA    // TUPLE_INT, 4 BYTES (1-4)
}; 
// TOTAL MESSAGE DATA 4x6+2+5+3+9+25 = 68 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x10+2 = 42 BYTES
// TOTAL MESSAGE 110 BYTES

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] = {
	RESOURCE_ID_IMAGE_SPECVALUE_NONE,   //0
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,   //1
	RESOURCE_ID_IMAGE_BLOOD_DROP,       //2
	RESOURCE_ID_IMAGE_STOP_LIGHT,       //3
	RESOURCE_ID_IMAGE_HOURGLASS,        //4
	RESOURCE_ID_IMAGE_QUESTION_MARKS,   //5
	RESOURCE_ID_IMAGE_LOGO              //6
};
	
// INDEX FOR ARRAY OF SPECIAL VALUE ICONS
static const uint8_t NONE_SPECVALUE_ICON_INDX = 0;
static const uint8_t BROKEN_ANTENNA_ICON_INDX = 1;
static const uint8_t BLOOD_DROP_ICON_INDX = 2;
static const uint8_t STOP_LIGHT_ICON_INDX = 3;
static const uint8_t HOURGLASS_ICON_INDX = 4;
static const uint8_t QUESTION_MARKS_ICON_INDX = 5;
static const uint8_t LOGO_SPECVALUE_ICON_INDX = 6;

// ARRAY OF TIMEAGO ICONS
static const uint8_t TIMEAGO_ICONS[] = {
	RESOURCE_ID_IMAGE_RCVRNONE,   //0
	RESOURCE_ID_IMAGE_RCVRON,     //1
	RESOURCE_ID_IMAGE_RCVROFF     //2
};

// INDEX FOR ARRAY OF TIMEAGO ICONS
static const uint8_t RCVRNONE_ICON_INDX = 0;
static const uint8_t RCVRON_ICON_INDX = 1;
static const uint8_t RCVROFF_ICON_INDX = 2;

// ARRAY OF ICONS FOR PERFECT BG
static const uint8_t PERFECTBG_ICONS[] = {
	RESOURCE_ID_IMAGE_CLUB100,         //0
	RESOURCE_ID_IMAGE_CLUB55           //1
};

// INDEX FOR ARRAY OF PERFECT BG ICONS
static const uint8_t CLUB100_ICON_INDX = 0;
static const uint8_t CLUB55_ICON_INDX = 1;

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

char *strtok(s, delim)
	register char *s;
	register const char *delim;
{
	register char *spanp;
	register int c, sc;
	char *tok;
	static char *last;


	if (s == NULL && (s = last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
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

static void load_values(){
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Loaded Values: %s", current_values);

  int num_a_items = 0;
  char *o;
  int mgormm = 0;
  int vibes = 0;
  int rawvibrate = 0;
  if (current_values == NULL) {
    return;
  } else {
    o = strtok(current_values,",");
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "mg or mm: %s", o);      
    mgormm = myAtoi(o);

    while(o != NULL) {
      num_a_items++;
      switch (num_a_items) {
        case 2:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowbg: %s", o);
          if (mgormm == 0){
            LOW_BG_MGDL = myAtoi(o);
            if (LOW_BG_MGDL < 60) {
            	MIDLOW_BG_MGDL = 55;
            	BIGLOW_BG_MGDL = 50;
            	HYPOLOW_BG_MGDL = 45;
            } else if (LOW_BG_MGDL < 70) {
            	MIDLOW_BG_MGDL = 60;
            	BIGLOW_BG_MGDL = 55;
            	HYPOLOW_BG_MGDL = 50;
            }
          } else {
            LOW_BG_MMOL = myAtoi(o);
            if (LOW_BG_MMOL < 33) {
            	MIDLOW_BG_MMOL = 31;
            	BIGLOW_BG_MMOL = 28;
            	HYPOLOW_BG_MMOL = 25;
            } else if (LOW_BG_MMOL < 39) {
            	MIDLOW_BG_MMOL = 33;
            	BIGLOW_BG_MMOL = 31;
            	HYPOLOW_BG_MMOL = 28;
            }
          }
          break;
        case 3:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "highbg: %s", o);
          if (mgormm == 0){
            HIGH_BG_MGDL = myAtoi(o);
            if (HIGH_BG_MGDL > 239) {
              MIDHIGH_BG_MGDL = 300;
              BIGHIGH_BG_MGDL = 350;
            }
          } else {
            HIGH_BG_MMOL = myAtoi(o);
              if (HIGH_BG_MMOL > 132) {
               MIDHIGH_BG_MMOL = 166;
               BIGHIGH_BG_MMOL =  200;
              }
          }
          break;
        case 4:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowsnooze: %s", o);
          LOW_SNZ_MIN = myAtoi(o);
          break;
        case 5:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "highsnooze: %s", o);      
          HIGH_SNZ_MIN = myAtoi(o);
          break;
        case 6:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowvibe: %s", o);
          LOWBG_VIBE = myAtoi(o);
          break;
        case 7:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "highvibe: %s", o);
          HIGHBG_VIBE = myAtoi(o);
          break;
        case 8:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "vibepattern: %s", o);
          vibes = myAtoi(o);
          if (vibes == 0){
            TurnOffAllVibrations = true;
            TurnOffStrongVibrations = true;
          } else if (vibes == 1){
            TurnOffAllVibrations = false;
            TurnOffStrongVibrations = true; 
          } else if (vibes == 2){
            TurnOffAllVibrations = false;
            TurnOffStrongVibrations = false;
          }
          break;
        case 9:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "timeformat: %s", o);
          timeformat = myAtoi(o);
          break;
        case 10:
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "rawvibrate: %s", o);
          rawvibrate = myAtoi(o);
          if (rawvibrate == 0) { TurnOffVibrationsCalcRaw = true; }
          else { TurnOffVibrationsCalcRaw = false; }
          break;
      }
      o = strtok(NULL,",");
    }
  }
   
  
} //End load_values

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
	
	if ( (TurnOffAllVibrations) || (HardCodeNoVibrations) ) {
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
    
	// erase cgm icon
    create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);
	
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

void handle_watch_battery_cgm(BatteryChargeState watch_charge_state) {
  
  static char watch_battery_text[] = "Wch 100%";

  if (watch_charge_state.is_charging) {
    snprintf(watch_battery_text, BATTLEVEL_FORMAT_SIZE, "CHARGING");
  } else {
    snprintf(watch_battery_text, BATTLEVEL_FORMAT_SIZE, "Wch %d%%", watch_charge_state.charge_percent);
  }
  text_layer_set_text(watch_battlevel_layer, watch_battery_text);
  
} // end handle_watch_battery_cgm

static void draw_date_from_app() {
 
  // VARIABLES
  time_t d_app = time(NULL);
  struct tm *current_d_app = localtime(&d_app);
  size_t draw_return = 0;

  // CODE START
  
  // format current date from app
  if (strcmp(time_watch_text, "00:00") == 0) {
      //APP_LOG(APP_LOG_LEVEL_INFO, "TimeFormat: %d", timeformat);
      if (timeformat == 0){
        draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", current_d_app);
      } else {
        draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%H:%M", current_d_app);
      }
  
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

  bool bluetooth_connected_syncerror = true;
  
  // CODE START
  
  bluetooth_connected_syncerror = bluetooth_connection_service_peek();
  if (!bluetooth_connected_syncerror) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // increment app sync retries counter
  appsyncandmsg_retries_counter++;
  
  // if hit max counter, skip resend and flag user
  if (appsyncandmsg_retries_counter < APPSYNCANDMSG_RETRIES_MAX) {
  
    // APPSYNC ERROR debug logs
    APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
  
    // try to resend the message; open app message outbox
    appsync_err_openerr = app_message_outbox_begin(&iter); 
    if (appsync_err_openerr == APP_MSG_OK) {
      // could open app message outbox; send message
      appsync_err_senderr = app_message_outbox_send();
      if (appsync_err_senderr == APP_MSG_OK) {
        // everything OK, reset AppSyncErrAlert so no vibrate
        AppSyncErrAlert = false;
        // sent message OK; return
	      return;
      } // if appsync_err_senderr
    } // if appsync_err_openerr
  } // if appsyncandmsg_retries_counter
    
  // flag error
  if (appsyncandmsg_retries_counter > APPSYNCANDMSG_RETRIES_MAX) {
    APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC TOO MANY MESSAGES ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC RESEND ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC RESEND MSG OPEN ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC RESEND MSG SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppSyncErrAlert: %i  appsyncandmsg_retries_counter: %i", AppSyncErrAlert, appsyncandmsg_retries_counter);
    return;
  } 
 
  // check bluetooth again
  bluetooth_connected_syncerror = bluetooth_connection_service_peek();   
  if (!bluetooth_connected_syncerror) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // set message to RESTART WATCH -> PHONE
  text_layer_set_text(message_layer, "√APP/RESTRT");
  
  // reset appsync retries counter
  appsyncandmsg_retries_counter = 0;
  
  // erase cgm and app ago times
  text_layer_set_text(cgmtime_layer, "");
    
  // erase cgm icon
  create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);
    

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
  
  bool bluetooth_connected_inboxdrop = true;
  
	// CODE START

	bluetooth_connected_inboxdrop = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_inboxdrop) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // increment app sync retries counter
  appsyncandmsg_retries_counter++;
  
  // if hit max counter, skip resend and flag user
  if (appsyncandmsg_retries_counter < APPSYNCANDMSG_RETRIES_MAX) {
  
	  // APPMSG IN DROP debug logs
	  APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR");
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
    
    // try to resend the message; open app message outbox
    appmsg_indrop_openerr = app_message_outbox_begin(&iter); 
    if (appmsg_indrop_openerr == APP_MSG_OK) {
      // could open app message outbox; send message
      appmsg_indrop_senderr = app_message_outbox_send();
      if (appmsg_indrop_senderr == APP_MSG_OK ) {
        // everything OK, reset AppSyncErrAlert so no vibrate
        AppMsgInDropAlert = false;
        // sent message OK; return
	      return;
      } // if appmsg_indrop_senderr
    } // if appmsg_indrop_openerr
  } // if appsyncandmsg_retries_counter
  
  // check bluetooth again
  bluetooth_connected_inboxdrop = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_inboxdrop) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // flag error
  if (appsyncandmsg_retries_counter > APPSYNCANDMSG_RETRIES_MAX) {
    APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP TOO MANY MESSAGES ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR CODE ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP RESEND ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP RESEND MSG OPEN ERR CODE: %i RES: %s", appmsg_indrop_openerr, translate_app_error(appmsg_indrop_openerr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP RESEND MSG SEND ERR CODE: %i RES: %s", appmsg_indrop_senderr, translate_app_error(appmsg_indrop_senderr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgInDropAlert: %i  appsyncandmsg_retries_counter: %i", AppMsgInDropAlert, appsyncandmsg_retries_counter);
    return;
  } 

  // set message to RESTART WATCH -> PHONE
	text_layer_set_text(message_layer, "√APP/RESTRT");
  
  // reset appsync retries counter
  appsyncandmsg_retries_counter = 0; 
      
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
    
	// erase cgm icon
	create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);

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
  
  bool bluetooth_connected_outboxfail = true;
  
	// CODE START
	
	bluetooth_connected_outboxfail = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_outboxfail) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // increment app sync and msg retries counter
  appsyncandmsg_retries_counter++;
  
  // if hit max counter, skip resend and flag user
  if (appsyncandmsg_retries_counter < APPSYNCANDMSG_RETRIES_MAX) {
  
	  // APPMSG OUT FAIL debug logs
	  APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
    
    // try to resend the message; open app message outbox
    appmsg_outfail_openerr = app_message_outbox_begin(&iter);
    if (appmsg_outfail_openerr == APP_MSG_OK) {
      // could open app message outbox; send message
      appmsg_outfail_senderr = app_message_outbox_send();
      if (appmsg_outfail_senderr == APP_MSG_OK ) {
        // everything OK, reset AppSyncErrAlert so no vibrate
        AppMsgOutFailAlert = false;
        // sent message OK; return
	      return;
      } // if appmsg_outfail_senderr
    } // if appmsg_outfail_openerr
  } // if appsyncandmsg_retries_counter
  
  // check bluetooth again
  bluetooth_connected_outboxfail = bluetooth_connection_service_peek();
    
  if (!bluetooth_connected_outboxfail) {
    // bluetooth is out, BT message already set; return out
    return;
  }
  
  // flag error
  if (appsyncandmsg_retries_counter > APPSYNCANDMSG_RETRIES_MAX) {
	  APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "appsyncandmsg_retries_counter: %i", appsyncandmsg_retries_counter);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL RESEND ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL RESEND MSG OPEN ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL RESEND MSG SEND ERR CODE: %i RES: %s", appmsg_outfail_senderr, translate_app_error(appmsg_outfail_senderr));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgOutFailAlert: %i  appsyncandmsg_retries_counter: %i", AppMsgOutFailAlert, appsyncandmsg_retries_counter);
    return;
  } 

  // set message to RESTART WATCH -> PHONE
	text_layer_set_text(message_layer, "√APP/RESTRT");
  
  // reset appsync retries counter
  appsyncandmsg_retries_counter = 0; 
    
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
    
	// erase cgm icon
	create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);
  
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
	  RESOURCE_ID_IMAGE_SPECVALUE_NONE,  //0
	  RESOURCE_ID_IMAGE_UPUP,            //1
	  RESOURCE_ID_IMAGE_UP,              //2
	  RESOURCE_ID_IMAGE_UP45,            //3
	  RESOURCE_ID_IMAGE_FLAT,            //4
	  RESOURCE_ID_IMAGE_DOWN45,          //5
	  RESOURCE_ID_IMAGE_DOWN,            //6
	  RESOURCE_ID_IMAGE_DOWNDOWN,        //7
	  RESOURCE_ID_IMAGE_LOGO             //8
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
	
  // Got an icon value, check data offline condition, clear vibrate flag if needed
  if ( (strcmp(current_bg_delta, "OFF") < 0) || (strcmp(current_bg_delta, "OFF") > 0) ) {
    DataOfflineAlert = false;
    dataoffline_retries_counter = 0;
  }
  
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
		  // unexpected, set logo icon
	      create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[LOGO_ARROW_ICON_INDX]);
	    }
	    DoubleDownAlert = false;
	  }
	} // if specvalue_alert == false
	else { // this is just for log when need it
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, SPEC VALUE ALERT IS TRUE, DONE");
	} // else specvalue_alert == true
	
} // end load_icon

// forward declarations for animation code
static void load_bg_delta();
static void load_cgmtime();
static void load_apptime();
static void load_rig_battlevel();

// ANIMATION CODE

// PERFECTBG ANIMATION
void perfectbg_animation_started(Animation *animation, void *data) {

	//APP_LOG(APP_LOG_LEVEL_INFO, "PERFECT BG ANIMATE, ANIMATION STARTED ROUTINE");
  
	// clear out BG and icon
  text_layer_set_text(bg_layer, " ");
  text_layer_set_text(message_layer, "WHOO HOO!\0");
  
} // end perfectbg_animation_started

void perfectbg_animation_stopped(Animation *animation, bool finished, void *data) {

	//APP_LOG(APP_LOG_LEVEL_INFO, "PERFECT BG ANIMATE, ANIMATION STOPPED ROUTINE");
	
	// reset bg and icon
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "PERFECT BG ANIMATE, ANIMATION STOPPED, SET TO BG: %s ", last_bg);
	text_layer_set_text(bg_layer, last_bg);
	load_icon();
  load_bg_delta();
  
} // end perfectbg_animation_stopped

void destroy_perfectbg_animation(PropertyAnimation **perfectbg_animation) {
  if (*perfectbg_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *perfectbg_animation)) {
    animation_unschedule((Animation*) *perfectbg_animation);
  }

  property_animation_destroy(*perfectbg_animation);
  *perfectbg_animation = NULL;
} // end destroy_perfectbg_animation

void animate_perfectbg() {

	// VARIABLES 
	
	// for animation
	GRect from_perfectbg_rect = GRect(0,0,0,0);
	GRect to_perfectbg_rect = GRect(0,0,0,0);

  Layer *animate_perfectbg_layer = NULL;
  
	// CODE START

	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE PERFECTBG");
	// slide BG out to right
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE PERFECTBG, ANIMATE BG, SLIDE BG");
  if (currentBG_isMMOL) { 
    create_update_bitmap(&perfectbg_bitmap,perfectbg_layer,PERFECTBG_ICONS[CLUB55_ICON_INDX]);
  }
  else {
    create_update_bitmap(&perfectbg_bitmap,perfectbg_layer,PERFECTBG_ICONS[CLUB100_ICON_INDX]);
  }
	animate_perfectbg_layer = bitmap_layer_get_layer(perfectbg_layer);
	from_perfectbg_rect = GRect(0, 3, 95, 47);
	to_perfectbg_rect = GRect(144, 3, 95, 47);      
	destroy_perfectbg_animation(&perfectbg_animation);
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, CREATE FRAME");
	perfectbg_animation = property_animation_create_layer_frame(animate_perfectbg_layer, &from_perfectbg_rect, &to_perfectbg_rect);
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SET DURATION AND CURVE");
	animation_set_duration((Animation*) perfectbg_animation, PERFECTBG_ANIMATE_SECS*MS_IN_A_SECOND);
	animation_set_curve((Animation*) perfectbg_animation, AnimationCurveLinear);
  
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SET HANDLERS");
	animation_set_handlers((Animation*) perfectbg_animation, (AnimationHandlers) {
	  .started = (AnimationStartedHandler) perfectbg_animation_started,
	  .stopped = (AnimationStoppedHandler) perfectbg_animation_stopped,
	}, NULL /* callback data */);
      
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SCHEDULE");
	animation_schedule((Animation*) perfectbg_animation);   

} //end animate_perfectbg

// happymsg ANIMATION
void happymsg_animation_started(Animation *animation, void *data) {

	//APP_LOG(APP_LOG_LEVEL_INFO, "HAPPY MSG ANIMATE, ANIMATION STARTED ROUTINE, CLEAR OUT BG DELTA");
  
	// clear out BG delta / message layer
  text_layer_set_text(message_layer, "");
  text_layer_set_text(cgmtime_layer, "");
  text_layer_set_text(rig_battlevel_layer, "");
  create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);   
  
} // end happymsg_animation_started

void happymsg_animation_stopped(Animation *animation, bool finished, void *data) {

	//APP_LOG(APP_LOG_LEVEL_INFO, "HAPPY MSG ANIMATE, ANIMATION STOPPED ROUTINE");
	
	// set BG delta / message layer
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "HAPPY MSG ANIMATE, ANIMATION STOPPED, SET TO BG DELTA);
  load_bg_delta();
  load_cgmtime();
  load_apptime();
  load_rig_battlevel();
  
} // end happymsg_animation_stopped

void destroy_happymsg_animation(PropertyAnimation **happymsg_animation) {
  if (*happymsg_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *happymsg_animation)) {
    animation_unschedule((Animation*) *happymsg_animation);
  }

  property_animation_destroy(*happymsg_animation);
  *happymsg_animation = NULL;
} // end destroy_happymsg_animation

void animate_happymsg(char *happymsg_to_display) {

	// VARIABLES 
	
	// for animation
	GRect from_happymsg_rect = GRect(0,0,0,0);
	GRect to_happymsg_rect = GRect(0,0,0,0);

	Layer *animate_happymsg_layer = NULL;
  static char animate_happymsg_buffer[30] = {0};
  
	// CODE START

	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG");
	// slide BG out to right
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SLIDE BG");
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "ANIMATE HAPPY MSG, STRING PASSED: %s", happymsg_to_display);
  strncpy(animate_happymsg_buffer, happymsg_to_display, HAPPYMSG_BUFFER_SIZE);
	text_layer_set_text(happymsg_layer, animate_happymsg_buffer);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "ANIMATE HAPPY MSG, MSG IN BUFFER: %s", animate_happymsg_buffer);
  animate_happymsg_layer = text_layer_get_layer(happymsg_layer);
	from_happymsg_rect = GRect(-10, 33, 144, 55);
	to_happymsg_rect = GRect(144, 33, 144, 55);      
	destroy_happymsg_animation(&happymsg_animation);
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, CREATE FRAME");
	happymsg_animation = property_animation_create_layer_frame(animate_happymsg_layer, &from_happymsg_rect, &to_happymsg_rect);
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SET DURATION AND CURVE");
	animation_set_duration((Animation*) happymsg_animation, HAPPYMSG_ANIMATE_SECS*MS_IN_A_SECOND);
	animation_set_curve((Animation*) happymsg_animation, AnimationCurveLinear);
  
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SET HANDLERS");
	animation_set_handlers((Animation*) happymsg_animation, (AnimationHandlers) {
	  .started = (AnimationStartedHandler) happymsg_animation_started,
	  .stopped = (AnimationStoppedHandler) happymsg_animation_stopped,
	}, NULL /* callback data */);
      
	//APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SCHEDULE");
	animation_schedule((Animation*) happymsg_animation);   

} //end animate_happymsg

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

  // happy message; max message 24 characters
  // DO NOT GO OVER 24 CHARACTERS, INCLUDING SPACES OR YOU WILL CRASH
  // YOU HAVE BEEN WARNED
	char happymsg_buffer111[26] = "EVERYTHING IS AWESOME\0";
	char happymsg_buffer123[26] = "ABC EASY AS... D*CAN*BE\0";
	char happymsg_buffer130[26] = "DON'T WORRY BE:)HAPPY\0";
	char happymsg_buffer65[26] = "STAYIN ALIVE AH,HA,HA,HA\0";
	char happymsg_buffer200[26] = "SHAKE*IT*OFF  SHAKE IT!\0";
	char happymsg_buffer300[26] = "PREPARE 4 GLORY! SPARTA!\0";
  
	// CODE START
	
	// if special value set, erase anything in the icon field
	if (specvalue_alert == true) {
	  create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
	}
	
	// set special value alert to false no matter what
	specvalue_alert = false;
  
    // see if we're doing MGDL or MMOL; get currentBG_isMMOL value in myBGAtoi
	  // convert BG value from string to int
	  
	  // FOR TESTING ONLY
    //strncpy(last_bg, "5", BG_BUFFER_SIZE);
	
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
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
	    text_layer_set_text(bg_layer, "");
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, SET BITMAP");
	    create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[QUESTION_MARKS_ICON_INDX]); 
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, DONE");
	    specvalue_alert = true;
	  }
	  else if (current_bg < bg_ptr[SPECVALUE_BG_INDX]) {
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED SPECIAL VALUE: SET LOGO ICON");
	    text_layer_set_text(bg_layer, "");
	    create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGO_SPECVALUE_ICON_INDX]);
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
 
      if (!HardCodeNoAnimations) {
        if ( ((!currentBG_isMMOL) && (current_bg == 100)) || ((currentBG_isMMOL) && (current_bg == 55)) ) {
		      // PERFECT BG CLUB, ANIMATE BG      
		      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE PERFECT BG");
		      animate_perfectbg();
        } // perfect bg club, animate BG

        // EVERY TIME YOU DO A NEW MESSAGE, YOU HAVE TO ALLOCATE A NEW HAPPY MSG BUFFER AT THE TOP OF LOAD BG FUNCTION
        
        if ((!currentBG_isMMOL) && (current_bg == 123)) {
		      // ANIMATE HAPPY MSG LAYER     
		      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		      animate_happymsg(happymsg_buffer123);
        } // animate happy msg layer @ 123
      
        if ((currentBG_isMMOL) && (current_bg == 65)) {
		      // ANIMATE HAPPY MSG LAYER     
		      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		      animate_happymsg(happymsg_buffer65);
        } // animate happy msg layer @ 65
      
        if ( ((!currentBG_isMMOL) && (current_bg == 200)) || ((currentBG_isMMOL) && (current_bg == 110)) ) {
		      // ANIMATE HAPPY MSG LAYER     
		      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		      animate_happymsg(happymsg_buffer200);
        } // animate happy msg layer @ 300
		
        if ((!currentBG_isMMOL) && (current_bg == 300)) {
		      // ANIMATE HAPPY MSG LAYER     
		      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		      animate_happymsg(happymsg_buffer300);
        } // animate happy msg layer @ 300
        
        if (HardCodeAllAnimations) {
          // extra animations for those that want them, these are the old ones
		      if ( ((!currentBG_isMMOL) && (current_bg == 111)) || ((currentBG_isMMOL) && (current_bg == 60)) ) {
		        // ANIMATE HAPPY MSG LAYER     
		        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		        animate_happymsg(happymsg_buffer111);
		      } // animate happy msg layer @ 111
        
		      if ( ((!currentBG_isMMOL) && (current_bg == 130)) || ((currentBG_isMMOL) && (current_bg == 70)) ) {
		        // ANIMATE HAPPY MSG LAYER     
		        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
		        animate_happymsg(happymsg_buffer130);
		      } // animate happy msg layer @ 130          
        } // HardCodeAllAnimations
        
      } // HardCodeNoAnimations; end all animation code
	  
      }	  } // end bg checks (if special_value_bitmap)
       
      // see if we're going to use the current bg or the calculated raw bg for vibrations
      if ( ((current_bg > 0) && (current_bg < bg_ptr[SPECVALUE_BG_INDX])) && (HaveCalcRaw) ) {
        
        current_calc_raw = myBGAtoi(last_calc_raw);
        
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, TurnOffVibrationsCalcRaw: %d", TurnOffVibrationsCalcRaw);
         
        if (!TurnOffVibrationsCalcRaw) {
          // set current_bg to calculated raw so we can vibrate on that instead
          current_bg = current_calc_raw;
          if (!currentBG_isMMOL) {
            bg_ptr = BG_MGDL;
            specvalue_ptr = SPECVALUE_MGDL;
          }
          else {
            bg_ptr = BG_MMOL;
            specvalue_ptr = SPECVALUE_MMOL;
          }
        } // TurnOffVibrationsCalcRaw
        
        // use calculated raw values in BG field; if different cascade down so we have last three values
        if (current_calc_raw != current_calc_raw1) {
            strncpy(last_calc_raw3, last_calc_raw2, BG_BUFFER_SIZE);
            strncpy(last_calc_raw2, last_calc_raw1, BG_BUFFER_SIZE);
            strncpy(last_calc_raw1, last_calc_raw, BG_BUFFER_SIZE);
            current_calc_raw1 = current_calc_raw;
        }
      }
      
      else {
        // if not in special values or don't have calculated raw, blank out the fields
        strncpy(last_calc_raw1, " ", BG_BUFFER_SIZE);
        strncpy(last_calc_raw2, " ", BG_BUFFER_SIZE);
        strncpy(last_calc_raw3, " ", BG_BUFFER_SIZE);
      }
      
      // set bg field accordingly for calculated raw layer
      text_layer_set_text(calcraw_last1_layer, last_calc_raw1);
      text_layer_set_text(calcraw_last2_layer, last_calc_raw2);
      text_layer_set_text(calcraw_last3_layer, last_calc_raw3);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, START VIBRATE, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, START VIBRATE, CURRENT_CALC_RAW: %d LAST_CALC_RAW: %s ", current_calc_raw, last_calc_raw);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, START VIBRATE, CALC_RAW 2: %d FORMAT CALC RAW 2: %s ", current_calc_raw2, formatted_calc_raw2);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, START VIBRATE, CALC_RAW 3: %d FORMAT CALC RAW 2: %s ", current_calc_raw3, formatted_calc_raw3);
      
      // check BG and vibrate if needed
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, CHECK TO SEE IF WE NEED TO VIBRATE");
	  // check for SPECIAL VALUE
      if ( ( ((current_bg > 0) && (current_bg < bg_ptr[SPECVALUE_BG_INDX]))
          && ((lastAlertTime == 0) || (lastAlertTime > SPECVALUE_SNZ_MIN)) )
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
        if (lastAlertTime > SPECVALUE_SNZ_MIN) { 
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
             && ((lastAlertTime == 0) || (lastAlertTime > HYPOLOW_SNZ_MIN)) ) 
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
        if (lastAlertTime > HYPOLOW_SNZ_MIN) { 
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
             && ((lastAlertTime == 0) || (lastAlertTime > BIGLOW_SNZ_MIN)) ) 
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
        if (lastAlertTime > BIGLOW_SNZ_MIN) { 
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
             && ((lastAlertTime == 0) || (lastAlertTime > MIDLOW_SNZ_MIN)) ) 
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
        if (lastAlertTime > MIDLOW_SNZ_MIN) { 
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
               && ((lastAlertTime == 0) || (lastAlertTime > LOW_SNZ_MIN)) )
             || ( ((current_bg > bg_ptr[MIDLOW_BG_INDX]) && (current_bg <= bg_ptr[LOW_BG_INDX])) && (!low_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, LOW BG ALERT");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime LOW SNOOZE VALUE IN: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "low_snz_min LOW SNOOZE VALUE IN: %i", LOW_SNZ_MIN);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite IN: %i", low_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!low_overwrite)) { 
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, LOW: VIBRATE");
          alert_handler_cgm(LOWBG_VIBE); 
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!low_overwrite) { low_overwrite = true; }
        }
      
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime > LOW_SNZ_MIN) {
          lastAlertTime = 0; 
          specvalue_overwrite = false;
          hypolow_overwrite = false;
          biglow_overwrite = false;
          midlow_overwrite = false;
          low_overwrite = false;
        }
      
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime LOW SNOOZE VALUE OUT: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "low_snz_min LOW SNOOZE VALUE OUT: %i", LOW_SNZ_MIN);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "low_overwrite OUT: %i", low_overwrite);
      }  // else if LOW BG
      
      // check for HIGH BG
      else if ( ( ((current_bg >= bg_ptr[HIGH_BG_INDX]) && (current_bg < bg_ptr[MIDHIGH_BG_INDX])) 
             && ((lastAlertTime == 0) || (lastAlertTime > HIGH_SNZ_MIN)) ) 
           || ( ((current_bg >= bg_ptr[HIGH_BG_INDX]) && (current_bg < bg_ptr[MIDHIGH_BG_INDX])) && (!high_overwrite) ) ) {
      
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HIGH BG ALERT");   
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HIGH SNOOZE VALUE IN: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "high_snz_min HIGH SNOOZE VALUE IN: %i", HIGH_SNZ_MIN);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "high_overwrite IN: %i", high_overwrite);
      
        // send alert and handle a bouncing connection
        if ((lastAlertTime == 0) || (!high_overwrite)) {  
		  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, HIGH: VIBRATE");
          alert_handler_cgm(HIGHBG_VIBE);
          if (lastAlertTime == 0) { lastAlertTime = 1; }
          if (!high_overwrite) { high_overwrite = true; }
        }
       
        // if hit snooze, reset snooze counter; will alert next time around
        if (lastAlertTime > HIGH_SNZ_MIN) {
          lastAlertTime = 0; 
          specvalue_overwrite = false;
          high_overwrite = false;
          midhigh_overwrite = false;
          bighigh_overwrite = false;
        } 
       
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime HIGH SNOOZE VALUE OUT: %i", lastAlertTime);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "high_snz_min HIGH SNOOZE VALUE OUT: %i", HIGH_SNZ_MIN);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "high_overwrite OUT: %i", high_overwrite);
      } // else if HIGH BG
    
      // check for MID HIGH BG
      else if ( ( ((current_bg >= bg_ptr[MIDHIGH_BG_INDX]) && (current_bg < bg_ptr[BIGHIGH_BG_INDX])) 
               && ((lastAlertTime == 0) || (lastAlertTime > MIDHIGH_SNZ_MIN)) )
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
        if (lastAlertTime > MIDHIGH_SNZ_MIN) { 
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
               && ((lastAlertTime == 0) || (lastAlertTime > BIGHIGH_SNZ_MIN)) )
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
        if (lastAlertTime > BIGHIGH_SNZ_MIN) { 
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
	uint32_t current_cgm_timeago = 0;
	int cgm_timeago_diff = 0;
    
	// CODE START
	
    // initialize label buffer
    strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);
       
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, NEW CGM TIME: %s", new_cgm_time);
    
    if (current_cgm_time == 0) {     
      // Init code or error code; set text layer & icon to empty value 
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
      text_layer_set_text(cgmtime_layer, "");
      create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);            
    }
    else {
	  // set rcvr on icon
      create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);
      
      cgm_time_now = time(NULL);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, TIME NOW IN CGM: %lu", cgm_time_now);
        
      current_cgm_timeago = abs(cgm_time_now - current_cgm_time);
        
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
        create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);        
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
	uint32_t current_app_timeago = 0;
	int app_timeago_diff = 0;
	  
	// CODE START
	
	draw_date_from_app();  
       
      app_time_now = time(NULL);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, TIME NOW: %lu", app_time_now);
      
      current_app_timeago = abs(app_time_now - current_app_time);
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, CURRENT APP TIMEAGO: %lu", current_app_timeago);
      
	  app_timeago_diff = (current_app_timeago / MINUTEAGO);
	  if ( (current_app_timeago < HOURAGO) && (app_timeago_diff >= PHONEOUT_WAIT_MIN) ) {
              
        // erase cgm ago times and cgm icon
        text_layer_set_text(cgmtime_layer, "");
        create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRNONE_ICON_INDX]);
		
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
    //} // else init code 
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, FUNCTION OUT");
} // end load_apptime

static void load_bg_delta() {
	//APP_LOG(APP_LOG_LEVEL_INFO, "BG DELTA FUNCTION START");
	
	// CONSTANTS
	const uint8_t MSGLAYER_BUFFER_SIZE = 14;
	const uint8_t BGDELTA_LABEL_SIZE = 14;
	const uint8_t BGDELTA_FORMATTED_SIZE = 14;
	
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
	  text_layer_set_text(message_layer, "CHECK RIG");
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
	if (strcmp(current_bg_delta, "OFF") == 0) {
    if (dataoffline_retries_counter >= DATAOFFLINE_RETRIES_MAX) {
      strncpy(formatted_bg_delta, "ATTN: NO DATA", MSGLAYER_BUFFER_SIZE);
      text_layer_set_text(message_layer, formatted_bg_delta);
      text_layer_set_text(bg_layer, " ");
      if (!DataOfflineAlert) {
        alert_handler_cgm(DATAOFFLINE_VIBE);
        DataOfflineAlert = true;
      } // DataOfflineAlert
      // NOTE: DataOfflineAlert is cleared in load_icon because that means we got a good message again
      // NOTE: dataoffline_retries_counter is cleared in load_icon because that means we got a good message again
    }  
    else {
      dataoffline_retries_counter++; 
	  }  
	  return;	
  } // strcmp "OFF"
  
  	// check if LOADING.., if true set message
  	// put " " (space) in bg field so logo continues to show
    if (strcmp(current_bg_delta, "LOAD") == 0) {
      strncpy(formatted_bg_delta, "LOADING 7.0", MSGLAYER_BUFFER_SIZE);
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

static void load_rig_battlevel() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, FUNCTION START");

	// CONSTANTS
  const uint8_t BATTLEVEL_LABEL_SIZE = 5;
	const uint8_t BATTLEVEL_PERCENT_SIZE = 6;
	
	// VARIABLES
	int current_battlevel = 0;

	// CODE START
	
	// initialize inverter layer to hide
	layer_set_hidden((Layer *)inv_rig_battlevel_layer, true);

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, LAST BATTLEVEL: %s", last_battlevel);
  
	if (strcmp(last_battlevel, " ") == 0) {
      // Init code or no battery, can't do battery; set text layer & icon to empty value 
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, NO BATTERY");
      text_layer_set_text(rig_battlevel_layer, "");
      LowBatteryAlert = false;	  
      return;
    }
  
	if (strcmp(last_battlevel, "0") == 0) {
      // Zero battery level; set here, so if we get zero later we know we have an error instead
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, SET STRING");
      text_layer_set_text(rig_battlevel_layer, "0%");
      layer_set_hidden((Layer *)inv_rig_battlevel_layer, false);
      if (!LowBatteryAlert) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, VIBRATE");
		alert_handler_cgm(LOWBATTERY_VIBE);
		LowBatteryAlert = true;
      }	 
      return;
    }
  
	current_battlevel = myAtoi(last_battlevel);
  
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, CURRENT BATTLEVEL: %i", current_battlevel);
  
	if ((current_battlevel <= 0) || (current_battlevel > 100) || (last_battlevel[0] == '-')) { 
    // got a negative or out of bounds or error battery level
	  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, UNKNOWN, ERROR BATTERY");
	  text_layer_set_text(rig_battlevel_layer, "ERR");
	  layer_set_hidden((Layer *)inv_rig_battlevel_layer, false);
    return;
	}
	// initialize formatted battlevel
  strncpy(formatted_battlevel, " ", BATTLEVEL_FORMAT_SIZE);
  
	// get current battery level and set battery level text with percent
  if (current_battlevel < 10) { strncpy(formatted_battlevel, "Rig  ", BATTLEVEL_LABEL_SIZE); }
  else { strncpy(formatted_battlevel, "Rig ", BATTLEVEL_LABEL_SIZE); }
	snprintf(battlevel_percent, BATTLEVEL_PERCENT_SIZE, "%i%%", current_battlevel);
  strcat(formatted_battlevel, battlevel_percent);
	text_layer_set_text(rig_battlevel_layer, formatted_battlevel);	  

	if ( (current_battlevel > 10) && (current_battlevel <= 20) ) {
    layer_set_hidden((Layer *)inv_rig_battlevel_layer, false);
    if (!LowBatteryAlert) {
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 20 OR LESS, VIBRATE");
	    alert_handler_cgm(LOWBATTERY_VIBE);
	    LowBatteryAlert = true;
    }
	}
  
	if ( (current_battlevel > 5) && (current_battlevel <= 10) ) {
    layer_set_hidden((Layer *)inv_rig_battlevel_layer, false);
    if (!LowBatteryAlert) {
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 10 OR LESS, VIBRATE");
	    alert_handler_cgm(LOWBATTERY_VIBE);
	    LowBatteryAlert = true;
    }
  }
  
	if ( (current_battlevel > 0) && (current_battlevel <= 5) ) {
    layer_set_hidden((Layer *)inv_rig_battlevel_layer, false);
    if (!LowBatteryAlert) {
	    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, LOW BATTERY, 5 OR LESS, VIBRATE");
	    alert_handler_cgm(LOWBATTERY_VIBE);
	    LowBatteryAlert = true;
    }	  
  }
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, END FUNCTION");
} // end load_rig_battlevel

static void load_noise() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD NOISE, FUNCTION START");

  	// CONSTANTS
  	const uint8_t NOISE_FORMATTED_SIZE = 8;
  
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD NOISE, CURRENT NOISE VALUE: %i ", current_noise_value);  
    
    switch (current_noise_value) {
      
    case NO_NOISE:;
      strncpy(formatted_noise, " ", NOISE_FORMATTED_SIZE);
      break;
    case CLEAN_NOISE:;
      strncpy(formatted_noise, "Cln", NOISE_FORMATTED_SIZE);
      break;
    case LIGHT_NOISE:;
      strncpy(formatted_noise, "Lgt", NOISE_FORMATTED_SIZE);
      break;
    case MEDIUM_NOISE:;
      strncpy(formatted_noise, "Med", NOISE_FORMATTED_SIZE);
      break;
    case HEAVY_NOISE:;
      strncpy(formatted_noise, "Hvy", NOISE_FORMATTED_SIZE);
      break;
    case WARMUP_NOISE:;
      strncpy(formatted_noise, " ", NOISE_FORMATTED_SIZE);
      break;
    case OTHER_NOISE:;
      strncpy(formatted_noise, " ", NOISE_FORMATTED_SIZE);
      break;
    default:;
      strncpy(formatted_noise, "ERR", NOISE_FORMATTED_SIZE);
    }
  
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, NOISE: %s ", formatted_noise);
  
    text_layer_set_text(noise_layer, formatted_noise);
  
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD NOISE, END FUNCTION");
} // end load_noise

void sync_tuple_changed_callback_cgm(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");
	
	// CONSTANTS
	const uint8_t ICON_MSGSTR_SIZE = 4;
	const uint8_t BG_MSGSTR_SIZE = 6;
	const uint8_t BGDELTA_MSGSTR_SIZE = 6;
	const uint8_t BATTLEVEL_MSGSTR_SIZE = 4;
	const uint8_t VALUE_MSGSTR_SIZE = 25;

	// CODE START
	
  // reset appsync retries counter
  appsyncandmsg_retries_counter = 0;
  
  //parse key and tuple
	switch (key) {

	case CGM_ICON_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: ICON ARROW");
      strncpy(current_icon, new_tuple->value->cstring, ICON_MSGSTR_SIZE);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, ICON VALUE: %s ", current_icon);
	  load_icon();
      break; // break for CGM_ICON_KEY

	case CGM_BG_KEY:;
	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG CURRENT");
      strncpy(last_bg, new_tuple->value->cstring, BG_MSGSTR_SIZE);
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BG VALUE: %s ", last_bg);
      load_bg();
      break; // break for CGM_BG_KEY

	case CGM_TCGM_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME");
      current_cgm_time = new_tuple->value->uint32;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CGM TIME VALUE: %i ", current_cgm_time);
      load_cgmtime();
      break; // break for CGM_TCGM_KEY

	case CGM_TAPP_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ APP TIME NOW");
      current_app_time = new_tuple->value->uint32;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, APP TIME VALUE: %i ", current_app_time);
      load_apptime();    
      break; // break for CGM_TAPP_KEY

	case CGM_DLTA_KEY:;
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG DELTA");
	  strncpy(current_bg_delta, new_tuple->value->cstring, BGDELTA_MSGSTR_SIZE);
   	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BG DELTA VALUE: %s ", current_bg_delta);
	  load_bg_delta();
	  break; // break for CGM_DLTA_KEY
	
	case CGM_UBAT_KEY:;
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: UPLOADER BATTERY LEVEL");
   	  //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL IN, COPY LAST BATTLEVEL");
      strncpy(last_battlevel, new_tuple->value->cstring, BATTLEVEL_MSGSTR_SIZE);
   	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BATTERY LEVEL VALUE: %s ", last_battlevel);
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL, CALL LOAD BATTLEVEL");
      load_rig_battlevel();
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL OUT");
      break; // break for CGM_UBAT_KEY

	case CGM_NAME_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: T1D NAME");
      text_layer_set_text(t1dname_layer, new_tuple->value->cstring);
      break; // break for CGM_NAME_KEY
    
  case CGM_VALS_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: VALUES");
      strncpy(current_values, new_tuple->value->cstring, VALUE_MSGSTR_SIZE);
      load_values();
      break; // break for CGM_VALS_KEY
    
  case CGM_CLRW_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: CALCULATED RAW");
      strncpy(last_calc_raw, new_tuple->value->cstring, BG_MSGSTR_SIZE);
      if ( (strcmp(last_calc_raw, "0") == 0) || (strcmp(last_calc_raw, "0.0") == 0) ) {
        strncpy(last_calc_raw, " ", BG_MSGSTR_SIZE);
        HaveCalcRaw = false;
      }
      else { HaveCalcRaw = true; }  
      text_layer_set_text(raw_calc_layer, last_calc_raw);
      break; // break for CGM_CLRW_KEY
    
 	case CGM_RWUF_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: RAW UNFILTERED");
      strncpy(last_raw_unfilt, new_tuple->value->cstring, BG_MSGSTR_SIZE);
      if ( (strcmp(last_raw_unfilt, "0") == 0) || (strcmp(last_raw_unfilt, "0.0") == 0) || (!TurnOnUnfilteredRaw) ) {
        strncpy(last_raw_unfilt, " ", BG_MSGSTR_SIZE);
      }
      text_layer_set_text(raw_unfilt_layer, last_raw_unfilt);
      break; // break for CGM_RWUF_KEY
    
  case CGM_NOIZ_KEY:;
      //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: NOISE");
	    current_noise_value = new_tuple->value->uint8;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, NOISE: %i ", current_noise_value);
      load_noise();
      break; // break for CGM_NOIZ_KEY
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
    if (timeformat == 0){
      tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", tick_time_cgm);
    } else {
      tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%H:%M", tick_time_cgm);
    }
	if (tick_return_cgm != 0) {
      text_layer_set_text(time_watch_layer, time_watch_text);
	}
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
	// increment BG snooze
    ++lastAlertTime;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
	
  // check watch battery
  handle_watch_battery_cgm(battery_state_service_peek());
    
  } 
  
} // end handle_minute_tick_cgm

void window_load_cgm(Window *window_cgm) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");
  
  // VARIABLES
  Layer *window_layer_cgm = NULL;
  
  // CODE START

  window_layer_cgm = window_get_root_layer(window_cgm);
  
  // TOPHALF WHITE
  tophalf_layer = text_layer_create(GRect(0, 0, 144, 83));
  text_layer_set_text_color(tophalf_layer, GColorBlack);
  text_layer_set_background_color(tophalf_layer, GColorWhite);
  text_layer_set_font(tophalf_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(tophalf_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(tophalf_layer));
  
  // HAPPY MSG LAYER
  happymsg_layer = text_layer_create(GRect(-10, 33, 144, 55));
  text_layer_set_text_color(happymsg_layer, GColorBlack);
  text_layer_set_background_color(happymsg_layer, GColorClear);
  text_layer_set_font(happymsg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(happymsg_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(happymsg_layer));
  
  // DELTA BG / MESSAGE LAYER
  message_layer = text_layer_create(GRect(0, 33, 144, 55));
  text_layer_set_text_color(message_layer, GColorBlack);
  text_layer_set_background_color(message_layer, GColorClear);
  text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));
  
  // ICON, ARROW OR SPECIAL VALUE
  icon_layer = bitmap_layer_create(GRect(78, -2, 78, 50));
  bitmap_layer_set_alignment(icon_layer, GAlignCenter);
  bitmap_layer_set_background_color(icon_layer, GColorClear);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(icon_layer)); 

  // RIG BATTERY LEVEL
  rig_battlevel_layer = text_layer_create(GRect(70, 61, 72, 22));
  text_layer_set_text_color(rig_battlevel_layer, GColorBlack);
  text_layer_set_background_color(rig_battlevel_layer, GColorClear);
  text_layer_set_font(rig_battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(rig_battlevel_layer, GTextAlignmentRight);
  layer_add_child(window_layer_cgm, text_layer_get_layer(rig_battlevel_layer));
  
  // INVERTER BATTERY LAYER
  inv_rig_battlevel_layer = inverter_layer_create(GRect(112, 66, 30, 15));
  layer_add_child(window_get_root_layer(window_cgm), inverter_layer_get_layer(inv_rig_battlevel_layer));
  
  // BG
  bg_layer = text_layer_create(GRect(0, -5, 95, 47));
  text_layer_set_text_color(bg_layer, GColorBlack);
  text_layer_set_background_color(bg_layer, GColorClear);
  text_layer_set_font(bg_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(bg_layer));

  // CALCULATED RAW INSTEAD OF BG - LAST VALUE (1)
  calcraw_last1_layer = text_layer_create(GRect(0, -7, 40, 25));
  text_layer_set_text_color(calcraw_last1_layer, GColorBlack);
  text_layer_set_background_color(calcraw_last1_layer, GColorClear);
  text_layer_set_font(calcraw_last1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(calcraw_last1_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(calcraw_last1_layer));
  
  // CALCULATED RAW INSTEAD OF BG - 2ND LAST VALUE (2)
  calcraw_last2_layer = text_layer_create(GRect(32, 3, 40, 25));
  text_layer_set_text_color(calcraw_last2_layer, GColorBlack);
  text_layer_set_background_color(calcraw_last2_layer, GColorClear);
  text_layer_set_font(calcraw_last2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(calcraw_last2_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(calcraw_last2_layer)); 
  
  // CALCULATED RAW INSTEAD OF BG - 3RD LAST VALUE (3)
  calcraw_last3_layer = text_layer_create(GRect(63, 16, 40, 25));
  text_layer_set_text_color(calcraw_last3_layer, GColorBlack);
  text_layer_set_background_color(calcraw_last3_layer, GColorClear);
  text_layer_set_font(calcraw_last3_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(calcraw_last3_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(calcraw_last3_layer)); 
  
  // PERFECT BG
  perfectbg_layer = bitmap_layer_create(GRect(0, -7, 95, 47));
  bitmap_layer_set_alignment(perfectbg_layer, GAlignTopLeft);
  bitmap_layer_set_background_color(perfectbg_layer, GColorClear);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(perfectbg_layer));
  
  // CGM TIME AGO ICON
  cgmicon_layer = bitmap_layer_create(GRect(2, 63, 40, 19));
  bitmap_layer_set_alignment(cgmicon_layer, GAlignLeft);
  bitmap_layer_set_background_color(cgmicon_layer, GColorWhite);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(cgmicon_layer));  
  
  // CGM TIME AGO READING
  cgmtime_layer = text_layer_create(GRect(26, 56, 40, 24));
  text_layer_set_text_color(cgmtime_layer, GColorBlack);
  text_layer_set_background_color(cgmtime_layer, GColorClear);
  text_layer_set_font(cgmtime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(cgmtime_layer));

  // T1D NAME
  t1dname_layer = text_layer_create(GRect(2, 140, 69, 28));
  text_layer_set_text_color(t1dname_layer, GColorWhite);
  text_layer_set_background_color(t1dname_layer, GColorClear);
  text_layer_set_font(t1dname_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(t1dname_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(t1dname_layer));
  
  // WATCH BATTERY LEVEL
  watch_battlevel_layer = text_layer_create(GRect(71, 145, 72, 22));
  text_layer_set_text_color(watch_battlevel_layer, GColorWhite);
  text_layer_set_background_color(watch_battlevel_layer, GColorBlack);
  text_layer_set_font(watch_battlevel_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(watch_battlevel_layer, GTextAlignmentRight);
  handle_watch_battery_cgm(battery_state_service_peek());
  layer_add_child(window_layer_cgm, text_layer_get_layer(watch_battlevel_layer));

  // TIME; CURRENT ACTUAL TIME FROM WATCH
  time_watch_layer = text_layer_create(GRect(0, 102, 144, 44));
  text_layer_set_text_color(time_watch_layer, GColorWhite);
  text_layer_set_background_color(time_watch_layer, GColorClear);
  text_layer_set_font(time_watch_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(time_watch_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(time_watch_layer));
  
  // DATE
  date_app_layer = text_layer_create(GRect(39, 80, 72, 28));
  text_layer_set_text_color(date_app_layer, GColorWhite);
  text_layer_set_background_color(date_app_layer, GColorClear);
  text_layer_set_font(date_app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(date_app_layer, GTextAlignmentCenter);
  draw_date_from_app();
  layer_add_child(window_layer_cgm, text_layer_get_layer(date_app_layer));
  
  // RAW CALCULATED
  raw_calc_layer = text_layer_create(GRect(1, 76, 40, 25));
  text_layer_set_text_color(raw_calc_layer, GColorWhite);
  text_layer_set_background_color(raw_calc_layer, GColorClear);
  text_layer_set_font(raw_calc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(raw_calc_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(raw_calc_layer));
  
  // NOISE
  noise_layer = text_layer_create(GRect(85, 76, 58, 27));
  text_layer_set_text_color(noise_layer, GColorWhite);
  text_layer_set_background_color(noise_layer, GColorClear);
  text_layer_set_font(noise_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(noise_layer, GTextAlignmentRight);
  layer_add_child(window_layer_cgm, text_layer_get_layer(noise_layer));
  
  // RAW UNFILT
  raw_unfilt_layer = text_layer_create(GRect(1, 92, 40, 25)); 
  text_layer_set_text_color(raw_unfilt_layer, GColorWhite);
  text_layer_set_background_color(raw_unfilt_layer, GColorClear);
  text_layer_set_font(raw_unfilt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(raw_unfilt_layer, GTextAlignmentLeft);
  layer_add_child(window_layer_cgm, text_layer_get_layer(raw_unfilt_layer));
  
  // put " " (space) in bg field so logo continues to show
  // " " (space) also shows these are init values, not bad or null values
  Tuplet initial_values_cgm[] = {
  TupletCString(CGM_ICON_KEY, " "),
	TupletCString(CGM_BG_KEY, " "),
	TupletInteger(CGM_TCGM_KEY, 0),
	TupletInteger(CGM_TAPP_KEY, 0),
	TupletCString(CGM_DLTA_KEY, "LOAD"),
	TupletCString(CGM_UBAT_KEY, " "),
	TupletCString(CGM_NAME_KEY, " "),
  TupletCString(CGM_VALS_KEY, " "),
  TupletCString(CGM_CLRW_KEY, " "),
  TupletCString(CGM_RWUF_KEY, " "),
  TupletInteger(CGM_NOIZ_KEY, 0)
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
  destroy_null_GBitmap(&cgmicon_bitmap);
  destroy_null_GBitmap(&specialvalue_bitmap);
  destroy_null_GBitmap(&perfectbg_bitmap);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY BITMAPS IF EXIST");  
  destroy_null_BitmapLayer(&icon_layer);
  destroy_null_BitmapLayer(&cgmicon_layer);
  destroy_null_BitmapLayer(&perfectbg_layer);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY TEXT LAYERS IF EXIST");  
  destroy_null_TextLayer(&tophalf_layer);
  destroy_null_TextLayer(&bg_layer);
  destroy_null_TextLayer(&cgmtime_layer);
  destroy_null_TextLayer(&message_layer);
  destroy_null_TextLayer(&rig_battlevel_layer);
  destroy_null_TextLayer(&watch_battlevel_layer);  
  destroy_null_TextLayer(&t1dname_layer);
  destroy_null_TextLayer(&time_watch_layer);
  destroy_null_TextLayer(&date_app_layer);
  destroy_null_TextLayer(&happymsg_layer);
  destroy_null_TextLayer(&raw_calc_layer);
  destroy_null_TextLayer(&raw_unfilt_layer);
  destroy_null_TextLayer(&noise_layer);
  destroy_null_TextLayer(&calcraw_last1_layer);
  destroy_null_TextLayer(&calcraw_last2_layer);
  destroy_null_TextLayer(&calcraw_last3_layer);

  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY INVERTER LAYERS IF EXIST");  
  destroy_null_InverterLayer(&inv_rig_battlevel_layer);
  
  // destroy animation
  destroy_perfectbg_animation(&perfectbg_animation);
  destroy_perfectbg_animation(&happymsg_animation);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD OUT");
} // end window_unload_cgm

static void init_cgm(void) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");

  // subscribe to the tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);

  // subscribe to the bluetooth connection service
  bluetooth_connection_service_subscribe(&handle_bluetooth_cgm);
  
  // subscribe to the watch battery state service
  battery_state_service_subscribe(&handle_watch_battery_cgm);
  
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
  
  // unsubscribe to the watch battery state service
  battery_state_service_unsubscribe();
  
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
