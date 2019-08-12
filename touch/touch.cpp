#include "touch.h"
#include <math.h>

template<typename T> inline T CLIP(T v, T amin, T amax)
	{ if (v < amin) return amin; else if (v > amax) return amax; else return v; }

#ifdef SWITCH
#define DISPLAY_WIDTH 1280
#define DISPLAY_HEIGHT 720
#elif __vita__
#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 544
#else
// TODO: How to find display size for each platform programmatically?
#define DISPLAY_WIDTH 1920
#define DISPLAY_HEIGHT 1080
#endif
#define GAME_WIDTH 640
#define GAME_HEIGHT 480
#define TOUCH_PORT_MAX_NUM 1
#define NO_TOUCH -1 // finger id setting if finger is not touching the screen

static void init_touch(void);
static void preprocess_events(SDL_Event *event);
static void preprocess_finger_down(SDL_Event *event);
static void preprocess_finger_up(SDL_Event *event);
static void preprocess_finger_motion(SDL_Event *event);
static void set_mouse_button_event(SDL_Event *event, uint32_t type, uint8_t button, int32_t x, int32_t y);
static void set_mouse_motion_event(SDL_Event *event, int32_t x, int32_t y, int32_t xrel, int32_t yrel);
static void convert_touch_xy_to_game_xy(float touch_x, float touch_y, int *game_x, int *game_y);

static int touch_initialized = 0;
static unsigned int simulated_click_start_time[TOUCH_PORT_MAX_NUM][2]; // initiation time of last simulated left or right click (zero if no click)
static int direct_touch = 1; // pointer jumps to finger
static int mouse_x = 0; // always reflects current mouse position
static int mouse_y = 0;

enum {
	MAX_NUM_FINGERS = 3, // number of fingers to track per panel
	MAX_TAP_TIME = 250, // taps longer than this will not result in mouse click events
	MAX_TAP_MOTION_DISTANCE = 10, // max distance finger motion in Vita screen pixels to be considered a tap
	SIMULATED_CLICK_DURATION = 50, // time in ms how long simulated mouse clicks should be
}; // track three fingers per panel

typedef struct {
	int id; // -1: not touching
	uint32_t time_last_down;
	int last_x; // last known screen coordinates
	int last_y; // last known screen coordinates
	float last_down_x; // SDL touch coordinates when last pressed down
	float last_down_y; // SDL touch coordinates when last pressed down
} Touch;

static Touch finger[TOUCH_PORT_MAX_NUM][MAX_NUM_FINGERS]; // keep track of finger status

typedef enum {
	DRAG_NONE = 0,
	DRAG_TWO_FINGER,
	DRAG_THREE_FINGER,
} DraggingType;

static DraggingType multi_finger_dragging[TOUCH_PORT_MAX_NUM]; // keep track whether we are currently drag-and-dropping

static void init_touch(void)
{
	for (int port = 0; port < TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < MAX_NUM_FINGERS; i++) {
			finger[port][i].id = NO_TOUCH;
		}
		multi_finger_dragging[port] = DRAG_NONE;
	}
	
	for (int port = 0; port < TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < 2; i++) {
			simulated_click_start_time[port][i] = 0;
		}
	}
}

void handle_touch(SDL_Event *event, int current_mouse_x, int current_mouse_y)
{
	mouse_x = current_mouse_x;
	mouse_y = current_mouse_y;

	if (!touch_initialized) {
		init_touch();
		touch_initialized = 1;
	}
	preprocess_events(event);
	if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
		event->type = SDL_USEREVENT;
		event->user.code = -1; // ensure this event is ignored;
	}
}

static void preprocess_events(SDL_Event *event)
{
	// Supported touch gestures:
	// left mouse click: single finger short tap
	// right mouse click: second finger short tap while first finger is still down
	// pointer motion: single finger drag
	// left button drag and drop: dual finger drag
	// right button drag and drop: triple finger drag
	if (event->type != SDL_FINGERDOWN && event->type != SDL_FINGERUP && event->type != SDL_FINGERMOTION) {
		return;
	}

	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	if (port != 0) {
		return;
	}

	switch (event->type) {
		case SDL_FINGERDOWN:
		preprocess_finger_down(event);
		break;
		case SDL_FINGERUP:
		preprocess_finger_up(event);
		break;
		case SDL_FINGERMOTION:
		preprocess_finger_motion(event);
		break;
	}
}

static void preprocess_finger_down(SDL_Event *event)
{
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	int x = mouse_x;
	int y = mouse_y;
	
	if (direct_touch) {
		convert_touch_xy_to_game_xy(event->tfinger.x, event->tfinger.y, &x, &y);
	}

	// make sure each finger is not reported down multiple times
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (finger[port][i].id != id) {
			continue;
		}
		finger[port][i].id = NO_TOUCH;
	}

	// we need the timestamps to decide later if the user performed a short tap (click)
	// or a long tap (drag)
	// we also need the last coordinates for each finger to keep track of dragging
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (finger[port][i].id != NO_TOUCH) {
			continue;
		}
		finger[port][i].id = id;
		finger[port][i].time_last_down = event->tfinger.timestamp;
		finger[port][i].last_down_x = event->tfinger.x;
		finger[port][i].last_down_y = event->tfinger.y;
		finger[port][i].last_x = x;
		finger[port][i].last_y = y;
		break;
	}
}

