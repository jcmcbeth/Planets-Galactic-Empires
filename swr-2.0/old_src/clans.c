#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"


#define MAX_NEST	100

CLAN_DATA * first_clan;
CLAN_DATA * last_clan;

/**
 * The string names of the flags for possible member powers.
 */
char* const power_flags[]  =  {  "withdraw", "buyship", "induct", "build", "empower",
                                 "pilot", "war", "wages"  };
/**
 * The string representations of the clan flags.
 */
char* const clan_flags[]   =  {  "hidden" };

/**
 * The string representations of the clan ranks.
 */
char* const rank_names[]   =  {  "member", "leader"   };

/* local routines */
void	fread_clan	args( ( CLAN_DATA *clan, FILE *fp ) );
bool	load_clan_file	args( ( char *clanfile ) );
void	write_clan_list	args( ( void ) );


/**
 * Gets the power flag value given the name of the flag.
 *
 * @author  Joel McBeth
 * @version 1.0     07/18/05 
 *
 * @param   name    The name of the flag.
 *
 * @returns The power flag value as a 64 bit integer or -1 if the flag was not found.
 */
long int get_powerflag(char* name)
{
   int i;

   for (i = 0; i < POWER_MAX_FLAGS; i++)
      if (str_cmp(name, power_flags[i]) == FALSE)
         return i;

   return -1;
}

/**
 * Gets the name of a power flag given the 64 bit integer value.
 *
 * @author  Joel McBeth
 * @version 1.0     07/18/05 
 *
 * @param   flag    The 64 bit integer value for the flag.
 *
 * @returns The name of the flag as a string.
 */
char* get_powerflag_name(long int flag)
{
   static char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];

   if ((flag < 0) || (flag >= POWER_MAX_FLAGS))
   {      
      sprintf(buf2, "get_powerflag_name: invalid flag, %ld", flag);
      bug(buf2, 0);

      return NULL;
   }

   strcpy(buf, power_flags[flag]);

   return buf;
}


/**
 * Gets a list of names of all the power flags.
 *
 * @author  Joel McBeth
 * @version 1.0     07/18/05 
 *
 * @returns The a string that contains a list of the names of the power flags.
 */
char* get_powerflags()
{
   static char buf[MAX_STRING_LENGTH];
   int i;

   /* Make it an empty string to avoid complications. */
   buf[0] = '\0';

   for (i = 0; i < POWER_MAX_FLAGS; i++)
   {
      strcat(buf, power_flags[i]);

      strcat(buf, ",");
   }

   /* Remove the comma on the end. */
   i = strlen(buf);
   buf[i - 1] = '\0';

   return buf;
}


/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( char *name )
{
    CLAN_DATA *clan;
    
    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_cmp( name, clan->name ) )
         return clan;

    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_prefix( name, clan->name ) )
         return clan;

    for ( clan = first_clan; clan; clan = clan->next )
       if ( nifty_is_name( name, clan->name ) )
         return clan;

    for ( clan = first_clan; clan; clan = clan->next )
       if ( nifty_is_name_prefix( name, clan->name ) )
         return clan;

    return NULL;
}

void write_clan_list( )
{
    CLAN_DATA *tclan;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s", CLAN_DIR, CLAN_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	    bug( "FATAL: cannot open clan.lst for writing!\n\r", 0 );
 	    return;
    }	  
    for ( tclan = first_clan; tclan; tclan = tclan->next )
	    fprintf( fpout, "%s\n", tclan->filename );

    fprintf( fpout, "$\n" );
    fclose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA *clan )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];
    MEMBER_DATA* member;

    if ( !clan )
    {
        bug( "save_clan: null clan pointer!", 0 );
        return;
    }
        
    if ( !clan->filename || clan->filename[0] == '\0' )
    {
        sprintf( buf, "save_clan: %s has no filename", clan->name );
        bug( buf, 0 );
        return;
    }

    sprintf( filename, "%s%s", CLAN_DIR, clan->filename );

    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
        bug( "save_clan: fopen", 0 );
        perror( filename );
    }
    else
    {
        fprintf( fp, "#CLAN\n" );
        fprintf( fp, "Name         %s~\n",	clan->name		);
        fprintf( fp, "Filename     %s~\n",	clan->filename		);
        fprintf( fp, "Description  %s~\n",	clan->description	);        
        fprintf( fp, "Atwar        %s~\n",	clan->atwar		);
        fprintf( fp, "Members      %d\n",		clan->members		);
        fprintf( fp, "Funds        %ld\n",	clan->funds		);
        fprintf( fp, "End\n\n"						);

        /* Write the members to file. */
        for (member = clan->first_member; member != NULL; member = member->next)
        {
            fprintf( fp, "#MEMBER\n" );
            fprintf( fp, "Name         %s~\n",  member->name         );
            fprintf( fp, "Rank         %d\n",   member->rank         );
            fprintf( fp, "LastPlayed   %d\n",   member->last_played  );
            fprintf( fp, "End\n\n");
        }

        fprintf( fp, "#END\n"						);
    }

    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

/*
 * Read in actual clan data.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_clan( CLAN_DATA *clan, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;

    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER(word[0]) )
        {
        case '*':
	        fMatch = TRUE;
	        fread_to_eol( fp );
	        break;

        case 'A':
	        KEY( "Atwar",	clan->atwar,	fread_string( fp ) );
	        break;

        case 'D':
	        KEY( "Description",	clan->description,	fread_string( fp ) );
	        break;

        case 'E':
	        if ( !str_cmp( word, "End" ) )
	        {
	            if (!clan->name)
	                clan->name		= STRALLOC( "" );
	            if (!clan->atwar)
	                clan->atwar		= STRALLOC( "" );
	            if (!clan->description)
	                clan->description 	= STRALLOC( "" );
	            return;
	        }
	        break;
            
        case 'F':
	        KEY( "Funds",	clan->funds,		fread_number( fp ) );
	        KEY( "Filename",	clan->filename,		fread_string_nohash( fp ) );
	        break;

        case 'M':
	        KEY( "Members",	clan->members,		fread_number( fp ) );
	        break;

        case 'N':
	        KEY( "Name",	clan->name,		fread_string( fp ) );
	        break;

        }

        if ( !fMatch )
        {
	        sprintf( buf, "Fread_clan: no match: %s", word );
	        bug( buf, 0 );
        }	
    }
}

/**
 * Reads a member data structure from a file.
 *
 * @author      Joel McBeth
 * @version     1.0     02/21/05
 * 
 * @param       fp      The file to read the structure from. 
 *
 * @returns     A member with the data read from the file.
 */
MEMBER_DATA* fread_member(FILE *fp)
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    MEMBER_DATA* member;

    member = new_member();

    for ( ; ; )
    {
	word   = feof(fp) ? "End" : fread_word(fp);
	fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = true;
                fread_to_eol(fp);
            break;
            case 'E':
                if (str_cmp(word, "End") == FALSE)
                {
                    /* Default values already set with new_member(). */
                    return member;
                }
                break;
            case 'N':
                KEY("Name",         member->name,           fread_string(fp));
                break;
            case 'L':
                KEY("LastPlayed",   member->last_played,    fread_number(fp));
                break;
            case 'R':
                KEY("Rank",         member->rank,           fread(number(fp));
                break;
        }

        if (fMatch == FALSE)
        {
            sprintf(buf, "Fread_member: no match: %s.", word);
	        bug(buf, 0);
        }
    }    
}

/**
 * Loads a clan from file.
 *
 * @author  unknown
 * @version 1.0     07/18/05 
 *
 * @param   clanfile    The filename of the clan file to load.
 *
 * @returns TRUE if loading suceeded, FALSE if it failed.
 */
