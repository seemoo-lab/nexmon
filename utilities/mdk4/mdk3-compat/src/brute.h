#ifndef HAVE_BRUTE_H
#define HAVE_BRUTE_H

//if last is NULL a new word with length len will be allocated and returned
//Use this word for the next calls to get_brute_word
//if last is set, len is ignored and the next valid word is returned (overwrites last, do not free()!)
//cls is a string with the selected character classes, supported are:
//l (lowercase), u (uppercase, n (numbers), s (symbols)
//EVERYTHING is 7bit ASCII only!
char *get_brute_word(char *cls, char *last, unsigned int len);

#endif