static void preprocess_finger_up(SDL_Event *event)
{
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int num_fingers_down = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (finger[port][i].id >= 0) {
			num_fingers_down++;
		}
	}

	int x = mouse_x;
	int y = mouse_y;


	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (finger[port][i].id != id) {
			continue;
		}

		finger[port][i].id = NO_TOUCH;
		if (!multi_finger_dragging[port]) {
			if ((event->tfinger.timestamp - finger[port][i].time_last_down) > MAX_TAP_TIME) {
				continue;
			}

			// short (<MAX_TAP_TIME ms) tap is interpreted as right/left mouse click depending on # fingers already down
			// but only if the finger hasn't moved since it was pressed down by more than MAX_TAP_MOTION_DISTANCE pixels
			float xrel = ((event->tfinger.x * DISPLAY_WIDTH) - (finger[port][i].last_down_x * DISPLAY_WIDTH));
			float yrel = ((event->tfinger.y * DISPLAY_HEIGHT) - (finger[port][i].last_down_y * DISPLAY_HEIGHT));
			float max_r_squared = (float) (MAX_TAP_MOTION_DISTANCE * MAX_TAP_MOTION_DISTANCE);
			if ((xrel * xrel + yrel * yrel) >= max_r_squared) {
				continue;
			}

			if (num_fingers_down != 2 && num_fingers_down != 1) {
				continue;
			}

			Uint8 simulated_button = 0;
			if (num_fingers_down == 2) {
				simulated_button = SDL_BUTTON_RIGHT;
				// need to raise the button later
				simulated_click_start_time[port][1] = event->tfinger.timestamp;
			} else if (num_fingers_down == 1) {
				simulated_button = SDL_BUTTON_LEFT;
				// need to raise the button later
				simulated_click_start_time[port][0] = event->tfinger.timestamp;
				if (direct_touch) {
					convert_touch_xy_to_game_xy(event->tfinger.x, event->tfinger.y, &x, &y);
				}
			}
			set_mouse_button_event(event, SDL_MOUSEBUTTONDOWN, simulated_button, x, y);
		} else if (num_fingers_down == 1) {
			// when dragging, and the last finger is lifted, the drag is over
			Uint8 simulated_button = 0;
			if (multi_finger_dragging[port] == DRAG_THREE_FINGER) {
				simulated_button = SDL_BUTTON_RIGHT;
			} else {
				simulated_button = SDL_BUTTON_LEFT;
			}
			set_mouse_button_event(event, SDL_MOUSEBUTTONUP, simulated_button, x, y);
			multi_finger_dragging[port] = DRAG_NONE;
		}
	}
}

