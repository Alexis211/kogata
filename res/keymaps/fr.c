#include <stdio.h>

#include <keymap_file.h>

keymap_t fr_keymap = {
	{		// normal
	/* 0x00 */	0, 0, '&', L'é', '"', '\'', '(', '-', L'è', '_', L'ç', L'à', ')', '=', 0, 0, 
	/* 0x10 */	'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', 0, 0, 'q', 's',
	/* 0x20 */	'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', L'ù', L'²', 0, '*', 'w', 'x', 'c', 'v',
	/* 0x30 */	'b', 'n', ',', ';', ':', '!', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 
	/* 0x50 */  0, 0, 0, 0, 0, 0, '<', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{		// shift
	/* 0x00 */	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', L'°', '+', 0, 0, 
	/* 0x10 */	'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', L'¨', L'£', 0, 0, 'Q', 'S',
	/* 0x20 */	'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', '~', 0, L'µ', 'W', 'X', 'C', 'V',
	/* 0x30 */	'B', 'N', '?', '.', '/', L'§', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
	/* 0x50 */  0, 0, 0, 0, 0, 0, '>', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{		// caps
	/* 0x00 */	0, 0, '&', L'É', '"', '\'', '(', '-', L'È', '_', L'Ç', L'À', ')', '=', 0, 0, 
	/* 0x10 */	'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '^', '$', 0, 0, 'Q', 'S',
	/* 0x20 */	'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', L'Ù', L'²', 0, '*', 'W', 'X', 'C', 'V',
	/* 0x30 */	'B', 'N', ',', ';', ':', '!', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
	/* 0x50 */  0, 0, 0, 0, 0, 0, '<', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{		// shift caps
	/* 0x00 */	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', L'°', '+', 0, 0, 
	/* 0x10 */	'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', L'¨', L'£', 0, 0, 'q', 's',
	/* 0x20 */	'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', '%', L'³', 0, L'µ', 'w', 'x', 'c', 'v',
	/* 0x30 */	'b', 'n', '?', '.', '/', L'§', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 
	/* 0x50 */  0, 0, 0, 0, 0, 0, '>', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{		// alt gr
	/* 0x00 */	0, 0, L'¹', '~', '#', '{', '[', '|', '`', '\\', '^', '@', ']', '}', 0, 0, 
	/* 0x10 */	L'æ', L'«', L'€', L'¶', L'ŧ', L'←', L'↓', L'→', 
				L'ø', L'þ', L'¨', L'¤', 0, 0, '@', L'ß',
	/* 0x20 */	L'ð', L'đ', L'ŋ', L'ħ', 'j', L'ĸ', L'ł', L'µ',
			 	'^', L'¬', 0, '`', L'ł', L'»', L'¢', L'“',
	/* 0x30 */	L'”', 'n', L'´', ';', L'·', '!', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
	/* 0x50 */  0, 0, 0, 0, 0, 0, '|', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{		// shift + alt gr
	/* 0x00 */	0, 0, L'¡', L'⅛', L'£', '$', L'⅜', L'⅝', L'⅞', L'™', L'±', L'°', L'¿', L'˛', 0, 0, 
	/* 0x10 */	L'Æ', '<', L'¢', L'®', L'Ŧ', L'¥', L'↑', L'ı', L'Ø', L'Þ', L'°', L'¯', 0, 0, L'Ω', L'§',
	/* 0x20 */	L'Ð', L'ª', L'Ŋ', L'Ħ', 'J', '&', L'Ł', L'º', L'ˇ', L'¬', 0, L'˘', L'Ł', '>', L'©', L'‘',
	/* 0x30 */	L'’', 'N', L'˝', L'×', L'÷', L'˙', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	/* 0x40 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
	/* 0x50 */  0, 0, 0, 0, 0, 0, L'¦', 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	},
	true,
};


int main() {
	fwrite(&fr_keymap, 1, sizeof(fr_keymap), stdout);

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
