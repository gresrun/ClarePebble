#include <pebble.h>

// Static Variables
static Window *main_window;
static MenuLayer *scene_list_layer;
static char **scenes;
static size_t num_scenes;

// Enums
enum MessageKey {
	KEY_MSG_TYPE  = 0x00,
	KEY_LIST_SIZE = 0x01
};

enum MessageType {
	MSG_RQST_SCENES = 0x00,
	MSG_RESP_SCENES = 0x01
};

// AppMessage
//// Message Type Senders
static void request_scenes_msg(void) {
	Tuplet type_tuple = TupletInteger(KEY_MSG_TYPE, MSG_RQST_SCENES);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter != NULL) {
		dict_write_tuplet(iter, &type_tuple);
		dict_write_end(iter);
	    app_message_outbox_send();
    }
}

//// Message Type Handlers
static void handle_scene_response(DictionaryIterator *iter) {
	Tuple *size_tuple = dict_find(iter, KEY_LIST_SIZE);
	size_t old_num_scenes = num_scenes;
	num_scenes = size_tuple->value->int32;
	char **scenesTmp = malloc(num_scenes * sizeof(char*));
	size_t i = 0;
	Tuple *tuple = dict_read_first(iter);
	while (tuple) {
		switch (tuple->key) {
			case KEY_LIST_SIZE: // Falling through deliberately
			case KEY_MSG_TYPE:
				// Ignore
				break;
			default:
				scenesTmp[i] = malloc(tuple->length);
				strncpy(scenesTmp[i], tuple->value->cstring, tuple->length);
				i++;
				break;
		}
		tuple = dict_read_next(iter);
	}
	if (scenes != NULL) {
		for (i = 0; i < old_num_scenes; i++) {
			free(scenes[i]);
		}
		free(scenes);
	}
	scenes = scenesTmp;
}

//// AppMessage Handlers
void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *type_tuple = dict_find(iter, KEY_MSG_TYPE);
	if (type_tuple) {
		switch(type_tuple->value->int32) {
			case MSG_RESP_SCENES:
				handle_scene_response(iter);
				break;
			default:
				break;
		}
	} else {
		APP_LOG(APP_LOG_LEVEL_WARNING, "Received App Message With No Type");
	}
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Was Delivered!");
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

//// Init
static void app_message_init(void) {
	// Register message handlers
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_register_outbox_sent(out_sent_handler);
	// Init buffers
	app_message_open(64, 64);
	request_scenes_msg();
}

// Callbacks
//// Service Type Callbacks
static uint16_t get_scene_list_rows(struct MenuLayer* menu_layer, uint16_t section_index, void *callback_context) {
	return 1;
}

static void draw_scene_list_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
//	PebbleFont *fonts = (PebbleFont*) callback_context;
//	PebbleFont *font = &fonts[cell_index->row];
	menu_cell_basic_draw(ctx, cell_layer, "foo", "bar", NULL);
}

static void select_scene_list_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
//	current_font = cell_index->row;
//	window_stack_push(font_window, true);
}

MenuLayerCallbacks scene_list_callbacks = {
	.get_num_rows = get_scene_list_rows,
	.draw_row = draw_scene_list_row,
	.select_click = select_scene_list_click
};

// UI
//// Main Window
static void main_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(main_window);
	scene_list_layer = menu_layer_create(layer_get_bounds(window_layer));
	menu_layer_set_callbacks(scene_list_layer, NULL, scene_list_callbacks);
	menu_layer_set_click_config_onto_window(scene_list_layer, main_window);
	layer_add_child(window_layer, menu_layer_get_layer(scene_list_layer));
}

static void main_window_unload(Window *window) {
	menu_layer_destroy(scene_list_layer);
}

// App Lifecycle
static void init(void) {
	main_window = window_create();
	app_message_init();
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});
	window_stack_push(main_window, true /*animated*/);
}

static void deinit(void) {
	window_destroy(main_window);
}

int main(void) {
	init();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed main window: %p", main_window);
	app_event_loop();
	deinit();
}
