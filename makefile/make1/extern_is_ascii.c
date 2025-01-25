int is_ascii(char c) {
	return c == '\n' || (c > 31 && c < 127);
}