bool load_clan_file( char *clanfile )
{
    char filename[256];
    CLAN_DATA *clan;
    MEMBER_DATA *member;
    FILE *fp;
    bool found;

    CREATE( clan, CLAN_DATA, 1 );
    
    found = FALSE;
    sprintf( filename, "%s%s", CLAN_DIR, clanfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {	
	    for ( ; ; )
	    {
	        char letter;
	        char *word;

	        letter = fread_letter( fp );
	        if ( letter == '*' )
	        {
		        fread_to_eol( fp );
		        continue;
	        }

	        if ( letter != '#' )
	        {                
		        bug( "Load_clan_file: # not found.", 0 );
		        break;
	        }

	        word = fread_word( fp );
	        if ( !str_cmp( word, "CLAN"	) )
	        {
                found = TRUE;

	    	    fread_clan( clan, fp );
	    	    break;
	        }
            else if (str_cmp(word, "MEMBER" ) == FALSE)
            {                
                member = fread_member(fp);

                LINK(member, clan->first_member, clan->last_member, next, prev);
            }
            else if ( !str_cmp( word, "END" ) )
            {
	            break;
            }
	        else
	        {
		        char buf[MAX_STRING_LENGTH];

		        sprintf( buf, "Load_clan_file: bad section: %s.", word );
		        bug( buf, 0 );
		        break;
	        }
	    }
	    fclose( fp );
    }

    if ( found )
    {
	    LINK( clan, first_clan, last_clan, next, prev );
        return found;	
    }
    else
    {
        free_clan(clan);
    }

    return found;
}


/*
 * Load in all the clan files.
 */
void load_clans( )
{
    FILE *fpList;
    char *filename;
    char clanlist[256];
    char buf[MAX_STRING_LENGTH];
    
    first_clan	= NULL;
    last_clan	= NULL;

    log_string( "Loading clans..." );

    sprintf( clanlist, "%s%s", CLAN_DIR, CLAN_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( clanlist, "r" ) ) == NULL )
    {
	perror( clanlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_clan_file( filename ) )
	{
	  sprintf( buf, "Cannot load clan file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done clans\n\r" );
    fpReserve = fopen( NULL_FILE, "r" );
    
    return;
}

/**
 * A command that inducts a person into a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/19/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_induct( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    clan = ch->pcdata->clan;

    /* Make sure the character is a leader or has the induct empowerment. */
    if (!is_leader(ch, clan) && !is_empowered(ch, POWER_INDUCT))
    {
        ch_printf(ch, "You are not bestowed the that power.");
        return;
    }    

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	    send_to_char( "Induct whom?\n\r", ch );
	    return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	    send_to_char( "That player is not here.\n\r", ch);
	    return;
    }

    if ( IS_NPC(victim) )
    {
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
    }

    if ( victim->pcdata->clan )
    {
	    if ( victim->pcdata->clan == clan )
	        send_to_char( "This player already belongs to your organization!\n\r", ch );
	    else
	        send_to_char( "This player already belongs to an organization!\n\r", ch );

	    return;
    }

    add_member(clan, victim);   
    
    act( AT_BLUE, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
    act( AT_BLUE, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
    act( AT_BLUE, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );

    save_char_obj( victim );
    save_clan(clan);
}


/**
 * A command that outcast a person from a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/19/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_outcast( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];    

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }    

    if (!is_empowered(ch, POWER_OUTCAST))
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Outcast whom?\n\r", ch );
	    return;
    }

    /* Get the victim and make sure they are in the room. */
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "That player is not here.\n\r", ch);
        return;
    }

    /* Can only outcast players! */
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }

    /* Can't outcast yourself. */
    if ( victim == ch )
    {
	    send_to_char( "Kick yourself out of your own clan?\n\r", ch );
	    return;
    }

    /* They victim has to be in the same clan. */
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n\r", ch );
	    return;
    }

    if (get_rank(ch) <= get_rank(victim))
    {
        ch_printf(ch, "Their rank is to high.\r\n");
        return;
    }

    remove_member(ch);

    act( AT_BLUE, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
    act( AT_BLUE, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
    act( AT_BLUE, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );

    sprintf(buf, "%s has been outcast from %s!", victim->name, clan->name);
    echo_to_all(AT_BLUE, buf, ECHOTAR_ALL);

    save_ch_obj(victim);
    save_clan(clan);
}


/**
 * An immortal command that sets most of the values for a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/19/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_setclan( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CLAN_DATA *clan;
    MEMBER_DATA* member;

    if ( IS_NPC(ch) || !ch->pcdata )
    	return;

    /* Uh, link dead people can't use set clan. */
    if ( !ch->desc )
	    return;

    switch( ch->substate )
    {
	    default:
	        break;
	    case SUB_CLANDESC:
	        clan = ch->dest_buf;

	        if ( !clan )
	        {
		        bug( "setclan: sub_clandesc: NULL ch->dest_buf", 0 );
		        stop_editing( ch );

                ch->substate = ch->tempnum;

		        send_to_char( "&RError: clan lost.\n\r" , ch );
		        return;
	        }

	        STRFREE( clan->description );
	        clan->description = copy_buffer( ch );
	        stop_editing( ch );
	        ch->substate = ch->tempnum;
            save_clan( clan );
	        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	    send_to_char( "Usage: setclan <clan> <field> <player>\n\r", ch );
	    send_to_char( "\n\rField being one of:\n\r", ch );
	    send_to_char( "funds name filename desc atwar member\n\r", ch );	
	    return;
    }

    clan = get_clan( arg1 );

    if ( !clan )
    {
	    send_to_char( "No such clan.\n\r", ch );
	    return;
    }

    if ( !strcmp( arg2, "funds" ) )
    {
	    clan->funds = atoi( argument );
	    send_to_char( "Done.\n\r", ch );
	    save_clan( clan );
	    return;
    }

    if ( !str_cmp( arg2, "atwar" ) )
    {
	    STRFREE( clan->atwar );
	    clan->atwar = STRALLOC( argument );
	    send_to_char( "Done.\n\r", ch );
	    save_clan( clan );
	    return;
    }
    
    if ( !strcmp( arg2, "name" ) )
    {
	    STRFREE( clan->name );
	    clan->name = STRALLOC( argument );
	    send_to_char( "Done.\n\r", ch );
	    save_clan( clan );
	    return;
    }

    if ( !str_cmp( arg2, "filename" ) )
    {
	    DISPOSE( clan->filename );
	    clan->filename = str_dup( argument );
	    send_to_char( "Done.\n\r", ch );
	    save_clan( clan );
	    write_clan_list( );
	    return;
    }

    if ( !str_cmp( arg2, "desc" ) )
    {
        ch->substate = SUB_CLANDESC;
        ch->dest_buf = clan;
        start_editing( ch, clan->description );
	    return;
    }

    if ( !strcmp( arg2, "member" ) )
    {
        argument = one_argument(argument, arg3);
        argument = one_argument(argument, arg4);

        if (arg3[0] == '\0' || arg4[0] == '\0')
        {
            ch_printf(ch, "Usage: setclan <clan> member <player> <field> <value>\r\n");
            ch_printf(ch, "\r\nField being one of:\n\r");
            ch_printf(ch, "\r\nrank, powers\r\n");
        }

        member = get_member_name(clan, arg3);

        if (member == NULL)
        {
            ch_printf(ch, "They aren't in that clan.\r\n");
            return;
        }

        if (!str_cmp(arg4, "rank"))
        {
            int rank;

            if (!argument || argument[0] == '\0')
            {
                ch_printf(ch, "Possible ranks are: \n\r");
                ch_printf(ch, "\r\n%s\r\n", get_ranks_string());
            }
            
            rank = get_rank_from_string(argument);

            if (rank == RANK_NONE)
            {
                sprintf(buf, "%s %s %s %s", arg1, arg2, arg3, arg4);
                do_setclan(ch, buf);

                return;
            }

            member->rank = rank;
            ch_printf(ch, "OK.\r\n");
        }
        else if (!str_cmp(arg4, "powers"))
        {
            int power;

            if (!argument || argument[0] == '\0')
            {
                ch_printf(ch, "Possible values are:\r\n");
                ch_printf(ch, "\r\nlist, or %s\r\n", get_powerflags());
            }

            if (!str_cmp(argument, "list"))
            {
                ch_printf(ch, "Their powers are:\r\n%s\r\n", get_powers_string_name(arg3));
                return;
            }

            power = get_powerflag(argument);

            if (power == -1)
            {
                sprintf(buf, "%s %s %s %s", arg1, arg2, arg3, arg4);
                do_setclan(ch, buf);

                return;
            }

            TOGGLE_BIT(member->powers, power);

            ch_printf(ch, "OK.");

        }

        save_clan(clan);
	    return;
    }

    do_setclan( ch, "" );
    return;
}

/**
 * A command that shows information about a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/19/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_showclan( CHAR_DATA *ch, char *argument )
{   
    CLAN_DATA *clan;
    PLANET_DATA *planet;
    char buf[MAX_STRING_LENGTH];
    int pCount = 0;
    int support;
    long revenue; 

    last_played = 0;

    if ( IS_NPC( ch ) )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    if ( argument[0] == '\0' )
    {
	    send_to_char( "Usage: showclan <clan>\n\r", ch );
	    return;
    }

    clan = get_clan( argument );
    if ( !clan )
    {
	    send_to_char( "No such clan.\n\r", ch );
	    return;
    }
        
    pCount = 0;
    support = 0;
    revenue = 0;
    
    for ( planet = first_planet ; planet ; planet = planet->next )
    {
        if ( clan == planet->governed_by )
        {
            support += planet->pop_support;
            pCount++;
            revenue += get_taxes(planet);
        }
    }
        
    if ( pCount > 1 )
        support /= pCount;


    ch_printf( ch, "&W%s      %s\n\r",
    			clan->name,
    			IS_IMMORTAL(ch) ? clan->filename : "" );
    ch_printf( ch, "&WDescription:&G\n\r%s\n\r&WLeaders: &G%s\n\r&WAt War With: &G%s\n\r",
    			clan->description,
    			get_leaders_string(clan),
    			clan->atwar );
    ch_printf( ch, "&WMembers: &G%d\n\r",
    			clan->members);
    ch_printf( ch, "&WSpacecraft: &G%d\n\r",
    			clan->spacecraft);
    ch_printf( ch, "&WVehicles: &G%d\n\r",
    			clan->vehicles);
    ch_printf( ch, "&WPlanets Controlled: &G%d\n\r",
    			pCount);
    ch_printf( ch, "&WAverage Popular Support: &G%d\n\r",
    			support);
    ch_printf(ch, "&WAge: &G%d\r\n", get_clan_age(clan));

    ch_printf(ch, "&WStatus: &G%d\r\n", get_clan_activity_string(clan));

    if(ch->pcdata->clan == clan || IS_IMMORTAL(ch)) {
	    ch_printf( ch, "&WWeekly Revenue: &G%ld\n\r", revenue);
	    ch_printf( ch, "&WHourly Wages: &G%ld\n\r", clan->salary );
	    ch_printf( ch, "&WTotal Funds: &G%ld\n\r", clan->funds );
    }    
}

/* Obselete 07/20/05
void do_makeclan( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    CLAN_DATA *clan;
    bool found;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeclan <clan name>\n\r", ch );
	return;
    }
    
    sprintf( filename, "%s%s", CLAN_DIR, strlower(argument) );

    CREATE( clan, CLAN_DATA, 1 );
    LINK( clan, first_clan, last_clan, next, prev );
    clan->name		= STRALLOC( argument );
    clan->description	= STRALLOC( "" );
    clan->leaders	= STRALLOC( "" );
    clan->atwar		= STRALLOC( "" );    
    clan->funds         = 0;
    clan->salary        = 0;
    clan->members       = 0;
}
*/

/**
 * A command that shows all the clans to the player.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/20/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_clans( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    PLANET_DATA *planet;
    int count = 0;
    int pCount, revenue;
    
    ch_printf( ch, "&WOrganization                                      Activity    Planets    Score\n\r");
    for ( clan = first_clan; clan; clan = clan->next )
    {
        pCount = 0;
        revenue = 0;
        
        for ( planet = first_planet ; planet ; planet = planet->next )
        {
            if ( clan == planet->governed_by )
            {  
                pCount++;
                revenue += get_taxes(planet);
            }
        }
          
        ch_printf( ch, "&Y%-46s    %-8s    %-7d    %d\n\r",
                  clan->name,  get_clan_activity_string(clan), pCount, revenue );
        count++;
    }

    if ( !count )
    {
	    set_char_color( AT_BLOOD, ch);
        send_to_char( "There are no organizations currently formed.\n\r", ch );
    }

    set_char_color( AT_WHITE, ch );
    send_to_char( "\n\rFor more information use: SHOWCLAN\n\r", ch );
    send_to_char( "See also: PLANETS\n\r", ch );
    
}

void do_shove( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int exit_dir;
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    bool nogo;
    ROOM_INDEX_DATA *to_room;    
    int chance;  

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    
    if ( arg[0] == '\0' )
    {
	send_to_char( "Shove whom?\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You shove yourself around, to no avail.\n\r", ch);
	return;
    }
    
    if ( (victim->position) != POS_STANDING )
    {
	act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	send_to_char( "Shove them in which direction?\n\r", ch);
	return;
    }

    exit_dir = get_dir( arg2 );
    if ( IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    &&  get_timer(victim, TIMER_SHOVEDRAG) <= 0)
    {
	send_to_char("That character cannot be shoved right now.\n\r", ch);
	return;
    }
    victim->position = POS_SHOVE;
    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
      nogo = TRUE;
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (!IS_AFFECTED(victim, AFF_PASS_DOOR)
    ||   IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n\r", ch );
        victim->position = POS_STANDING;
	return;
    }
    to_room = pexit->to_room;

    if ( IS_NPC(victim) )
    {
	send_to_char("You can only shove player characters.\n\r", ch);
	return;
    }
    
chance = 50;

/* Add 3 points to chance for every str point above 15, subtract for 
below 15 */

chance += ((get_curr_str(ch) - 15) * 3);

chance += (ch->top_level - victim->top_level);
 
/* Debugging purposes - show percentage for testing */

