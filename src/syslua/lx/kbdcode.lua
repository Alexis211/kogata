local kbdcode = {}

kbdcode.event = {
	KEYRELEASE		=	0,
	KEYPRESS		=	1,
}

kbdcode.code = {
	ESC =			1,
	RETURN = 		28,
	BKSP =			14,
	UP = 			200,
	DOWN =			208,
	LEFT =			203,
	RIGHT = 		205,
	HOME =			199,
	END =			207,
	PGUP =			201,
	PGDOWN = 		209,

	LSHIFT = 		42,
	RSHIFT = 		54,
	CAPSLOCK =		58,
	LCTRL = 		29,
	RCTRL = 		157,
	LALT =			56,
	RALT =			184,
	LSUPER = 		219,
	RSUPER = 		220,
	MENU =			221,
	TAB =			15,
	INS =			210,
	DEL =			211,

	F1 = 			59,
	F2 = 			60,
	F3 = 			61,
	F4 = 			62,
	F5 = 			63,
	F6 = 			64,
	F7 = 			65,
	F8 = 			66,
	F9 = 			67,
	F10 =			68,
	F11 =			87,
	F12 =			88,

	NUMLOCK =		69,
	SCRLLOCK =		70,
	PRTSCN = 		183,
	SYSREQ = 		84,

	KPHOME = 		71,
	KPUP =			72,
	KPPGUP = 		73,
	KPLEFT = 		75,
	KP5 =			76,
	KPRIGHT =		77,
	KPEND = 		79,
	KPDOWN = 		80,
	KPPGDOWN =		81,
	KPINS = 		82,
	KPDEL = 		83,
}

kbdcode.flags = {
	CHAR		=	0x01,
	ALT			=	0x02,
	CTRL		=	0x04,
	SUPER		=	0x08,
	SHIFT		=	0x10,
	CAPS		=	0x20,
	MOD			=	0x40,
}


return kbdcode
