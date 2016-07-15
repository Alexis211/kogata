#pragma once

#include <proto/fb.h>

#include <kogata/syscall.h>

/*
	Definition of the GIP protocol (Graphics Initiation Protocol).
	This is a protocol meant for communication on a local machine between
	a process providing a graphics window (screen & input) and a client
	process that does user interaction.

	Examples :
	- giosrv makes the hardware available as a GIP server
	- the login manager is a GIP client of giosrv
	- the user window manager is a GIP client of the login manager
	  (when a user session is active, the login manager is mostly a GIP proxy,
	  but it intercepts the Ctrl+Alt+Del keyboard shortcut to terminate/lock
	  the active user session)
	- each window managed by the WM acts as a GIP server.
	- the GUI library basically knows how to be a GIP client that does actually
	  usefull stuff (displaying a gui)

	Features of the GIP protocol for OUTPUT :
	- as an option, mode setting (for when the buffer is a display)
	- as an option, buffer resizing (for when the buffer is a window)
		TODO: interacts somehow with WMP (window managing protocol)
	- server-side allocation of a framebuffer
	- as an option, double buffering (server allocates two buffers,
	  client tells server to switch buffers)
	- "buffer damage" notification from clients on active buffer,
	  (not necessary, and disabled, when the buffer is actually the
	  display output itself)
	  (a buffer switch request implies a global buffer damage notification)
	
	Features of the GIP protocol for KEYBOARD INPUT :
	- raw keycode notification from server on keypress/keyrelease

	Features of the GIP protocol for MOUSE INPUT :
	- as an option, raw mouse data input
	- as an option, mouse data parsing & notification in terms
	  of (mouse_x, mouse_y)
	- as an option, cursor display handling (server mouse data
	  parsing must be enabled)

	The active/inactive features are defined on a per-GIP-channel basis.

	Typical GIP session :
	- C: RESET
	- S: INTIATE (available features)
	- S: BUFFER_INFO (base buffer)
	- C: ENABLE_FEATURES (desired features)
	- S: OK
	- S: BUFFER_INFO (double buffering is enabled! new buffers needed)
	- C: draws on buffer 1
	- C: SWITCH_BUFFER 1	
	- S does not answer to SWITCH_BUFFER messages (they happen too often!)
	- C: draws on buffer 0
	- C: SWITCH_BUFFER 0
	- ...
 */

// GIP features
#define GIPF_DOUBLE_BUFFER		0x01
#define GIPF_DAMAGE_NOTIF		0x02
#define GIPF_MODESET			0x04
// #define GIP_F_RESIZING			0x08		// TODO semantics for this shit
#define GIPF_MOUSE_XY			0x10
#define GIPF_MOUSE_CURSOR		0x20

// GIP message IDs
//  ---- GIPC : commands, expect a reply
//  ---- GIPR : reply to a command
//  ---- GIPN : notification, no reply expected
#define GIPC_RESET				0		// client: plz open new session
#define GIPR_INITIATE			1		// server: ok, here is supported feature list
#define GIPR_OK					2
#define GIPR_FAILURE			3
#define GIPC_ENABLE_FEATURES	4
#define GIPC_DISABLE_FEATURES	5

#define GIPN_BUFFER_INFO		10		// server: buffer #i is at #token and has #geom
#define GIPC_QUERY_MODE			11		// client: what about mode #i?
#define GIPR_MODE_INFO			12		// server: mode #i is xxyy
#define GIPC_SET_MODE			13		// client: please switch to mode #i
#define GIPN_BUFFER_DAMAGE		14		// client: please update region
#define GIPC_SWITCH_BUFFER		15		// client: please switch to buffer b (0 or 1)

#define GIPN_KEY_DOWN			20		// server: key k down
#define GIPN_KEY_UP				21		// server: key k up

#define	GIPN_MOUSE_DATA			30		// server: raw mouse data
#define GIPN_MOUSE_XY			31		// server: mouse moved at xy ; client: put mouse at xy
#define GIPN_MOUSE_PRESSED		32		// server: button b pressed
#define GIPN_MOUSE_RELEASED		33		// server: button b released
#define GIPC_LOAD_CURSOR			34		// client: this is graphics for cursor #i
#define GIPC_SET_CURSOR			35		// client: please use cursor #i (0 = hide cursor)

typedef struct {
	uint32_t code;		// message type
	uint32_t req_id;	// for reply messages, code of the reply
	uint32_t arg;
} gip_msg_header;

typedef struct {
	token_t tok;
	fb_info_t geom;
} gip_buffer_info_msg;

typedef struct {
	fb_info_t geom;
} gip_mode_info_msg;

typedef struct {
	fb_region_t region;
} gip_buffer_damage_msg;

/* vim: set ts=4 sw=4 tw=0 noet :*/