/* sprintf(buf, "Shove percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/

if (chance < number_percent( ))
{
  send_to_char("You failed.\n\r", ch);
  victim->position = POS_STANDING;
  return;
}
    act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
    act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
    move_char( victim, get_exit(ch->in_room,exit_dir), 0);
    if ( !char_died(victim) )
      victim->position = POS_STANDING;
    WAIT_STATE(ch, 12);
    /* Remove protection from shove/drag if char shoves -- Blodkai */
    if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE)   
    &&   get_timer(ch, TIMER_SHOVEDRAG) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

void do_drag( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int exit_dir;
    CHAR_DATA *victim;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *to_room;
    bool nogo;
    int chance;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drag whom?\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char("You take yourself by the scruff of your neck, but go nowhere.\n\r", ch);
	return; 
    }

    if ( IS_NPC(victim) )
    {
	send_to_char("You can only drag player characters.\n\r", ch);
	return;
    }

    if ( victim->fighting )
    {
        send_to_char( "You try, but can't get close enough.\n\r", ch);
        return;
    }
          
    if ( arg2[0] == '\0' )
    {
	send_to_char( "Drag them in which direction?\n\r", ch);
	return;
    }

    exit_dir = get_dir( arg2 );

    if ( IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    &&   get_timer( victim, TIMER_SHOVEDRAG ) <= 0)
    {
	send_to_char("That character cannot be dragged right now.\n\r", ch);
	return;
    }

    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
      nogo = TRUE;
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (!IS_AFFECTED(victim, AFF_PASS_DOOR)
    ||   IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n\r", ch );
	return;
    }

    to_room = pexit->to_room;

    chance = 50;
    

/*
sprintf(buf, "Drag percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/
if (chance < number_percent( ))
{
  send_to_char("You failed.\n\r", ch);
  victim->position = POS_STANDING;
  return;
}
    if ( victim->position < POS_STANDING )
    {
	sh_int temp;

	temp = victim->position;
	victim->position = POS_DRAG;
	act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR ); 
	act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT ); 
	move_char( victim, get_exit(ch->in_room,exit_dir), 0);
	if ( !char_died(victim) )
	  victim->position = temp;
/* Move ch to the room too.. they are doing dragging - Scryn */
	move_char( ch, get_exit(ch->in_room,exit_dir), 0);
	WAIT_STATE(ch, 12);
	return;
    }
    send_to_char("You cannot do that to someone who is standing.\n\r", ch);
    return;
}


/**
 * A command that resigns a character from his clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/20/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_resign( CHAR_DATA *ch, char *argument )
{

    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];
            
    if ( IS_NPC(ch) || !ch->pcdata )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}
        
    clan =  ch->pcdata->clan;
        
    if ( clan == NULL )
    {
        send_to_char( "You have to join an organization before you can quit it.\n\r", ch );
        return;
	}

    if ( is_leader(ch) )
    {
        ch_printf( ch, "You can't resign from %s ... you are one of the leaders!\n\r", clan->name );
        return;
    }

    remove_member(ch);
       
    act( AT_BLUE, "You resign your position in $t", ch, clan->name, NULL , TO_CHAR );
    sprintf(buf, "%s has quit %s!", ch->name, clan->name);
    echo_to_all(AT_BLUE, buf, ECHOTAR_ALL);

    save_char_obj( ch );	/* clan gets saved when pfile is saved */
    save_clan(clan);
    
    return;

}


/**
 * A command that resigns a character from his clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/20/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_clan_withdraw( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    long       amount;
    char buf[MAX_STRING_LENGTH];
    
    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	    send_to_char( "You don't seem to belong to an organization to withdraw funds from...\n\r", ch );
	    return;
    }

    if (!ch->in_room || !IS_SET(ch->in_room->room_flags, ROOM_BANK) )
    {
       send_to_char( "You must be in a bank to do that!\n\r", ch );
       return;
    }
    
    if (!is_empowered(ch, POWER_WITHDRAW))
    {
   	    send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability." ,ch );
   	    return;
    }

    clan = ch->pcdata->clan;
    
    amount = atoi( argument );
    
    if ( !amount )
    {
	    send_to_char( "How much would you like to withdraw?\n\r", ch );
	    return;
    }
    
    if ( amount > clan->funds )
    {
	    ch_printf( ch,  "%s doesn't have that much!\n\r", clan->name );
	    return;
    }
    
    if ( amount < 0 )
    {
	    ch_printf( ch,  "Nice try...\n\r" );
	    return;
    }
    
    ch_printf( ch,  "You withdraw %ld credits from %s's funds.\n\r", amount, clan->name );
    
    clan->funds -= amount;
    ch->gold += amount;

    /* If if it a large transaction then it should be logged for possible cheating. */ 
    if (amount >= LARGE_TRANSACTION)
    {
        sprintf(buf, "TRANSACTION: %s widthdrew %ld credits from the clan %s.", ch->name, amount, clan->name);
        log_string(buf);
    }
    
    save_clan ( clan );
    save_char_obj(ch);
}


/**
 * A command that gives money to a player's clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/20/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_clan_donate( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    long       amount;
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	    send_to_char( "You don't seem to belong to an organization to donate to...\n\r", ch );
        return;
    }

    if (!ch->in_room || !IS_SET(ch->in_room->room_flags, ROOM_BANK) )
    {
       send_to_char( "You must be in a bank to do that!\n\r", ch );
       return;
    }

    clan = ch->pcdata->clan;
    
    amount = atoi( argument );
    
    if ( !amount )
    {
	    send_to_char( "How much would you like to donate?\n\r", ch );
	    return;
    }

    if ( amount < 0 )
    {
	    ch_printf( ch,  "Nice try...\n\r" );
	    return;
    }
    
    if ( amount > ch->gold )
    {
	    send_to_char( "You don't have that much!\n\r", ch );
	    return;
    }

    /* If if it a large transaction then it should be logged for possible cheating. */ 
    if (amount >= LARGE_TRANSACTION)
    {
        sprintf(buf, "TRANSACTION: %s donated %ld credits to the clan %s.", ch->name, amount, clan->name);
        log_string(buf);
    }
    
    ch_printf( ch,  "You donate %ld credits to %s's funds.\n\r", amount, clan->name );
    
    clan->funds += amount;
    ch->gold -= amount;

    save_char_obj(ch);
    save_clan ( clan );            
}

/**
 * A command that makes a regular member a leader.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/20/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_appoint ( CHAR_DATA *ch , char *argument )
{    
    MEMBER_DATA* member;
    char name[MAX_STRING_LENGTH];
    
    /* Make sure the character isn't an NPC. */
    if ( IS_NPC( ch ) || !ch->pcdata )
    {
        ch_printf(ch, "You can't do that.\r\n");
        return;
    }

    /* Check if the character is in a clan. */
    if ( !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    /* Make sure the character is a leader. */
    if (  !is_leader(ch)  )
    {
	    send_to_char( "Only your leaders can do that!\n\r", ch );
	    return;
    }

    if ( argument[0] == '\0' )
    {
	    send_to_char( "Useage: appoint name\n\r", ch );
	    return;
    }

    one_argument( argument, name );
    
    member = get_member_name(ch->pcdata->clan, name);

    /* Check if they are in the clan. */
    if (member == NULL)
    {
        ch_printf(ch, "That person isn't in your clan.\r\n");
        return;
    }

    /* Check if they are already a leader. */
    if (member->rank == MAX_RANK)
    {
        ch_printf(ch, "They are already a leader!\r\n");
        return;
    }

    /* Make them a leader. */
    member->rank = MAX_RANK;

    save_clan(ch->pcdata->clan);        
}


/**
 * Demotes a character in a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_demote ( CHAR_DATA *ch , char *argument )
{    
    char name[MAX_STRING_LENGTH];
    MEMBER_DATA* member;

    /* Make sure they aren't an NPC. */
    if ( IS_NPC( ch ) || !ch->pcdata )
      return;

    /* Make sure they have a clan. */
    if ( !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    /* Make sure the character is a leader of the clan. */
    if (!is_leader(ch))
    {
	    send_to_char( "Only your leaders can do that!\n\r", ch );
	    return;
    }    

    if ( argument[0] == '\0' )
    {
	    send_to_char( "Demote who?\n\r", ch );
	    return;
    }

    one_argument(argument, name);

    member = get_member_name(ch->pcdata->clan, name);

    /* Make sure they are in the clan. */
    if (member == NULL)
    {
        ch_printf(ch, "That person isn't in your clan.\r\n");
        return;
    }

    /* Make sure they can be demoted. */
    if (member->rank == MAX_RANK)
    {
        ch_printf(ch, "They can't be demoted.\r\n");
        return;
    }

    if (member->rank == MIN_RANK)
    {
        ch_printf(ch, "They can't be demoted anymore.\r\n");
        return;
    }

    /* Demote them. */
    member->rank--;    
    
    save_clan ( ch->pcdata->clan );        
}

