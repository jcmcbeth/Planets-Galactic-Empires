/**
 * String handling and utility functions.
 *
 * @author  Joel McBeth
 * @author  Various others
 *
 * @version 1.0     02/12/04
 */

#include <ctype.h>
#include <string.h>
#include "mud.h"

/**
 * Creates a duplicate string.
 *
 * @author  Thoric
 * @version 1.0
 *
 * @param   str     The string to duplicate.
 *
 * @returns A duplicate string.
 */
char *str_dup( char const *str )
{
    static char *ret;
    int len;

    if ( !str )
	return NULL;
    
    len = strlen(str)+1;

    CREATE( ret, char, len );
    strcpy( ret, str );
    return ret;
}

/**
 * Replaces all occurances of a character in a string.
 *
 * @author  Joel McBeth
 * @version 1.0     02/12/04
 *
 * @param   str     The string that will be used to replace the characters.
 * @param   c       The character that will be replaced.
 * @param   rep     The character the will replace the other character.
 *
 * @returns The string with the characters replaced.
 */
char* str_rep(const char* str, char c, char rep)
{
    char* new_str;
    int length;

    if (str == NULL)
        return NULL;

    length = strlen(str);
    CREATE(new_str, char, length + 1);    

    for (; *str != '\0'; str++)
    {
        if (*str == c)
            *new_str = rep;
        else
            *new_str = *str;
    }

    return new_str;
}

/**
 * Finds the index of the first occurance of a given substring starting at a specified index.
 *
 * @author  Joel McBeth
 * @version 1.0         02/13/05
 *
 * @param   str         The string to search.
 * @param   find_str    The string to search for.
 * @param   start       The index to start searching at.
 *
 * @returns The index of the substring or -1 if it was not found.
 */
int str_str(const char* str, const char* find_str, int start = 0)
{
    int str_len, find_len;
    int i, j;
    int search_to;
    bool found;

    if ((str == NULL) || (find_str == NULL))
        return -1;

    str_len = strlen(str);
    find_len = strlen(find);

    /* If the starting index is less then 0 or greater than the
     * length then it wont be found.
     */
    if ((start < 0) || (start > str_len))
        return -1;

    /* It can't possibly be in the string if it is longer then it. */
    if (str_len < find_len)
        return -1;

    /* No need to check what is left if it would even be the length
     * of the substring.
     */
    search_to = (str_len - find_len);
        
    for (i = start; i <= search_to; i++)
    {
        found = TRUE;
        for (j = 0; j < find_len; j++)
        {            
            if (str[i + j] != find_str[j])
                found = FALSE;
        }

        if (found == TRUE)
            return i;
    }

    return -1;
}

/**
 * Counts the occurances of a substring in a string.
 *
 * @author  Joel McBeth
 * @version 1.0         02/13/05
 *
 * @param   str         The string to search.
 * @param   find_str    The string to search for.
 *
 * @returns The number of times the substring occurs in the string.
 */
int str_strn(const char* str, const char* find_str)
{
    int i = 0;
    int c;    

    if ((str == NULL) || (find_str == NULL))
        return 0;    

    c = 0;

    while (*str != '\0')
    {
        /* Find the index of the string. */
        i = str_str(str, find_str);

        /* If it isn't found then were are done. */
        if (i == -1)
            break;
        else
            c++;

        /* Move the string to one index past the last place the substring was found
         * so it will search the rest of the string. */
        str += i + 1;        
    }
 
    return c;
}

/**
 * Replaces all occurances of a substring with another substring. 
 *
 * @author  Joel McBeth
 * @version 1.0         02/13/05
 *
 * @param   str         The string that will have the substrings replaced.
 * @param   find_str    The string that will be replaced.
 * @param   rep_str     The string the will replace the found substring.
 *
 * @returns The string with the substring replaced, or str if any of the arguments are NULL.
 */
