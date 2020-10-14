/*
 * util.c - Miscellaneous utility fns
 */


/* Convert a character string representing a hex value to an int */
int xtoi(char *hexstring) {
	int	i = 0;
	
	if ((*hexstring == '0') && (*(hexstring+1) == 'x')) {
		  hexstring += 2;
	}

	while (*hexstring) {
		char c = toupper(*hexstring++);
		if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) {
			break;
		}
		c -= '0';
		if (c > 9) {
			c -= 7;
		}
		i = (i << 4) + c;
	}

	return i;
}

u32 rgb_color(int r, int g, int b) {
	return ((r << 24) | (g << 16) | (b << 8) | 255);
}

//asprintf definition deleted because we no longer need it