/**
 * A command that empowers another player with clan permissions.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/21/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_empower ( CHAR_DATA *ch , char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];
    MEMBER_DATA* member;
    int i;

    /* Make sure they aren't an NPC. */
    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    clan = ch->pcdata->clan;

    /* Make sure they have the ability to empower others. */
    if (!is_empowered(ch, POWER_EMPOWER))
    {
	    send_to_char( "You clan hasn't seen fit to bestow that ability to you!\n\r", ch );
	    return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' )
    {
	    send_to_char( "Empower whom to do what?\n\r", ch );
	    return;
    }

    /* Make sure the character is in the room. */
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	    send_to_char( "That player is not here.\n\r", ch);
	    return;
    }

    /* Make sure the character isn't an NPC. */
    if ( IS_NPC(victim) )
    {
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
    }

    /* Make sure they character doesn't try to empower himself. */
    if ( victim == ch )
    {
	    send_to_char( "Nice try.\n\r", ch );
	    return;
    }
 
    /* Make sure both characters are in the same clan. */
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n\r", ch );
	    return;
    }

    /* Soooooo tacky. */
    if ( arg2[0] == '\0' )
        strcpy( arg2 , "HelpMeImConfused" );
    

    /* If the second arg is list then list all the victim's powers. */
    if ( !str_cmp( arg2, "list" ) )
    {
        ch_printf( ch, "Current bestowed commands on %s: %s.\n\r",
                      victim->name, get_powers_string(victim));
        return;
    } 

    member = get_member(victim);

    if (member == NULL)            
    {
        sprintf(buf, "do_empower: %s has NULL member data.", victim->name);
        bug(buf);
        return;    
    }

    /* Remove all powers from the victim. */
    if ( !str_cmp( arg2, "none" ) )
    {    
        member->powers = POWER_NONE;

        ch_printf( ch, "Bestowments removed from %s.\n\r", victim->name );
        ch_printf( victim, "%s has removed your bestowed clan abilities.\n\r", ch->name );

        return;
    }    
    else if ( !str_cmp( arg2, "pilot" ) )
    {
        if (!is_empowered(ch, POWER_PILOT))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_PILOT))
        {
            REMOVE_BIT(member->powers, POWER_PILOT);

            ch_printf(victim, "%s has taken away your permission to fly clan ships.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to fly clan ships.\r\n");

        }
        else
        {
            SET_BIT(member->powers, POWER_PILOT);

            ch_printf( victim, "%s has given you permission to fly clan ships.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to fly clan ships.\n\r", ch );
        }
    }
    else if ( !str_cmp( arg2, "withdraw" ) )
    {
        if (!is_empowered(ch, POWER_WITHDRAW))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_WITHDRAW))
        {
            REMOVE_BIT(member->powers, POWER_WITHDRAW);

            ch_printf(victim, "%s has taken away your permission to withdraw clan funds.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to withdraw clan funds.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_WITHDRAW);

            ch_printf( victim, "%s has given you permission to withdraw clan funds.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to withdraw clan funds.\n\r", ch );
        }
    }
    else if ( !str_cmp( arg2, "clanbuyship" ) )
    {
        if (!is_empowered(ch, POWER_BUYSHIP))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_BUYSHIP))
        {
            REMOVE_BIT(member->powers, POWER_BUYSHIP);

            ch_printf(victim, "%s has taken away your permission to buy clan ships.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to buy clan ships.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_BUYSHIP);

            ch_printf( victim, "%s has given you permission to buy clan ships.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to use clanbuyship.\n\r", ch );
        }
    }
    else if ( !str_cmp( arg2, "induct" ) )
    {
        if (!is_empowered(ch, POWER_INDUCT))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_INDUCT))
        {
            REMOVE_BIT(member->powers, POWER_INDUCT);

            ch_printf(victim, "%s has taken away your permission to induct new members.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to induct new members.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_INDUCT);

            ch_printf( victim, "%s has given you permission to induct new members.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to induct new members.\n\r", ch );
        }      
    }
    else if ( !str_cmp( arg2, "empower" ) )
    {
        if (!is_empowered(ch, POWER_EMPOWER))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_EMPOWER))
        {
            REMOVE_BIT(member->powers, POWER_EMPOWER);

            ch_printf(victim, "%s has taken away your permission to empower others.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to empower others.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_EMPOWER);

            ch_printf( victim, "%s has given you the ability to empower others.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to empower others.\n\r", ch );
        }         
    }
    else if ( !str_cmp( arg2, "build" ) )
    {
        if (!is_empowered(ch, POWER_BUILD))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_BUILD))
        {
            REMOVE_BIT(member->powers, POWER_BUILD);

            ch_printf(victim, "%s has taken away your permission to build and modify areas.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to build and modify areas.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_BUILD);

            ch_printf( victim, "%s has given you permission to build and modify areas.\n\r", ch->name );
            send_to_char( "Ok, they now have the ability to modify and build areas.\n\r", ch );
        }      
    }
    else if ( !str_cmp( arg2, "war" ) )
    {
        if (!is_empowered(ch, POWER_WAR))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_WAR))
        {
            REMOVE_BIT(member->powers, POWER_WAR);

            ch_printf(victim, "%s has taken away your permission to declare war.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to declare war.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_WAR);

            ch_printf( victim, "%s has given you permission to declare war.\n\r", ch->name );
            ch_printf(ch, "Ok, they now have the ability to declare war.\n\r");
        }      
    }
    else if ( !str_cmp( arg2, "wages" ) )
    {
        if (!is_empowered(ch, POWER_WAGES))        
            ch_printf(ch, "How can you empower someone with a power you don't have?\r\n");                    
        else if (is_empowered(victim, POWER_WAGES))
        {
            REMOVE_BIT(member->powers, POWER_WAGES);

            ch_printf(victim, "%s has taken away your permission to set the clan wages.\r\n", ch->name);
            ch_printf(ch, "They no longer have the ability to set the clan wages.\r\n");
        }
        else
        {
            SET_BIT(member->powers, POWER_WAGES);

            ch_printf( victim, "%s has given you permission to set the clan wages.\n\r", ch->name );
            ch_printf(ch, "Ok, they now have the ability to set the clan wages.\n\r");
        }      
    }
    else if (!str_cmp(arg2, "all"))
    {
        MEMBER_DATA* chmem;
        bool hasAll = TRUE, victHasAll = TRUE;

        chmem = get_member(ch);

        if (member == NULL)            
        {
            sprintf(buf, "do_empower: the ch %s has NULL member data.", ch->name);
            bug(buf);
            return;    
        }
        
        /* Check if the ch has all the powers. Faster than calling is_empowered a lot. */         
        for (i = 0; i < POWER_MAX_FLAGS; i++)
        {
            if (!IS_SET(chmem->powers, 1 << i))
            {
                hasAll = FALSE;
                break;
            }
        }

        /* Check if the victim has all the powers. */
        for (i = 0; i < POWER_MAX_FLAGS; i++)
        {
            if (!IS_SET(member->powers, 1 << i))
            {
                victHasAll = FALSE;
                break;
            }
        }
        
        if (!hasAll)        
            ch_printf(ch, "How can you empower someone with a powers you don't have?\r\n");                
        else if (victHasAll)        
            ch_printf(ch, "They already have permission to use all the powers.\r\n");        
        else /* Set all the power bits. */
        {
            /* I know there is a better way to do this, but I can't think of it now. */
            for (i = 0; i < POWER_MAX_FLAGS; i++)            
                SET_BIT(member->flags, 1 << i);

            ch_printf(victim, "%s has given you permission to use all clan powers.\r\n", ch->name);
            ch_printf(ch, "OK, they now have the ability to use all clan powers.\r\n");
        }
    }
    else
    {
      send_to_char( "Currently you may empower members with only the following:\n\r", ch ); 
      send_to_char( "\n\rpilot:        ability to fly clan ships\n\r", ch );
      send_to_char(     "withdraw:     ability to withdraw clan funds\n\r", ch );
      send_to_char(     "clanbuyship:  ability to buy clan ships\n\r", ch );    
      send_to_char(     "induct:       ability to induct new members\n\r", ch );    
      send_to_char(     "build:        ability to create and edit rooms\n\r", ch );    
      send_to_char(     "empower:      ability to bestow other members (use with caution)\n\r", ch );    
      send_to_char(     "war:          ability to war other clans\r\n", ch);
      send_to_char(     "wages:        ability to set the clan's wages\r\n", ch);
      send_to_char(     "all:          gives all powers\r\n", ch);
      send_to_char(     "none:         removes bestowed abilities\n\r", ch );      
      send_to_char(     "list:         shows bestowed abilities\n\r", ch );    
    }
        
    save_clan(clan);
    return;
}

/**
 * A command that sets the player as leader if all the other leaders are dead.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_overthrow( CHAR_DATA *ch , char * argument )
{    
    MEMBER_DATA* member;
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
    {
        ch_printf(ch, "You can't do that.\r\n");
        return;
    }
       
    if ( !ch->pcdata || !ch->pcdata->clan )
    {
       send_to_char( "You have to be part of an organization before you can claim leadership.\n\r", ch );
       return;
    }

    if ( has_leaders(ch->pcdata->clan) )
    {
       send_to_char( "Your organization already has strong leadership...\n\r", ch );
       return;
    }

    member = get_member(ch);

    if (member == NULL)
    {
        sprintf(buf, "do_overthrow: unable to get member data for %s.", ch->name);
        bug(buf);
        return;
    }

    member->rank = MAX_RANK;

    ch_printf( ch, "OK. You are now a leader of %s.\n\r", ch->pcdata->clan->name );       

    save_clan(clan);         
}

/**
 * A command that sets a player's clan at war to another clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_war ( CHAR_DATA *ch , char *argument )
{
    CLAN_DATA *wclan;
    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    clan = ch->pcdata->clan;

    if (!is_empowered(ch, POWER_WAR))
    {
	    send_to_char( "You clan hasn't empowered you to declare war!\n\r", ch );
	    return;
    }

    if ( argument[0] == '\0' )
    {
	    send_to_char( "Declare war on who?\n\r", ch );
	    return;
    }

    if ( ( wclan = get_clan( argument ) ) == NULL )
    {
	    send_to_char( "No such clan.\n\r", ch);
	    return;
    }

    if ( wclan == clan )
    {
	    send_to_char( "Declare war on yourself?!\n\r", ch);
	    return;
    }
    
    if (get_clan_age(clan) < (CLAN_CEASEFIRE_TIME / 4))
    {
       ch_printf(ch, "You may not declare war on them yet.\r\n");
       return;
    }

    if ( clan_is_atwar(clan, wclan) )
    {
        CLAN_DATA *tclan;
        strcpy( buf, "" );
        
        for ( tclan = first_clan ; tclan ; tclan = tclan->next )
        {
            if ( nifty_is_name( tclan->name , clan->atwar ) && tclan != wclan )
            {
                 strcat ( buf, "\n\r " );
                 strcat ( buf, tclan->name );
                 strcat ( buf, " " );
            }
        }
        
        STRFREE ( clan->atwar );
        clan->atwar = STRALLOC( buf );
        
        sprintf( buf , "%s has declared a ceasefire with %s!" , clan->name , wclan->name );
        echo_to_all( AT_WHITE , buf , ECHOTAR_ALL );

        clan_clan(clan);
        
	    return;
    }
    
    strcpy ( buf, clan->atwar );
    strcat ( buf, "\n\r " );
    strcat ( buf, wclan->name );
    strcat ( buf, " " );
    
    STRFREE ( clan->atwar );
    clan->atwar = STRALLOC( buf );
    
    sprintf( buf , "%s has declared war on %s!" , clan->name , wclan->name );
    echo_to_all( AT_RED , buf , ECHOTAR_ALL );

    save_clan(clan);

}

/**
 * A command that sets the wages of a clan.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_setwages ( CHAR_DATA *ch , char *argument )
{
    CLAN_DATA *clan;
    int salary;

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->clan )
    {
	    send_to_char( "Huh?\n\r", ch );
	    return;
    }

    clan = ch->pcdata->clan;

    if (!is_empowered(ch, POWER_WAGES))
    {
	    send_to_char( "You clan hasn't empowered you to set wages!\n\r", ch );
	    return;
    }


    if ( argument[0] == '\0' )
    {
	    send_to_char( "Set clan wages to what?\n\r", ch );
	    return;
    }

    salary = atoi(argument);

    if (salary == 0 && !str_cmp(argument, "0"))
    {
        ch_printf(ch, "The salary has to be a number.\r\n");
        return;
    }

    if (salary < 0)
    {
        ch_printf(ch, "You can't pay them that.\r\n");
        return;
    }

    clan->salary = atoi( argument );
    
    ch_printf( ch , "Clan wages set to %d credits per hour\n\r" , clan->salary );

    save_clan(clan);
}

/**
 * Checks if the string passed to it would be a valid name for a clan.
 * This should check is the name is valid for security reasons as well
 * to check if it is appropriate.
 *
 * @author  Joel McBeth
 * @version 1.0         02/10/05
 *
 * @param   name   The name to check.
 *
 * @returns TRUE if the name is valid, FALSE if it is not.
 */
bool is_clan_name_valid(char* name)
{    
    /* So someone doesn't try to name their clan to conflict with the revolting clan. */
    if (str_cmp(name, REVOLT_CLAN_NAME) == FALSE)
    {
        return FALSE;
    }

    /* TODO: Implement this. */
    return TRUE;
}

