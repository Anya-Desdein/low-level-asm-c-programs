section .data
    global ascii_table_babel ; globally accessible ascii table
    ; please note that some characters aren't printable

    decimal_ascii_table_babel db 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    db 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    db ' ', '!', '"', '#', '$', '%', '&', 39, '(', ')', '*', '+'
    db ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7'
    db '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A', 'B', 'C'
    db 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'
    db 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '['
    db 92, ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g'
    db 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's'
    db 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127