char* str_strrep(const char* str, const char* find_str, const char* rep_str)
{    
    char* new_str, ref_str;
    int count;    
    int str_len, find_len, rep_len, new_len;
    int len_delta;
    int i, j;
    int pos;

    if ((str == NULL) || (find_str == NULL) || (rep_str == NULL))
        return str;

    count = str_strn(str, find_str);    

    str_len = strlen(str);
    find_len = strlen(find_str);
    rep_len = strlen(rep_str);

    /* Find the difference in the length to calculate the new length. */
    len_delta = find_len - rep_len;

    new_len = str_len + (len_delta * count) + 1;

    CREATE(new_str, char, new_len);

    ref_str = new_str;
        
    while (i < str_len)
    {
        /* Find the next index of the substring. */
        pos = str_str(str, find_str, pos);
        
        /* Copy all the characters up to the substring to the new string. */
        while (i < pos)
            *(ref_str++) = str[i++];

        /* Copy the replacement string into the new string. */
        for (j = pos; j < rep_len; j++)                
            *(ref_str++) = rep_str[j];                    
 
        /* We don't want to copy any of those characters. */
        i += find_len;

        /* Increment the pos so we don't just keep finding the same substring over. */
        pos++;
    }

    return new_str;
}

/**
 * Checks if a string is a number.
 *
 * @author  Unknown
 * @version 1.0     02/12/05
 *
 * @param   str     The string to check.
 *
 * @returns TRUE if it is a number, FALSE if it is not.
 */
bool is_number(const char* str)
{
    if ( *str == '\0' )
	return FALSE;

    for ( ; *str != '\0'; str++ )
    {
	if ( !isdigit(*str) )
	    return FALSE;
    }

    return TRUE;
}

/**
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 *
 * @author  Unknown
 * @version 1.0         02/12/05
 *
 * @param   argument    The string to take the argument from.
 * @param   arg_first   Where the first argument will be stored.
 * 
 * @returns The string with the first argument removed.
 */
char* one_argument(char* argument, char* arg_first)
{
    char cEnd;
    sh_int count;

    count = 0;

    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' || ++count >= 255 )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*arg_first = LOWER(*argument);
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( isspace(*argument) )
	argument++;

    return argument;
}

/**
 * Pick off one argument from a string and return the rest.
 * Understands quotes.  Delimiters = { ' ', '-' }
 *
 * @author  Unknown
 * @version 1.0         02/12/04
 *
 * @param   argument    The string to take the argument from.
 * @param   arg_first   Where the first argument will be stored.
 * 
 * @returns The string with the first argument removed.
 */
char* one_argument2( char *argument, char *arg_first )
{
    char cEnd;
    sh_int count;

    count = 0;

    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' || ++count >= 255 )
    {
	if ( *argument == cEnd || *argument == '-' )
	{
	    argument++;
	    break;
	}
	*arg_first = LOWER(*argument);
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( isspace(*argument) )
	argument++;

    return argument;
}

/**
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 *
 * @author  Unknown
 * @version 1.0     02/13/05
 *
 * @param   str     The string that will have it's tildes replaced.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
	if ( *str == '~' )
	    *str = '-';    
}

/**
 * Encodes the tildes in a string.
 * Used for player-entered strings that go into disk files.
 *
 * @author  Thoric
 * @version 1.0     02/13/05
 *
 * @param   str     The string that will have it's tildes encoded.
 *
 * @returns A string the the tildes encoded.
 */
void hide_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
	if ( *str == '~' )
	    *str = HIDDEN_TILDE;    
}

/**
 * It decodes encoded tildes in a string.
 *
 * @author  Unknown
 * @version 1.0     02/13/05
 *
 * @param   str     The string to decode.
 *
 * @returns A string with the tildes decoded.
 */
char *show_tilde( char *str )
{
    static char buf[MAX_STRING_LENGTH];
    char *bufptr;

    bufptr = buf;
    for ( ; *str != '\0'; str++, bufptr++ )
    {
	if ( *str == HIDDEN_TILDE )
	    *bufptr = '~';
	else
	    *bufptr = *str;
    }
    *bufptr = '\0';

    return buf;
}

