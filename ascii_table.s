section .data
	global ascii_table_babel ; globally accessible ascii table

	decimal_ascii_table_babel db 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	decimal_ascii_table_babel db 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	decimal_ascii_table_babel db ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+'
	decimal_ascii_table_babel db ',', '-', '.', '/','0', '1', '2', '3', '4', '5', '6', '7'	
	decimal_ascii_table_babel db '8', '9', ':', ';', '<', '=', '>', '?','@', 'A', 'B', 'C'
	decimal_ascii_table_babel db 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'
   	decimal_ascii_table_babel db 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '['
	decimal_ascii_table_babel db '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g'
	decimal_ascii_table_babel db 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's' 
	decimal_ascii_table_babel db 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127