static void preprocess_finger_motion(SDL_Event *event)
{
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int num_fingers_down = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (finger[port][i].id >= 0) {
			num_fingers_down++;
		}
	}

	if (num_fingers_down == 0) {
		return;
	}

	if (num_fingers_down >= 1) {
		int x = mouse_x;
		int y = mouse_y;
		int xrel = 0;
		int yrel = 0;

		if (direct_touch) {
			convert_touch_xy_to_game_xy(event->tfinger.x, event->tfinger.y, &x, &y);
		} else {
			// for relative mode, use the pointer speed setting
			float speedFactor = 1.0;

			// convert touch events to relative mouse pointer events
			// Whenever an SDL_event involving the mouse is processed,
			x = (mouse_x + (event->tfinger.dx * 1.25 * speedFactor * DISPLAY_WIDTH));
			y = (mouse_y + (event->tfinger.dy * 1.25 * speedFactor * DISPLAY_HEIGHT));
		}

		x = CLIP(x, 0, (int)DISPLAY_WIDTH);
		y = CLIP(y, 0, (int)DISPLAY_HEIGHT);
		xrel = x - mouse_x;
		yrel = y - mouse_y;

		// update the current finger's coordinates so we can track it later
		for (int i = 0; i < MAX_NUM_FINGERS; i++) {
			if (finger[port][i].id != id)
			continue;
			finger[port][i].last_x = x;
			finger[port][i].last_y = y;
		}

		// If we are starting a multi-finger drag, start holding down the mouse button
		if (num_fingers_down >= 2 && !multi_finger_dragging[port]) {
			// only start a multi-finger drag if at least two fingers have been down long enough
			int num_fingers_downlong = 0;
			for (int i = 0; i < MAX_NUM_FINGERS; i++) {
				if (finger[port][i].id == NO_TOUCH) {
					continue;
				}
				if (event->tfinger.timestamp - finger[port][i].time_last_down > MAX_TAP_TIME) {
					num_fingers_downlong++;
				}
			}
			if (num_fingers_downlong >= 2) {
				int mouse_down_x = mouse_x;
				int mouse_down_y = mouse_y;
				if (direct_touch) {
					for (int i = 0; i < MAX_NUM_FINGERS; i++) {
						if (finger[port][i].id == id) {
							uint32_t earliest_time = finger[port][i].time_last_down;
							for (int j = 0; j < MAX_NUM_FINGERS; j++) {
								if (finger[port][j].id >= 0 && (i != j) ) {
									if (finger[port][j].time_last_down < earliest_time) {
										mouse_down_x = finger[port][j].last_x;
										mouse_down_y = finger[port][j].last_y;
										earliest_time = finger[port][j].time_last_down;
									}
								}
							}
							break;
						}
					}
				}

				Uint8 simulated_button = 0;
				if (num_fingers_downlong == 2) {
					simulated_button = SDL_BUTTON_LEFT;
					multi_finger_dragging[port] = DRAG_TWO_FINGER;
				} else {
					simulated_button = SDL_BUTTON_RIGHT;
					multi_finger_dragging[port] = DRAG_THREE_FINGER;
				}
				SDL_Event ev;
				set_mouse_button_event(&ev, SDL_MOUSEBUTTONDOWN, simulated_button, mouse_down_x, mouse_down_y);
				SDL_PushEvent(&ev);
			}
		}

		if (!xrel && !yrel) {
			return;
		}

		// check if this is the "oldest" finger down (or the only finger down)
		// otherwise it will not affect mouse motion
		bool update_pointer = true;
		if (num_fingers_down > 1) {
			for (int i = 0; i < MAX_NUM_FINGERS; i++) {
				if (finger[port][i].id != id) {
					continue;
				}
				for (int j = 0; j < MAX_NUM_FINGERS; j++) {
					if (finger[port][j].id == NO_TOUCH || (j == i)) {
						continue;
					}
					if (finger[port][j].time_last_down < finger[port][i].time_last_down) {
						update_pointer = false;
					}
				}
			}
		}
		if (!update_pointer) {
			return;
		}
		set_mouse_motion_event(event, x, y, xrel, yrel);
	}
}

void finish_simulated_mouse_clicks(int current_mouse_x, int current_mouse_y)
{
	mouse_x = current_mouse_x;
	mouse_y = current_mouse_y;

	for (int port = 0; port < TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < 2; i++) {
			if (simulated_click_start_time[port][i] == 0) {
				continue;
			}

			Uint32 current_time = SDL_GetTicks();
			if (current_time - simulated_click_start_time[port][i] < SIMULATED_CLICK_DURATION) {
				continue;
			}

			int simulated_button;
			if (i == 0) {
				simulated_button = SDL_BUTTON_LEFT;
			} else {
				simulated_button = SDL_BUTTON_RIGHT;
			}
			SDL_Event ev;
			set_mouse_button_event(&ev, SDL_MOUSEBUTTONUP, simulated_button, mouse_x, mouse_y);
			SDL_PushEvent(&ev);

			simulated_click_start_time[port][i] = 0;
		}
	}
}

static void set_mouse_button_event(SDL_Event *event, uint32_t type, uint8_t button, int32_t x, int32_t y)
{
	event->type = type;
	event->button.button = button;
	if (type == SDL_MOUSEBUTTONDOWN) {
		event->button.state = SDL_PRESSED;
	} else {
		event->button.state = SDL_RELEASED;
	}
	event->button.x = x;
	event->button.y = y;
}

static void set_mouse_motion_event(SDL_Event *event, int32_t x, int32_t y, int32_t xrel, int32_t yrel)
{
	event->type = SDL_MOUSEMOTION;
	event->motion.x = x;
	event->motion.y = y;
	event->motion.xrel = xrel;
	event->motion.yrel = yrel;
}

static void convert_touch_xy_to_game_xy(float touch_x, float touch_y, int *game_x, int *game_y) {
	const int screen_h = GAME_HEIGHT;
	const int screen_w = GAME_WIDTH;
	const int disp_w = DISPLAY_WIDTH;
	const int disp_h = DISPLAY_HEIGHT;

	int x, y, w, h;
	float sx, sy;

	h = disp_h;
	w = h * 16.0/9.0;

	x = (disp_w - w) / 2;
	y = (disp_h - h) / 2;

	sy = (float) h / (float) screen_h;
	sx = (float) w / (float) screen_w;

	// Find touch coordinates in terms of screen pixels
	float disp_touch_x = (touch_x * (float) disp_w);
	float disp_touch_y = (touch_y * (float) disp_h);

	*game_x = CLIP((int)((disp_touch_x - x) / sx), 0, (int) GAME_WIDTH);
	*game_y = CLIP((int)((disp_touch_y - y) / sy), 0, (int) GAME_HEIGHT);
}