/**
 * Compare strings, case insensitive.
 *
 * @author  Unknown
 * @version 1.0     02/13/05
 *
 * @param   astr    The first string to compare.
 * @param   bstr    The string to compare to the first.
 *
 * @returns TRUE If different (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
    if ( !astr )
    {
	bug( "Str_cmp: null astr." );
	if ( bstr )
	  fprintf( stderr, "str_cmp: astr: (null)  bstr: %s\n", bstr );
	return TRUE;
    }

    if ( !bstr )
    {
	bug( "Str_cmp: null bstr." );
	if ( astr )
	  fprintf( stderr, "str_cmp: astr: %s  bstr: (null)\n", astr );
	return TRUE;
    }

    for ( ; *astr || *bstr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}



/**
 * Compares two case insensitive strings to see if the second starts the first. 
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   astr    The string to see if it has a prefix.
 * @param   bstr    The string to check if it begins the first string.
 *
 * @returns TRUE if the second string does not begin the first string, otherwise FALSE. 
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( !astr )
    {
	bug( "Strn_cmp: null astr." );
	return TRUE;
    }

    if ( !bstr )
    {
	bug( "Strn_cmp: null bstr." );
	return TRUE;
    }

    for ( ; *astr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}



/**
/**
 * Checks if a string contains a case insensitive substring.
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   astr    The string to see if it contains a substring.
 * @param   bstr    The substring to search for.
 *
 * @returns TRUE if the second string does not contain the substring, otherwise FALSE.
 */
bool str_infix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
	return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
	if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
	    return FALSE;

    return TRUE;
}



/**
 * Checks if a string contains a case insensitive substring at the end.
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   astr    The string to search.
 * @param   bstr    The substring to search for.
 *
 * @returns TRUE the substring does not end the string, otherwise FALSE.
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
	return FALSE;
    else
	return TRUE;
}



/**
 * Makes a string proper caps where only the first character is capitolized.
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   str     The string to make proper case.
 *
 * @returns The string in proper case.
 */
char *capitalize( const char *str )
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
	strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}


/**
 * Gets a string in lowercase.
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   str     The string to get the lowercase of.
 *
 * @returns The string in lowercase.
 */
char *strlower( const char *str )
{
    static char strlow[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
	strlow[i] = LOWER(str[i]);
    strlow[i] = '\0';
    return strlow;
}

/**
 * Gets a string in uppercase.
 *
 * @author  Unknown
 * @version 1.0     13/15/05
 *
 * @param   str     The string to get the uppercase of.
 *
 * @returns The string in uppercase.
 */
char *strupper( const char *str )
{
    static char strup[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
	strup[i] = UPPER(str[i]);
    strup[i] = '\0';
    return strup;
}

/**
 * Checks if a character is a vowel.
 *
 * @author  Thoric
 * @version 1.0     13/15/05
 *
 * @param   letter  The character to check. 
 *
 * @returns TRUE if it is a vowel, false otherwise.
 */
bool isavowel( char letter )
{
    char c;

    c = tolower( letter );
    if ( c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' )
      return TRUE;
    else
      return FALSE;
}

/**
 * Checks if a or an should be at the beginning of the string.
 * Even takes into consideration where 'y' is considered a vowel.
 *
 * @author  Thoric
 * @version 1.0     13/15/05
 *
 * @param   str     The string to see if it has a prefix.
 *
 * @returns The string with a or an at the beginning.
 */
char *aoran( const char *str )
{
    static char temp[MAX_STRING_LENGTH];

    if ( !str )
    {
	bug( "Aoran(): NULL str" );
	return "";
    }

    /* Isn't english wonderful? :-P */
    if ( isavowel(str[0])
    || ( strlen(str) > 1 && tolower(str[0]) == 'y' && !isavowel(str[1])) )
      strcpy( temp, "an " );
    else
      strcpy( temp, "a " );
    strcat( temp, str );
    return temp;
}

/**
 * Given a string like 14.foo, return 14 and 'foo'.
 *
 * @author  Unknown
 * @version 1.0         13/15/05
 *
 * @param   argument    The argument that will have the number parsed.
 * @param   arg         Where the argument without the number will be stored.
 *
 * @returns The number that was parsed from the argument.
 */
int number_argument( char *argument, char *arg )
{
    char *pdot;
    int number;

    for ( pdot = argument; *pdot != '\0'; pdot++ )
    {
	if ( *pdot == '.' )
	{
	    *pdot = '\0';
	    number = atoi( argument );
	    *pdot = '.';
	    strcpy( arg, pdot+1 );
	    return number;
	}
    }

    strcpy( arg, argument );
    return 1;
}

/**
 * See if a string is in a space delimited string list.
 *
 * @author  Unknown
 * @version 1.0         02/12/05
 *
 * @param   str         The string to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string is in the string list.
 */
bool is_name( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
	namelist = one_argument( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_cmp( str, name ) )
	    return TRUE;
    }
}

/**
 * See if a string prefix is in a space delimited string list.
 *
 * @author  Unknown
 * @version 1.0         02/12/05
 *
 * @param   str         The string prefix to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string is in the string list.
 */
bool is_name_prefix( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
	namelist = one_argument( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_prefix( str, name ) )
	    return TRUE;
    }
}

/**
 * See if a string occurs in a space/dash delimited string list.
 *
 * @author  Thoric
 * @version 1.0         02/12/05
 *
 * @param   str         The string to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string is in the string list.
 */
bool is_name2( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
	namelist = one_argument2( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_cmp( str, name ) )
	    return TRUE;
    }
}