/**
 * Adds an online character to a clan.
 * If a character already is in a clan then this does nothing.
 *
 * @author  Joel McBeth
 * @version 1.0     02/22/05
 *
 * @param   clan    The clan to add the member to.
 * @param   ch      The character to add.
 */
void add_member(CLAN_DATA* clan, CHAR_DATA* ch)
{
    MEMBER_DATA* member;
    char buf[MAX_STRING_LENGTH];

    if (clan == NULL)
    {
        bug("add_member: NULL clan.", 0);
        return;
    }

    if (ch == NULL)
    {
        bug("add_member: NULL ch.", 0);
        return;
    }

    /* If the character already has a clan then do nothing. */    
    if (ch->clan != NULL)
    {
        sprintf(buf, "add_member: %s already has a clan.", ch->name);
        bug(buf);

        return;
    }
     
    if (ch->pcdata->clanname != NULL)
        STRFREE(ch->pcdata->clan_name);

    /* Set the players clan to the clan. */
    ch->pcdata->clan_name = QUICKLINK(clan->name);
    ch->pcdata->clan = clan;

    /* Create the member data for the character. */
    member = new_member();

    STRFREE(member->name);
    member->name = QUICKLINK(ch->name);

    member->last_played = current_time;

    /* Add the new member data to the list of members. */
    LINK(member, clan->first_member, clan->last_member, next, prev);

    clan->members++;
}

/**
 * Removes a character from a clan. 
 *
 * @author  Joel McBeth
 * @version 1.0   03/12/05
 *
 * @param   clan  The clan to remove the member from.
 * @param   ch    The character to remove from the clan.
 */
void remove_member(CHAR_DATA* ch)
{
    MEMBER_DATA* member;
    CLAN_DATA* clan;
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL)
    {
        bug("remove_member: NULL ch.", 0);
        return;
    }

    clan = get_ch_clan(ch);

    /* The character doesn't have a clan! */
    if (clan == NULL)   
        return;   

    member = get_member(ch);

    if (member == NULL)
    {
        sprintf(buf, "remove_member: %s has a clan but doesn't have a member.", ch->name);
        bug(buf);
        return;
    }

    UNLINK(member, clan->first_member, clan->last_member, next, prev);

    free_member(member);

    /* Better safe than sorry! */
    if (ch->pcdata->clan_name != NULL)
    {
        sprintf(buf, "remove_member: %s has a NULL clan name.", ch->name);
        bug(buf);
        STRFREE(ch->clan_name);
    }

    ch->pcdata->clan = NULL;

    clan->members--;
}

/**
 * Gets the member data of a character his clan.
 *
 * @author  Joel McBeth
 * @version 1.0   03/12/05
 * 
 * @param   ch    The character to get the member data of.
 *
 * @returns The member data for the character, if the character is an NPC or not in a clan NULL is returned.
 */
MEMBER_DATA* get_member(CHAR_DATA* ch)
{
   MEMBER_DATA* member;   
   CLAN_DATA* clan;
   char buf[MAX_STRING_LENGTH];

   if (ch == NULL)
   {
      bug("get_member: NULL ch.", 0);
      return NULL;
   }

   if (IS_NPC(ch))
   {
       bug("get_member: ch is an NPC.", 0);
       return NULL;
   }

   clan = get_ch_clan(ch);

   if (clan == NULL)
   {
      bug("get_member: ch doesn't have a clan.", 0);
      return NULL;
   }

   /* Go through and see if the member's name is the characters name. */
   for (member = clan->first_member; member != NULL; memeber = member->next)
   {
      /* The member's name and the character's name match. */
      if (str_cmp(member->name, ch->name) == FALSE)      
         return member;      
   }

   /**
    * The character has clan as their clan but there is no member data in
    * the clan for the character if the function hasn't returned yet.
    */
   sprintf(buf, "get_member: %s was in the clan %s but a member wasn't found.", ch->name, ch->pcdata->clan->name);
   bug(buf);

   /* The member wasn't found. */
   return NULL;
}

/**
 * Gets the member data of a character in a clan given the character's name.
 *
 * @author  Joel McBeth
 * @version 1.0     07/28/05
 * 
 * @param   name    The character to get the member data of.
 *
 * @returns The member data for the character, if the character is not in a clan NULL is returned.
 */
MEMBER_DATA* get_member_name(char* name)
{
   MEMBER_DATA* member;
   CLAN_DATA* clan;
   char buf[MAX_STRING_LENGTH];

   /* Make sure the name isn't blank or NULL. */
   if (name == NULL || name[0] == '\0')
   {
      bug("get_member_name: NULL or blank name", 0);
      return NULL;
   }

   /* Go through and see if the member's name is the characters name. */
   for (member = clan->first_member; member != NULL; memeber = member->next)
   {
      /* The member's name and the character's name match. */
      if (str_cmp(member->name, name) == FALSE)      
         return member;      
   }

   /* If the character wasn't in a clan then it gets here and returns NULL. */
   return NULL;
}

/**
 * Frees all resources associated with the member from memory.
 * However it does not free other members in the list. Makes 
 * the member NULL. Should be called after it is removed from
 * a list.
 *
 * @author  Joel McBeth
 * @version 1.0     02/22/05
 *
 * @param   member  The member to free.
 */
void free_member(MEMBER_DATA* member)
{
    if (member == NULL)
    {
        bug("free_member: trying to free a NULL member.", 0);
        return;
    }

    STRFREE(member->name);

    DISPOSE(member);

    member = NULL;
}

/**
 * Allocates memory and sets default values.
 *
 * @author  Joel McBeth
 * @version 1.0     02/22/05
 *
 * @returns A pointer to a allocated and initialized member.
 */
MEMBER_DATA* new_member(void)
{
    MEMBER_DATA* member;

    CREATE(member, MEMBER_DATA, 1);

    member->next        = NULL:
    member->prev        = NULL;
    member->name        = STRALLOC(""):
    member->rank        = RANK_MEMBER;
    member->last_played = 0;

    return member;
}

/**
 * Frees all the resources associated with the clan from memory.
 * It does not free other clans in the list. Makes the clan NULL.
 *
 * @author  Joel McBeth
 * @version 1.0     02/08/05 
 *
 * @param   clan    The structure that will be freed. 
 */
void free_clan(CLAN_DATA* clan)
{
    MEMBER_DATA* member;
    MEMBER_DATA* next;

    if (clan == NULL)
    {
        bug("free_clan: trying to free a NULL clan.", 0);
        return;
    }

    str_free(clan->filename);

    STRFREE(clan->name);
    STRFREE(clan->description);
    STRFREE(clan->atwar);    

    /* Free all the members. */
    for (member = all_members; member != NULL; member = next)
    {
        next = member->next;

        free_member(member);
    }

    DISPOSE(clan);

    clan = NULL;
}

/**
 * Allocates memory and sets initial default values.
 *
 * @author  Joel McBeth
 * @version 1.0     02/08/05 
 *
 * @returns An initialized structure.
 */
CLAN_DATA* new_clan(void)
{
    CLAN_DATA* clan;

    /* Allocate memory. */
    CREATE(clan, CLAN_DATA, 1);

    /* Initialize values. */
    clan->prev          = NULL;
    clan->next          = NULL;
    clan->first_member  = NULL;
    clan->last_member   = NULL;
    clan->filename      = str_dup("");
    clan->name          = STRALLOC("");
    clan->description   = STRALLOC("");
    clan->atwar         = STRALLOC("");    
    clan->funds         = 0;
    clan->salary        = 0;
    clan->members       = 0;
    clan->spacecraft    = 0;
    clan->vehicles      = 0;
    clan->created       = -1;    

    return clan;
}

/**
 * Removes the clan from the game world.
 *
 * @author  Joel McBeth
 * @version 1.0     02/10/05
 *
 * @param   clan    The clan to be removed.
 */
void extract_clan(CLAN_DATA* clan)
{
    if (clan == NULL)
    {
        bug("extract_clan: trying to extract a null clan.");
        return;
    }

    UNLINK(clan, first_clan, last_clan, next, prev);
}

/**
 * Adds a clan to the game world.
 * This also saves the clan.
 *
 * @author  Joel McBeth
 * @version 1.0     02/10/05
 *
 * @param   clan    The clan to add.
 */
void add_clan(CLAN_DATA* clan)
{
    if (clan == NULL)
    {
        bug("add_clan: trying to add a null clan.");
        return;
    }

    LINK(clan, first_clan, last_clan, next, prev);
}

/**
 * Takes a name of a clan and turns it into a valid filename.
 * Succeptible to buffer overflows.
 *
 * @author  Joel McBeth
 * @version 1.0     02/13/05
 *
 * @param   name    The name of the clan.
 *
 * @returns A valid filename for the clan.
 */
char* get_valid_clan_filename(const char* name)
{
    static char filename[MAX_FILENAME_LENGTH];

    if (name == NULL)
    {
        bug("get_valid_clan_filename: NULL name", 0);
        return NULL;
    }    

    /* Replace spaces. */
    strcpy(filename,  str_rep(name, ' ', '_'));

    strcat(filename, ".clan");

    return filename;
}

/**
 * Checks if the clan filename doesn't contain any invalid characters.
 * This includes spaces. Also checks for buffer possible buffer overflows.
 *
 * @author  Joel McBeth
 * @version 1.0         02/18/05
 *
 * @param   filename    The filename of the clan to check.
 *
 * @returns If the filename is valid.
 */
bool is_valid_clan_filename(const char* filename)
{
    if (filename == NULL)
    {
        bug("is_valid_clan_filename: NULL filename", 0);
        return FALSE;
    }

    if (strlen(filename) > MAX_FILENAME_LENGTH)
    {
        return FALSE;
    }

    /* Check if there are spaces. */
    if (strchr(filename, ' ') != NULL)
    {
        return FALSE;
    }

    return TRUE;
}


/**
 * Checks if another clan has the same filename.
 *
 * @author  Joel McBeth
 * @version 1.0         02/18/05
 *
 * @param   filename    The clan filename.
 *
 * @returns If the clan filename is taken.
 */
bool is_clan_filename_taken(const char* filename)
{
    CLAN_DATA* clan_ref;

    for (clan_ref = first_clan; clan_ref != NULL; clan_ref = clan_ref->next)
    {
        /* Check all the clans and see if the filenames are the same. */
        if (str_cmp(clan_ref->filename, filename) == FALSE)
            return TRUE;
    }

    return FALSE;
}

/**
 * Creates a new clan and adds it to the game world.
 *
 * @author  Joel McBeth
 * @version 1.0     02/10/05
 *
 * @param   name    The name to give the clan.
 *
 * @returns The clan that was added.
 */