/**
 * See if a string prefix occurs in s space/dash delimited string list.
 *
 * @author  Unknown
 * @version 1.0         02/12/05
 *
 * @param   str         The string prefix to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string prefix is in the string list.
 */
bool is_name2_prefix( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
	namelist = one_argument2( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_prefix( str, name ) )
	    return TRUE;
    }
}

/*								-Thoric
 * Checks if str is a name in the namelist supporting multiple keywords
 */

/**
 * Checks if a string occurs in a space/dash delimited string list supporting multiple keywords.
 *
 * @author  Thoric
 * @version 1.0         02/12/05
 *
 * @param   str         The string to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string is in the string list.
 */
bool nifty_is_name( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];
    
    if ( !str || str[0] == '\0' )
      return FALSE;
 
    for ( ; ; )
    {
	str = one_argument2( str, name );
	if ( name[0] == '\0' )
	    return TRUE;
	if ( !is_name2( name, namelist ) )
	    return FALSE;
    }
}


/**
 * See if a string prefix occurs in a dash/space delimited string list.
 *
 * @author  Unknown
 * @version 1.0         02/12/05
 *
 * @param   str         The string prefix to search for.
 * @param   namelist    The string list.
 *
 * @returns If the string prefix is in the string list.
 */
bool nifty_is_name_prefix( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];
    
    if ( !str || str[0] == '\0' )
      return FALSE;
 
    for ( ; ; )
    {
	str = one_argument2( str, name );
	if ( name[0] == '\0' )
	    return TRUE;
	if ( !is_name2_prefix( name, namelist ) )
	    return FALSE;
    }
}

/**
 * Tells if two hostnames are close enough to be considered the same.
 * This is to compare two hostnames like 255-255-255-32.asdf.fdsa.net and
 * 255-255-255-76.asdf.fdsa.net.
 *
 * @author  Joel McBeth
 * @version 1.0     02/09/05
 *
 * @param   host1   The first host to compare if it is equal to the second.
 * @param   host2   The second host to compare if it is equal to the first.
 *
 * @returns TRUE if the host should be consdiered the same, FALSE otherwise.
 */
bool same_host(char* host1, char* host2)
{
    char* stra, strb;

    if ((host1 == NULL) || (host2 == NULL))
    {
        bug("same_host: host1 or host2 is NULL", 0);
        return FALSE;
    }

    /* Start comparing the strings after the first '.' */

    if ((stra = strchr(host1, '.')) == NULL)
    {
        bug("same_host: host1 doesn't contain a .");
        return FALSE;
    }

    if ((strb = strch(host2, ".")) == NULL)
    {
        bug("same_host: host2 doesn't contain a .");
        return FALSE;
    }

    /* If they are different lengths they are not the same. */
    if (strlen(stra) != strlen(strb))
    {
        return FALSE;
    }

    /* Check if they are the same. */
    if (str_cmp(stra, strb) == FALSE)
    {
        return TRUE;
    }

    return FALSE;
}