CLAN_DATA* create_clan(char* name)
{
    CLAN_DATA* clan;   

    /* Shortcircuiting. */
    if ((name == NULL) || (name[0] == '\0'))
    {
        bug("create_clan: NULL or empty name.");
        return NULL;
    }

    /* Make sure the clan name and the length of the extention don't cause a buffer overflow. */
    if ((strlen(name) + 5) > (MAX_FILENAME_LENGTH - 1))
    {
        bug("create_clan: the length of the name causes a buffer overflow.");
        return NULL;
    }

    /* Allocate memory. */
    clan = new_clan();

    clan->created = current_time;

    /* Set the name and filename. */
    clan->name = STRALLOC(name);    
    
    clan->filename = str_dup(get_valid_clan_filename(clan->name));    

    add_clan(clan);

    /* Save it to file. */
    save_clan(clan);
    write_clan_list();

    return clan;
}

/**
 * This will remove a clan from the game and create a backup and free the memory.
 *
 * @author  Joel McBeth
 * @version 1.0     02/14/05
 *
 * @param   clan    The clan to remove.
 */
void remove_clan(CLAN_DATA* clan)
{
    char filename[256], old_filename[256];
    CHAR_DATA* ch;
    SHIP_DATA* ship;
    PLANET_DATA* planet;

    if (clan == NULL)
    {
        bug("remove_clan: NULL clan");
        return;
    }

    /* Make sure everyone online has their clan reset. */
    for (ch = first_char; ch != NULL; ch = ch->next)
    {        
        if ((IS_NPC(ch) == FALSE) && (ch->pcdata->clan == clan))
        {
            /* Make sure no players are still in the clan that are logged on. */
            STRFREE(ch->pcdata->clan_name);
            ch->pcdata->clan_name = STRALLOC("");
            ch->pcdata->clan = NULL;
        }
        else if ((IS_NPC(ch) == TRUE) && (ch->mob_clan == clan))
        {
            /* Make sure no NPC's still reference the clan. */
            ch->mob_clan = NULL;
        }
    }

    for (ship = first_ship; ship != NULL; ship = ship->next)
    {
        /* Make sure you remove all the ships from their clan. */
        if ((ship->type != MOB_SHIP) && (str_cmp(ship->owner, clan->name) == FALSE))
        {
            STRFREE(ship->owner);
            ship->owner = STRALLOC("");
        }        
    }

    for (planet = first_planet; planet != NULL; planet = planet->next)
    {
        /* Make sure no planets still have this set as their clan. */
        if (planet->governed_by == clan)        
            planet->governed_by = NULL;        
    }   

    sprintf(old_filename, "%s%s", CLAN_DIR, clan->filename);

    /* Remove from the game world. */
    extract_clan(clan);
    /* Free the memory. */
    free_clan(clan);

    /* If for some reason the clan didn't have a filename. */
    if ((old_filename == NULL) || (old_filename[0] == '\0'))
    {
        bug("remove_clan: clan had a NULL or empty filename.");

        sprintf(filename, "%s.bak", old_filename);

        rename(old_filename, filename);
    }

    write_clan_list();
}

/**
 * An immortal command that will remove a clan.
 *
 * @author  Joel McBeth
 * @version 1.0     02/14/05 
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.  
 */
void do_clanremove(CHAR_DATA* ch, char* args)
{
    CLAN_DATA* clan;

    if (IS_NPC(ch) == TRUE)
    {
        ch_printf(ch, "You can't do that.\r\n");
        return;
    }    

    if (ch->desc == NULL)
    {
        ch_printf(ch, "You can't do that right now.\r\n");
        return;
    }

    if (args[0] == '\0')
    {
        ch_printf(ch, "Remove what clan?\r\n");
        return;
    }

    clan = get_clan(args);
    if (clan == NULL)
    {
        ch_printf(ch, "You can't remove a clan that doesn't exist.\r\n");
        return;
    }

    remove_clan(clan);

    ch_printf(ch, "OK.\r\n");;
}

/**
 * An immortal command that will create a new clan.
 *
 * @author  Joel McBeth
 * @version 1.0     02/14/05 
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.  
 */
void do_clancreate(CHAR_DATA* ch, char* args)
{
    CLAN_DATA* clan;

    if (IS_NPC(ch) == TRUE)
    {
        ch_printf(ch, "You can't do that.\r\n");
        return;
    }    

    if (ch->desc == NULL)
    {
        ch_printf(ch, "You can't do that right now.\r\n");
        return;
    }

    if (args[0] == '\0')
    {
        ch_printf(ch, "Create what clan? %s\r\n", args);
        return;
    }

    for (clan = first_clan; clan != NULL; clan = clan->next)
    {

        if (str_cmp(args, clan->name) == FALSE)
        {
           ch_printf(ch, "The clan already exist.\r\n");
           return;
        }
    }

    clan = create_clan(args);

    ch_printf(ch, "OK.\r\n");
}

/**
 * This gets a list of the characters support in a formatted list.
 * * @author  Joel McBeth
 * @version 1.0     02/11/05
 *
 * @param   ch      The character that has the list of supporters.
 *
 * @returns A string containing a comma formated list of the characters supporters.
 */
char* get_supporters_string(CHAR_DATA* ch)
{
    CHAR_DATA* char_ref;
    static char buf[MAX_STRING_LENGTH];

    /* Make sure the character isn't NULL. */
    if (ch == NULL)
    {
        bug("get_supporters_string: NULL ch");
        return NULL;
    }

    buf[0] = '\0';

    /* Get all the supporters for the clan. */
    for (char_ref = first_char; char_ref != NULL; char_ref = char_ref->next)
    {
       /**
        * Make sure you ignore the characer founding the clan, ignore NPC characters, and then
        * add the name to the list of supporters.
        */
        if ((char_ref != ch) &&
            (IS_NPC(char_ref) == FALSE) &&
            (str_cmp(ch->name, char_ref->pcdata->supports) == FALSE))
        {
            /* Don't put a comma if it is the first name. */
            if (buf[0] != '\0')
                strcat(buf, ", ");

            strcat(buf, char_ref->name);                                
        }
    }      

    if (buf[0] == '\0')
        strcpy(buf, "no one");

    return buf;
}

/**
 * A command that will create a clan if there are enough supporters.
 * It will take an argument that will be the name of the clan, then it
 * will go to the editor so you can write the clan description.
 * Supporters with similar hostnames will only count once.
 *
 * @author  Joel McBeth
 * @version 1.0     02/08/05
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.
 */
void do_found(CHAR_DATA* ch, char* argument)
{    
    CHAR_DATA* temp_ch;
    CLAN_DATA* clan;
    CLAN_DATA* clan_ref;
    char buf[MAX_STRING_LENGTH];
    int num_supporters = 0;
    MEMBER_DATA* member;

    /* This is to handle being in the clan description editor. */
    switch(ch->substate)
    {        
        default:            
            break;

        case SUB_CLANDESC:
            clan = ch->dest_buf;

            /* A serious problem has occured. */
            if (clan == NULL)
            {
                bug("do_found: sub_clandesc: NULL ch->dest_buf", 0);
                stop_editing(ch);
                ch->substate = ch->tempnum;
                send_to_char("&RError: clan lost.\n\r" , ch);
                return;
            }

            /* This probably doesn't need to be hashed. */
            STRFREE(clan->description);

            clan->description = copy_buffer(ch);
            stop_editing(ch);

            ch->substate = ch->tempnum;

            /* I don't see why not? */
            ch->dest_buf = NULL;

            save_clan(clan);
            return;
    }

    /* Mobs can't found clans. */
    if (IS_NPC(ch) == TRUE)
    {
        ch_printf(ch, "Huh?\r\n");
        return;
    }

    /* Link-dead people can't found clans either. */
    if (ch->desc == NULL)
    {
        ch_printf(ch, "You can't do that right now.\r\n");
        return;
    }

    /* Make sure that you aren't already in a clan. */
    if (ch->pcdata->clan != NULL)
    {
        ch_printf(ch, "You already are in a clan.\r\n");
        return;
    }

    if (argument[0] == '\0')
    {
        ch_printf(ch, "You must give your clan a name.\r\n");
        return;
    }

    if (is_clan_name_valid(argument) == FALSE)
    {
        ch_printf(ch, "That is not a valid name for a clan.\r\n");
        return;
    }

    /* So you don't have to mess with buffer overflows. */
    if (strlen(argument) > 80)
    {
        ch_printf(ch, "The clan name can be at most 80 characters long.\r\n");
        return;
    }

    /* Get all the supporters for the clan. */
    for (temp_ch = first_char; temp_ch; temp_ch = temp_ch->next)
    {
        /**
         * Make sure you ignore the character founding the clan, ignore NPC characters, and then
         * check to see if the person who the character supports is the character founding the
         * clan.
         */
        if ((temp_ch != ch) &&
            (!IS_NPC(temp_ch)) &&
            (!str_cmp(ch->name, temp_ch->pcdata->supports)))
        {            
            num_supporters++;
        }
    }

    /* Make sure you have enough supporters. */
    if (num_supporters < MIN_SUPPORTERS)
    {
        ch_printf(ch, "You don't have enough supporters, you need at least %d supporters.\r\n", MIN_SUPPORTERS);
        return;
    }

    /* Make sure a clan with that name doesn't already exist. */
    for (clan_ref = first_clan; clan_ref != NULL; clan_ref = clan_ref->next)
    {
        if (str_cmp(clan_ref->name, argument) == FALSE)
        {
            ch_printf(ch, "That clan already exist.\r\n");
            return;
        }
    }

    clan = create_clan(argument);

    add_member(clan, ch);
    
    member = get_member(ch);

    if (member == NULL)
    {
        sprintf("do_found: unable to get member data for %s.", ch->name);
        bug(buf);
        return;
    }

    member->rank == RANK_LEADER;


    /* Save a few bytes of memory. */
    STRFREE(ch->pcdata->supports);
    ch->pcdata->supports = STRALLOC("");

    /* Add all the supporters to the clan. */
    for (temp_ch = first_char; temp_ch; temp_ch = temp_ch->next)
    {
        /**
         * Make sure you ignore the charatcer founding the clan, ignore NPC characters, and then
         * add all the supporters to the clan.
         */
        if ((temp_ch != ch) &&
            (IS_NPC(temp_ch) == FALSE) &&
            (str_cmp(ch->name, temp_ch->pcdata->supports) == FALSE))
        {   
            add_member(clan, ch);

            /* Reset who they support to nothing. */
            STRFREE(temp_ch->pcdata->supports);
            temp_ch->pcdata->supports = STRALLOC("");
        }
    }   

    sprintf(buf, "The clan %s has been formed.", clan->name);
    echo_to_all(AT_WHITE, buf, ECHOTAR_ALL);

    ch->substate = SUB_CLANDESC;
    ch->dest_buf = clan;
    start_editing(ch, clan->description);
}

/**
 * A command that will give support to a player so they can use found.
 * You will not be able to support someone with a similar hostname.
 *
 * @author  Joel McBeth
 * @version 1.1     02/12/05
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.
 */
void do_support(CHAR_DATA* ch, char* argument)
{
    CHAR_DATA* victim;
    CHAR_DATA* old_victim;
    CHAR_DATA* char_ref;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    /* NPCs definatly can not support someone. */
    if (IS_NPC(ch) == TRUE)
    {
        ch_printf(ch, "Huh?\r\n");
        return;
    }

    /* You can't support anyone if you are link-dead. */
    if (ch->desc == NULL)
    {
        ch_printf(ch, "You can't do that right now.\r\n");
        return;
    }             

    /* Make sure they put arguments. */
    if (argument[0] == '\0')
    {
        ch_printf(ch, "Support who?\r\n");
        return;
    }
    
    if (str_cmp(argument, "list") == FALSE)
    {
        buf[0] = '\0';

         /* Get all the supporters for the clan. */
        for (char_ref = first_char; char_ref != NULL; char_ref = char_ref->next)
        {
            /**
            * Make sure you ignore the characer founding the clan, ignore NPC characters, and then
            * add the name to the list of supporters.
            */
            if ((char_ref != ch) &&
                (IS_NPC(char_ref) == FALSE) &&
                (str_cmp(ch->name, char_ref->pcdata->supports) == FALSE))
            {
                /* Don't put a comma if it is the first name. */
                if (buf[0] != '\0')
                    strcat(buf, ", ");

                strcat(buf, char_ref->name);                                
            }
        }      

        /* Display who your support and who your supporters are, if you support no one it will say No one. */
        ch_printf(ch, "You support %s, and your supporters are:\r\n\t%s\r\n", (ch->pcdata->supports[0] != '\0') ? ch->pcdata->supports : "no one", (buf[0] != '\0') ? buf : "no one");
        return;
    }

    /* If you don't want to support someone anymore. */
    if ((str_cmp(argument, "none") == FALSE) ||
        (str_cmp(argument, "no one") == FALSE) ||
        (str_cmp(argument, "noone") == FALSE))
    {                        
        old_victim = get_char_world(ch, ch->pcdata->supports);
        if (old_victim != NULL)
        {
            ch_printf(old_victim, "%s no longer supports you.\r\n", ch->name);
        }

        STRFREE(ch->pcdata->supports);
        ch->pcdata->supports = STRALLOC("");

        ch_printf(ch, "OK.\r\n");

        return;
    }

    /* If you are in a clan you can't support anyone. */
    if (ch->pcdata->clan != NULL)
    {
        ch_printf(ch, "You already support a clan.\r\n");
        return;
    }

    /* Make sure they don't already support someone. */
    if (ch->pcdata->supports[0] != '\0')
    {
        ch_printf(ch, "You already support someone.\r\n");
        return;
    }

    argument = one_argument(argument, arg1);
        
    /* Make sure the character is in the room. */
    victim = get_char_room(ch, arg1);
    if (victim == NULL)
    {
        ch_printf(ch, "You don't see that person here.\r\n");
        return;
    }

    /* Make sure they are not trying to support themselves. */
    if (victim == ch)
    {
        ch_printf(ch, "It is good to have high self esteem.\r\n");
        return;
    }

    /* You can't support someone who is link-dead. */
    if (victim->desc == NULL)
    {
        ch_printf(ch, "You can't support them right now.\r\n");
        return;
    }

    if (IS_NPC(victim) == TRUE)
    {
        ch_printf(ch, "You can't support them.\r\n");
        return;
    }

    if ((victim->pcdata->clan != NULL) ||
        (victim->pcdata->supports[0] != '\0'))
    {
        ch_printf(ch, "They already support someone.\r\n");
        return;
    }

    /* If victim has a very similar hostname they will not be allowed to support. */
    if (same_host(victim->desc->host, ch->desc->host) == TRUE)
    {
        ch_printf(ch, "You may not support them.\r\n");
        return;
    }

    /* Make sure the character doesn't have a similar hostname to other supporters either. */
    for (char_ref = first_char; char_ref != NULL; char_ref = char_ref->next)
    {
        if ((IS_NPC(char_ref) == FALSE) &&
            (same_host(victim->desc->host, ch->desc->host) == TRUE))
        {
            ch_printf(ch, "You may not support them.\r\n");
            return;
        }
    }

    ch->pcdata->supports = STRALLOC(victim->name);

    act(AT_PLAIN, "You show your support for $N.", ch, NULL, victim, TO_CHAR);
    act(AT_PLAIN, "$n shows $s support to $N.", ch, NULL, victim, TO_NOTVICT);
    act(AT_PLAIN, "$n shows $s support.", ch, NULL, victim, TO_VICT);
}

/**
 * A command that opens up the editor so you can set the clan description.
 *
 * @author  Joel McBeth
 * @version 1.0     02/08/05
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.
 */
void do_clandescription(CHAR_DATA* ch, char* argument)
{
    CLAN_DATA* clan;    

    /* NPC's, people without clans, and link-dead people can't use this command. */
    if ((IS_NPC(ch) == TRUE) ||
        ((IS_NPC(ch) == FALSE) && (ch->pcdata->clan == NULL)) ||
        (ch->desc == NULL))
    {
        ch_printf(ch, "Huh?\r\n");
        return;
    }
    
    clan = ch->pcdata->clan;

    /* This is to handle being in the clan description editor. */
    switch(ch->substate)
    {
        default:            
            break;

        case SUB_CLANDESC:
            clan = ch->dest_buf;

            /* A serious problem has occured. */
            if (clan == NULL)
            {
                bug("do_clandescription: sub_clandesc: NULL ch->dest_buf", 0);
                stop_editing(ch);
                ch->substate = ch->tempnum;
                send_to_char("&RError: clan lost.\n\r" , ch);
                return;
            }

            /* This probably doesn't need to be hashed. */
            STRFREE(clan->description);

            clan->description = copy_buffer(ch);
            stop_editing(ch);

            ch->substate = ch->tempnum;

            /* I don't see why not? */
            ch->dest_buf = NULL;

            save_clan(clan);
            return;
    }

    if (is_leader(ch))
    {
        ch_printf(ch, "You must be a leader.\r\n");
        return;
    }

    ch->substate = SUB_CLANDESC;
    ch->dest_buf = clan;
    start_editing(ch, clan->description);
}

/**
 * A command that will show all the current members of the clan and their activity status.
 *
 * @author      Joel McBeth
 * @version     1.0     02/21/05
 *
 * @param       ch      The character that called the command.
 * @param       args    The arguments supplied with the command.
 */
void do_clanmembers(CHAR_DATA* ch, char* args)
{
   MEMBER_DATA* member;
   CLAN_DATA* clan;

   if (IS_NPC(ch))
   {
      ch_printf(ch, "You can't do that.\r\n");
      return;
   }

   if (ch->desc == NULL)
   {
      ch_printf(ch, "You can't do that right now.\r\n");
      return;
   }

   clan = ch_get_clan(ch);
   if (clan == NULL)
   {
      ch_printf(ch, "You aren't in a clan.\r\n");
      return;
   }

   if (!is_leader(ch))
   {
      ch_printf(ch, "You must be a leader to do that.\r\n");
      return;
   }

   ch_printf(ch, "&W%-66s%-8s%s\r\n", "Player", "Rank", "Status&G");

   for (member = clan->first_member; member != NULL; member = member->next)
   {
      ch_printf(ch, "&W%s-66s%-8s%s\r\n", member->name, get_rank_string(ch), get_activity_string(ch));
   }
}

/**
 * Gets the number of seconds since the member last logged off.
 *
 * @author      Joel McBeth
 * @version     1.0     02/23/05
 *
 * @param       member  The member to get the activity of.
 *
 * @returns     The number of seconds since the member last logged off.
 */
int get_member_activity(MEMBER_DATA* member)
{
    if (member == NULL)
    {
        bug("get_member_activity: NULL member.", 0);
        return;
    }

    return (current_time - member->last_played);
}

/**
 * Gets a string representation of a members's activity.
 *
 * @author      Joel McBeth
 * @version     1.0     02/23/05
 *
 * @param       member  The member to get the activity of.
 *
 * @returns     A string that represents the activity of the member.
 */
char* get_member_activity_string(MEMBER_DATA* member)
{
    static char buf[MAX_STRING_LENGTH];
    int activity;

    if (member == NULL)
    {
        bug("get_member_activity_string: NULL member.", 0);
        return;
    }

    activity = get_member_activity(member);

    if (activity >= MEMBER_TIME_TILL_INACTIVE)    
        strcpy(buf, "Inactive");
    else
        strcpy(buf, "Active");

    return buf;
}

/**
 * Gts a string representation of a clan's activity.
 *
 * @author      Joel McBeth
 * @version     1.0     02/23/05
 *
 * @param       clan    The clan to get the activity of.
 *
 * @returns     A string that represents the activity of the clan.
 */
char* get_clan_activity_string(CLAN_DATA* clan)
{
    static char buf[MAX_STRING_LENGTH];
    int activity;

    if (clan == NULL)
    {
        bug("get_clan_activity_string: NULL clan.", 0);
        return;
    }

    activity = get_clan_activity(clan);
    
    if (activity >= CLAN_TIME_TILL_DEAD
        strcpy(buf, "Dead");
    else if (activity >= CLAN_TIME_TILL_INACTIVE)
        strcpy(buf, "Inactive");
    else
        strcpy(buf, "Active");

    return buf;
}

/**
 * Gets the time in seconds since the last member has logged off.
 *
 * @author      Joel McBeth
 * @version     1.0     02/23/05
 *
 * @param       clan    The clan to get the activity of.
 *
 * @returns     The number of seconds since a clan member has logged off.
 */
int get_clan_activity(CLAN_DATA* clan)
{
    int last;
    int i;
    MEMBER_DATA* member;

    if (clan == NULL)
    {
        bug("get_clan_activity: NULL clan.", 0);
        return;
    }

    activity = 0;

    /* Search for the most recently active player. */
    for (member = clan->first_member; member != NULL; member = member->next)
    {        
        if (member->last_played > activity)
            last = member->last_played;
    }

    return (current_time - last);
}

/**
 * Gets the age of the clan in game years.
 *
 * @author  Joel McBeth
 * @version 1.0     02/22/05
 *
 * @param   clan    The clan to get the age of.
 *
 * @returns The age of the clan in game years.
 */
int get_clan_age(CLAN_DATA* clan)
{
    if (clan == NULL)
    {
        bug("get_clan_age: NULL clan.", 0);
        return -1;
    }

    /* Get how long the clan has existed and then convert it to game years. */
    return (clan->created - current_time) / (3600 * HOURS_PER_GAMEYEAR);
}

/**
 * Checks if a clan is at war with another clan.
 *
 * @author  Joel McBeth
 * @version 1.0         02/22/05
 *
 * @param   clan        The clan to see if it is wared with the other clan.
 * @param   checkclan   The clan to check if it is warred by the other clan.
 *
 * @returns TRUE if the clan is warred with the clan to check, FALSE otherwise.
 */
bool clan_is_atwar(CLAN_DATA* clan, CLAN_DATA* checkclan)
{
    if (clan == NULL)
    {
        bug("clan_is_atwar: NULL clan", 0);
        return;
    }

    if (checkclan == NULL)
    {
        bug("clan_is_atwar: NULL checkclan", 0);
        return;
    }

    if (isname(clan->atwar, checkclan->name) == TRUE)    
        return TRUE;    

    return FALSE;
}

/**
 * Checks if a character is a leader their clan.
 *
 * @author  Joel McBeth
 * @version 1.0      03/11/05
 *
 * @param   clan     The clan to see if the character is a leader of.
 * @param   ch       The character to check if it is the leader of the clan.
 *
 * @returns TRUE if the character is a leader of the clan, FALSE otherwise.
 */
bool is_leader(CHAR_DATA* ch)
{
   MEMBER_DATA* member;   

   if (ch == NULL)
   {
      bug("is_leader: NULL ch.", 0);
      return FALSE;
   }
   
   member = get_member(ch);

   /* Make sure the character has a clan. */
   if (member == NULL)
       return FALSE;

   /* Check if they are a leader. */
   if (member->rank == RANK_LEADER)
      return TRUE;

   /* The character isn't a member or they aren't a leader. */
   return FALSE;
}

/**
 * Checks if a character is a member of a clan.
 *
 * @author  Joel McBeth
 * @version 1.0   03/12/05
 *
 * @param   clan  The clan to see if the character is a member of.
 * @param   ch    The character to see if it is the member of the clan.
 *
 * @returns TRUE if the character is a member, FALSE if they are not.
 */
bool is_member(CLAN_DATA* clan, CHAR_DATA* ch)
{
   MEMBER_DATA* member;
   CLAN_DATA* ch_clan;

   if (clan == NULL)
   {
      bug("is_member: NULL clan.", 0);
      return FALSE;
   }

   if (ch == NULL)
   {
      bug("is_member: NULL ch.", 0);
      return FALSE;
   }   
   
   ch_clan = get_ch_clan(ch);

   /**
    * Don't need to check if ch_clan is NULL because clan can't be NULL and if
    * ch_clan is NULL they wont equal and the function defaults to FALSE. The
    * get_member() function could be used also but this is faster, doubt it would
    * make a difference, though.
    */
   if (clan == ch_clan)
       return TRUE;
   
   return FALSE;
}

/**
 * Checks if a character has a certain rank in his clan.
 *
 * @author  Joel McBeth
 * @version 1.0   03/12/05
 *
 * @param   ch      The character to check if they have a certain rank.
 * @param   rank    The rank to check if the character has.
 *
 * @returns TRUE if the character has that rank, FALSE if they don't, or if they are an NPC or don't have a clan.
 */
bool is_rank(CHAR_DATA* ch, sh_int rank)
{
    MEMBER_DATA* member;

    if (ch == NULL)
    {
        bug("is_rank: NULL ch.", 0);
        return FALSE;
    }

    member = get_member(ch);

    /* Check if the member was in a clan. */
    if (member == NULL)
        return FALSE;

    if (member->rank == rank)
        return TRUE;

    return FALSE;
}

/**
 * Gets a character's rank.
 *
 * @author  Joel McBeth
 * @version 1.0   07/28/05
 *
 * @param   ch      The character to get their rank of.
 *
 * @returns An integer value that represents their rank.
 */
int get_rank(CHAR_DATA* ch)
{
    MEMBER_DATA* member;

    if (ch == NULL)
    {
        bug("get_rank: NULL ch.", 0);
        return RANK_NONE;
    }

    member = get_member(ch);

    /* Not in a clan. */
    if (member == NULL)
        return RANK_NONE;

    return member->rank;
}

/**
 * Safely gets the clan of a character.
 * This takes into consideration if the character is an NPC or if for some
 * reason the pcdata is NULL or even if the character itself is NULL.
 *
 * @author  Joel McBeth
 * @version 1.0   03/12/05
 *
 * @param   ch    The character to get the clan of.
 *
 * @returns The character's clan, NULL if the character doesn't have a clan.
 */
CLAN_DATA* get_ch_clan(CHAR_DATA* ch)
{   
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL)      
    {
        bug("ch_get_clan: NULL ch.", 0);
        return NULL;
    }

    /* NPCs don't have clans. */
    if (IS_NPC(ch))    
        return NULL;    

    /* I think IS_NPC checks if pcdata is NULL to see if its an NPC. */
    if (ch->pcdata == NULL)
    {
        sprintf(buf, "ch_get_clan: %s has NULL pcdata", ch->name);
        bug(buf);
        return NULL;
    }

    return ch->pcdata->clan;  
}

/**
 * Gets a string representation of a character's clan powers.
 *
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch      The character that has the powers.
 *
 * @returns     A string that represents the character's powers or a blank string if there was a problem.
 */
char* get_powers_string(CHAR_DATA* ch)
{
    static char buf[MAX_STRING_LENGTH];    
    MEMBER_DATA* member;
    int i;

    /* Start with a blank string. */
    buf[0] = '\0';

    if (ch == NULL)
    {
        bug("get_powers_string: NULL ch.", 0);
        return buf;
    }

    member = get_member(ch);
    
    /* Make sure the character has a clan. */
    if (member == NULL)
    {
        bug("get_powers_string: ch isn't in a clan.", 0);
        return buf;
    }    

    for (i = 0; i < POWER_MAX_FLAGS; i++)
    {
        if (IS_SET(member->powers, 1 << i))        
            sprintf(buf, "%s%s,", buf, power_flags[i]);        
    }

    /* Get rid of the comma on the end. */
    buf[strlen(buf) - 1] = '\0';

    return buf;
}

/**
 * Gets a string representation of a character's clan powers.
 *
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch      The name of the character.
 *
 * @returns     A string that represents the character's powers or a blank string if there was a problem.
 */
char* get_powers_string_name(char* name)
{
    static char buf[MAX_STRING_LENGTH];    
    MEMBER_DATA* member;
    int i;

    /* Start with a blank string. */
    buf[0] = '\0';

    if (!name || name[0] == '\0')
    {
        bug("get_powers_string_name: NULL or blank name.", 0);
        return buf;
    }

    member = get_member_name(name);
    
    /* Make sure the character has a clan. */
    if (member == NULL)
    {
        bug("get_powers_string: ch isn't in a clan.", 0);
        return buf;
    }    

    for (i = 0; i < POWER_MAX_FLAGS; i++)
    {
        if (IS_SET(member->powers, 1 << i))        
            sprintf(buf, "%s%s,", buf, power_flags[i]);        
    }

    /* Get rid of the comma on the end. */
    buf[strlen(buf) - 1] = '\0';

    return buf;
}

/**
 * Gets a string representation of all ranks.
 *
 * @author      Joel McBeth
 * @version     1.0     07/28/05 
 *
 * @returns     A string that is a list of all possible ranks.
 */
char* get_ranks_string()
{
    static char buf[MAX_STRING_LENGTH];
    int i;

    /* Start with a blank string. */
    buf[0] = '\0';

    for (i = 0; i <= MAX_RANK; i++)    
        sprintf(buf, "%s%s,", buf, rank_names[i]);            

    /* Get rid of the comma on the end. */
    buf[strlen(buf) - 1] = '\0';

    return buf;
}

/**
 * Converts a string to a rank.
 *
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       rankstr The string to convert.
 *
 * @returns     A rank.
 */
int get_ranks_from_string(char* rankstr)
{
    int i;

    if (!rankstr)
        return RANK_NONE;

    for (i = 0; i <= MAX_RANK; i++)
    {
        if (!str_cmp(rankstr, rank_names[i]))
            return i;
    }

    return RANK_NONE;
}

/**
 * Gets a string representation of a rank.
 *
 * @author      Joel McBeth
 * @version     1.0     02/23/05
 *
 * @param       member  The member to get the rank string of.
 *
 * @returns     A string that represents the member's rank.
 */
char* get_member_rank_string(MEMBER_DATA* member)
{
    static char buf[MAX_STRING_LENGTH];    

    if (member == NULL)
    {
        bug("get_member_rank_string: NULL member.", 0);
        return;
    }

    strcpy(buf, rank_names[member->rank]);

    return buf;
}

/**
 * Checks if a character has a certain power.
 *
 * @author      Joel McBeth
 * @version     1.0     07/18/05
 *
 * @param       ch          The character to check if they have the power.
 * @param       empowerment The power to check.
 *
 * @returns     TRUE if the character is empowered with the specified power, FALSE otherwise.
 */
bool is_empowered(CHAR_DATA* ch, long int power)
{
    MEMBER_DATA* member;
    CLAN_DATA* clan;

    if (ch == NULL)
    {
        bug("is_empowered: NULL ch", 0);
        return;
    }

    clan = get_ch_clan(ch);
    
    /* Make sure they character has a clan. */
    if (clan == NULL)
        return FALSE;

    member = get_member(ch);

    /* Shouldn't happen unless some kind of wierd error occured. */
    if (member == NULL)    
        return FALSE;

    /* Leaders have all powers. */
    if (member->rank == MAX_RANK)
        return TRUE;

    if (IS_SET(member->powers, power))
        return TRUE;

    return FALSE;
}

/**
 * Checks if the clan has any leaders.
 *
 * @author      Joel McBeth
 * @version     1.0     07/18/05
 *
 * @param       clan    The clan to check if it has any leaders.
 *
 * @returns     True if the clan has at least one leader, FALSE if it has no leaders.
 */
bool has_leaders(CLAN_DATA* clan)
{
    MEMBER_DATA* member;

    if (clan == NULL)
    {
        bug("has_leaders: NULL clan.", 0);
        return;
    }

    for (member = clan->first_member; member != NULL; member = member->next)
    {
        if (member->rank == RANK_LEADER)
            return TRUE;
    }

    return FALSE;
}

/**
 * Counts all the members in a clan.
 * Should use clan->members unless using this function to check that
 * value for acuracy.
 *
 * @author      Joel McBeth
 * @version     1.0     07/18/05
 *
 * @param       clan    The clan to count the members of.
 *
 * @returns     The number of members in a clan.
 */
int member_count(CLAN_DATA* clan)
{
    MEMBER_DATA* member;
    int count;

    if (clan == NULL)
    {
        bug("member_count: NULL clan.", 0);
        return;
    }

    count = 0;
    for (member = clan->first_member; member != NULL; member = member->next)    
        count++;    

    return count;
}

/**
 * Gets a string list of all the leaders.
 *
 * @author      Joel McBeth
 * @version     1.0     07/18/05
 *
 * @param       clan    The clan to get the leaders of.
 *
 * @returns     A string with all the leaders.
 */
char* get_leaders_string(CLAN_DATA* clan)
{
    static char buf[MAX_STRING_LENGTH];
    MEMBER_DATA* member;

    buf[0] = '\0';

    if (clan == NULL)
    {
        bug("get_leaders_string: NULL clan.", 0);
        return buf;
    }

    for (member = clan->first_member; member != NULL; member = member->next)
    {
        if (member->rank == RANK_LEADER)        
            sprintf(buf, "%s%s,", buf, member->name);        
    }

    /* Remove the comma at the end. */
    buf[strlen(buf) - 1] = '\0';

    return buf;
}