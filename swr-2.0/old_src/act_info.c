#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "mud.h"


char *	const	where_name	[] =
{
    "<used as light>     ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on body>      ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<energy shield>     ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<wielded>           ",
    "<held>              ",
    "<dual wielded>      ",
    "<worn on ears>      ",
    "<worn on eyes>      ",
    "<missile wielded>   "
};


/*
 * Local functions.
 */
void	show_char_to_char_0	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char_1	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char	args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
void	show_char_to_char	args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
void	show_ships_to_char	args( ( SHIP_DATA *ship, CHAR_DATA *ch ) );
bool	check_blind		args( ( CHAR_DATA *ch ) );
void    show_condition          args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool 	is_online		args( ( char * argument ) );


char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';
    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );
    if ( ( IS_AFFECTED(ch, AFF_DETECT_MAGIC) || IS_IMMORTAL(ch) )
	 && IS_OBJ_STAT(obj, ITEM_MAGIC)  )   strcat( buf, "&B(Blue Aura)&w "   );
    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "(Glowing) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HIDDEN)	  )   strcat( buf, "(Hidden) "	  );
    if ( IS_OBJ_STAT(obj, ITEM_BURRIED)	  )   strcat( buf, "(Burried) "	  );
    if ( IS_IMMORTAL(ch)
	 && IS_OBJ_STAT(obj, ITEM_PROTOTYPE) ) strcat( buf, "(PROTO) "	  );

    if ( fShort )
    {
	if ( obj->short_descr )
	    strcat( buf, obj->short_descr );
    }
    else
    {
	if ( obj->description )
	    strcat( buf, obj->description );
    }

    return buf;
}


/*
 * Some increasingly freaky halucinated objects		-Thoric
 */
char *halucinated_object( int ms, bool fShort )
{
    int sms = URANGE( 1, (ms+10)/5, 20 );

    if ( fShort )
    switch( number_range( 6-URANGE(1,sms/2,5), sms ) )
    {
	case  1: return "a sword";
	case  2: return "a stick";
	case  3: return "something shiny";
	case  4: return "something";
	case  5: return "something interesting";
	case  6: return "something colorful";
	case  7: return "something that looks cool";
	case  8: return "a nifty thing";
	case  9: return "a cloak of flowing colors";
	case 10: return "a mystical flaming sword";
	case 11: return "a swarm of insects";
	case 12: return "a deathbane";
	case 13: return "a figment of your imagination";
	case 14: return "your gravestone";
	case 15: return "the long lost boots of Ranger Thoric";
	case 16: return "a glowing tome of arcane knowledge";
	case 17: return "a long sought secret";
	case 18: return "the meaning of it all";
	case 19: return "the answer";
	case 20: return "the key to life, the universe and everything";
    }
    switch( number_range( 6-URANGE(1,sms/2,5), sms ) )
    {
	case  1: return "A nice looking sword catches your eye.";
	case  2: return "The ground is covered in small sticks.";
	case  3: return "Something shiny catches your eye.";
	case  4: return "Something catches your attention.";
	case  5: return "Something interesting catches your eye.";
	case  6: return "Something colorful flows by.";
	case  7: return "Something that looks cool calls out to you.";
	case  8: return "A nifty thing of great importance stands here.";
	case  9: return "A cloak of flowing colors asks you to wear it.";
	case 10: return "A mystical flaming sword awaits your grasp.";
	case 11: return "A swarm of insects buzzes in your face!";
	case 12: return "The extremely rare Deathbane lies at your feet.";
	case 13: return "A figment of your imagination is at your command.";
	case 14: return "You notice a gravestone here... upon closer examination, it reads your name.";
	case 15: return "The long lost boots of Ranger Thoric lie off to the side.";
	case 16: return "A glowing tome of arcane knowledge hovers in the air before you.";
	case 17: return "A long sought secret of all mankind is now clear to you.";
	case 18: return "The meaning of it all, so simple, so clear... of course!";
	case 19: return "The answer.  One.  It's always been One.";
	case 20: return "The key to life, the universe and everything awaits your hand.";
    }
    return "Whoa!!!";
}


/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char **prgpstrShow;
    int *prgnShow;
    int *pitShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count, offcount, tmp, ms, cnt;
    bool fCombine;

    if ( !ch->desc )
	return;

    /*
     * if there's no list... then don't do all this crap!  -Thoric
     */
    if ( !list )
    {
    	if ( fShowNothing )
    	{
	   if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) )
	      send_to_char( "     ", ch );
	   send_to_char( "Nothing.\n\r", ch );
	}
	return;
    }
    /*
     * Alloc space for output lines.
     */
    count = 0;
    for ( obj = list; obj; obj = obj->next_content )
	count++;

    ms  = (ch->mental_state ? ch->mental_state : 1)
	* (IS_NPC(ch) ? 1 : (ch->pcdata->condition[COND_DRUNK] ? (ch->pcdata->condition[COND_DRUNK]/12) : 1));

    /*
     * If not mentally stable...
     */
    if ( abs(ms) > 40 )
    {
	offcount = URANGE( -(count), (count * ms) / 100, count*2 );
	if ( offcount < 0 )
	  offcount += number_range(0, abs(offcount));
	else
	if ( offcount > 0 )
	  offcount -= number_range(0, offcount);
    }
    else
	offcount = 0;

    if ( count + offcount <= 0 )
    {
    	if ( fShowNothing )
    	{
	   if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) )
	      send_to_char( "     ", ch );
	   send_to_char( "Nothing.\n\r", ch );
	}
	return;
    }

    CREATE( prgpstrShow,	char*,	count + ((offcount > 0) ? offcount : 0) );
    CREATE( prgnShow,		int,	count + ((offcount > 0) ? offcount : 0) );
    CREATE( pitShow,		int,	count + ((offcount > 0) ? offcount : 0) );
    nShow	= 0;
    tmp		= (offcount > 0) ? offcount : 0;
    cnt		= 0;

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj; obj = obj->next_content )
    {
	if ( offcount < 0 && ++cnt > (count + offcount) )
	    break;
	if ( tmp > 0 && number_bits(1) == 0 )
	{
	    prgpstrShow [nShow] = str_dup( halucinated_object(ms, fShort) );
	    prgnShow	[nShow] = 1;
	    pitShow	[nShow] = number_range( 0, MAX_ITEM_TYPE );
	    nShow++;
	    --tmp;
	}
	if ( obj->wear_loc == WEAR_NONE
	&& can_see_obj( ch, obj ) )
	{
	    pstrShow = format_obj_to_char( obj, ch, fShort );
	    fCombine = FALSE;

	    if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) )
	    {
		/*
		 * Look for duplicates, case sensitive.
		 * Matches tend to be near end so run loop backwords.
		 */
		for ( iShow = nShow - 1; iShow >= 0; iShow-- )
		{
		    if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
		    {
			prgnShow[iShow] += obj->count;
			fCombine = TRUE;
			break;
		    }
		}
	    }

	    pitShow[nShow] = obj->item_type;
	    /*
	     * Couldn't combine, or didn't want to.
	     */
	    if ( !fCombine )
	    {
		prgpstrShow [nShow] = str_dup( pstrShow );
		prgnShow    [nShow] = obj->count;
		nShow++;
	    }
	}
    }
    if ( tmp > 0 )
    {
	int x;
	for ( x = 0; x < tmp; x++ )
	{
	    prgpstrShow [nShow] = str_dup( halucinated_object(ms, fShort) );
	    prgnShow	[nShow] = 1;
	    pitShow	[nShow] = number_range( 0, MAX_ITEM_TYPE );
	    nShow++;
	}
    }

    /*
     * Output the formatted list.		-Color support by Thoric
     */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
	switch(pitShow[iShow]) {
	default:
	  set_char_color( AT_OBJECT, ch );
	  break;
	case ITEM_MONEY:
	  set_char_color( AT_YELLOW, ch );
	  break;
	case ITEM_FOOD:
	  set_char_color( AT_HUNGRY, ch );
	  break;
	case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
	  set_char_color( AT_THIRSTY, ch );
	  break;
	}
	if ( fShowNothing )
	    send_to_char( "     ", ch );
	send_to_char( prgpstrShow[iShow], ch );
/*	if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) ) */
	{
	    if ( prgnShow[iShow] != 1 )
		ch_printf( ch, " (%d)", prgnShow[iShow] );
	}

	send_to_char( "\n\r", ch );
	DISPOSE( prgpstrShow[iShow] );
    }

    if ( fShowNothing && nShow == 0 )
    {
	if ( IS_NPC(ch) || IS_SET(ch->act, PLR_COMBINE) )
	    send_to_char( "     ", ch );
	send_to_char( "Nothing.\n\r", ch );
    }

    /*
     * Clean up.
     */
    DISPOSE( prgpstrShow );
    DISPOSE( prgnShow	 );
    DISPOSE( pitShow	 );
    return;
}


/*
 * Show fancy descriptions for certain spell affects		-Thoric
 */
void show_visible_affects_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
        if ( IS_GOOD(victim) )
        {
            set_char_color( AT_WHITE, ch );
            ch_printf( ch, "%s glows with an aura of divine radiance.\n\r",
		IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
        }
        else if ( IS_EVIL(victim) )
        {
            set_char_color( AT_WHITE, ch );
            ch_printf( ch, "%s shimmers beneath an aura of dark energy.\n\r",
		IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
        }
        else
        {
            set_char_color( AT_WHITE, ch );
            ch_printf( ch, "%s is shrouded in flowing shadow and light.\n\r",
		IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
        }
    }
    if ( IS_AFFECTED(victim, AFF_FIRESHIELD) )
    {
        set_char_color( AT_FIRE, ch );
        ch_printf( ch, "%s is engulfed within a blaze of mystical flame.\n\r",
	    IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
    }
    if ( IS_AFFECTED(victim, AFF_SHOCKSHIELD) )
    {
        set_char_color( AT_BLUE, ch );
	ch_printf( ch, "%s is surrounded by cascading torrents of energy.\n\r",
	    IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
    }
/*Scryn 8/13*/
    if ( IS_AFFECTED(victim, AFF_ICESHIELD) )
    {
        set_char_color( AT_LBLUE, ch );
        ch_printf( ch, "%s is ensphered by shards of glistening ice.\n\r",
	    IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
    }
    if ( IS_AFFECTED(victim, AFF_CHARM)       )
    {
	set_char_color( AT_MAGIC, ch );
	ch_printf( ch, "%s looks ahead free of expression.\n\r",
	    IS_NPC( victim ) ? capitalize(victim->short_descr) : (victim->name) );
    }
    if ( !IS_NPC(victim) && !victim->desc
    &&    victim->switched && IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
	set_char_color( AT_MAGIC, ch );
	strcpy( buf, PERS( victim, ch ) );
	strcat( buf, " appears to be in a deep trance...\n\r" );
    }
}

void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ( IS_NPC(victim) )
      strcat( buf, " "  );
      
    if ( !IS_NPC(victim) && !victim->desc )
    {
	if ( !victim->switched )		strcat( buf, "(Link Dead) "  );
	else
	if ( !IS_AFFECTED(victim->switched, AFF_POSSESS) )
						strcat( buf, "(Switched) " );
    }
    if ( !IS_NPC(victim)
    && IS_SET(victim->act, PLR_AFK) )		strcat( buf, "[AFK] ");        

    if ( (!IS_NPC(victim) && IS_SET(victim->act, PLR_WIZINVIS))
      || (IS_NPC(victim) && IS_SET(victim->act, ACT_MOBINVIS)) ) 
    {
        if (!IS_NPC(victim))
	sprintf( buf1,"(Invis %d) ", victim->pcdata->wizinvis );
        else sprintf( buf1,"(Mobinvis %d) ", victim->mobinvis);
	strcat( buf, buf1 );
    }
    if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
    if ( IS_AFFECTED(victim, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
    if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) strcat( buf, "&P(Pink Aura)&w "  );
    if ( IS_EVIL(victim)
    &&   IS_AFFECTED(ch, AFF_DETECT_EVIL)     ) strcat( buf, "&R(Red Aura)&w "   );
    if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_LITTERBUG  ) )
						strcat( buf, "(LITTERBUG) "  );
    if ( IS_NPC(victim) && IS_IMMORTAL(ch)
	 && IS_SET(victim->act, ACT_PROTOTYPE) ) strcat( buf, "(PROTO) " );
    if ( victim->desc && victim->desc->connected == CON_EDITING )
						strcat( buf, "(Writing) " );

    set_char_color( AT_PERSON, ch );
    if ( victim->position == victim->defposition && victim->long_descr[0] != '\0' )
    {
	strcat( buf, victim->long_descr );
	send_to_char( buf, ch );
	show_visible_affects_to_char( victim, ch );
	return;
    }

   /*   strcat( buf, PERS( victim, ch ) );       old system of titles
    *    removed to prevent prepending of name to title     -Kuran  
    *
    *    But added back bellow so that you can see mobs too :P   -Durga 
    */
    
    if ( !IS_NPC(victim) && !IS_SET(ch->act, PLR_BRIEF) )
	strcat( buf, victim->pcdata->title );
    else
        strcat( buf, PERS( victim, ch ) );  
    
    switch ( victim->position )
    {
    case POS_DEAD:     strcat( buf, " is DEAD!!" );			break;
    case POS_MORTAL:   strcat( buf, " is mortally wounded." );		break;
    case POS_INCAP:    strcat( buf, " is incapacitated." );		break;
    case POS_STUNNED:  strcat( buf, " is lying here stunned." );	break;
    case POS_SLEEPING:
        if (ch->position == POS_SITTING
        ||  ch->position == POS_RESTING )
            strcat( buf, " is sleeping nearby." );
	else
            strcat( buf, " is deep in slumber here." );
        break;
    case POS_RESTING:
        if (ch->position == POS_RESTING)
            strcat ( buf, " is sprawled out alongside you." );
        else
	if (ch->position == POS_MOUNTED)
	    strcat ( buf, " is sprawled out at the foot of your mount." );
	else
            strcat (buf, " is sprawled out here." );
        break;
    case POS_SITTING:
        if (ch->position == POS_SITTING)
            strcat( buf, " sits here with you." );
        else
        if (ch->position == POS_RESTING)
            strcat( buf, " sits nearby as you lie around." );
        else
            strcat( buf, " sits upright here." );
        break;
    case POS_STANDING:
	if ( IS_IMMORTAL(victim) )
            strcat( buf, " is here before you." );
	else
        if ( ( victim->in_room->sector_type == SECT_UNDERWATER )
        && !IS_AFFECTED(victim, AFF_AQUA_BREATH) && !IS_NPC(victim) )
            strcat( buf, " is drowning here." );
	else
	if ( victim->in_room->sector_type == SECT_UNDERWATER )
            strcat( buf, " is here in the water." );
	else
	if ( ( victim->in_room->sector_type == SECT_OCEANFLOOR )
	&& !IS_AFFECTED(victim, AFF_AQUA_BREATH) && !IS_NPC(victim) )
	    strcat( buf, " is drowning here." );
	else
	if ( victim->in_room->sector_type == SECT_OCEANFLOOR )
	    strcat( buf, " is standing here in the water." );
	else
	if ( IS_AFFECTED(victim, AFF_FLOATING)
        || IS_AFFECTED(victim, AFF_FLYING) )
          strcat( buf, " is hovering here." );
        else
          strcat( buf, " is standing here." );
        break;
    case POS_SHOVE:    strcat( buf, " is being shoved around." );	break;
    case POS_DRAG:     strcat( buf, " is being dragged around." );	break;
    case POS_MOUNTED:
	strcat( buf, " is here, upon " );
	if ( !victim->mount )
	    strcat( buf, "thin air???" );
	else
	if ( victim->mount == ch )
	    strcat( buf, "your back." );
	else
	if ( victim->in_room == victim->mount->in_room )
	{
	    strcat( buf, PERS( victim->mount, ch ) );
	    strcat( buf, "." );
	}
	else
	    strcat( buf, "someone who left??" );
	break;
    case POS_FIGHTING:
	strcat( buf, " is here, fighting " );
	if ( !victim->fighting )
	    strcat( buf, "thin air???" );
	else if ( who_fighting( victim ) == ch )
	    strcat( buf, "YOU!" );
	else if ( victim->in_room == victim->fighting->who->in_room )
	{
	    strcat( buf, PERS( victim->fighting->who, ch ) );
	    strcat( buf, "." );
	}
	else
	    strcat( buf, "someone who left??" );
	break;
    }

    strcat( buf, "\n\r" );
    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );
    show_visible_affects_to_char( victim, ch );
    return;
}



void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    int iWear;
    bool found;

    if ( can_see( victim, ch ) )
    {
    act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT    );
    act( AT_ACTION, "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
    }

    if ( victim->description[0] != '\0' )
    {
	send_to_char( victim->description, ch );
    }
    else
    {
    act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
    }

    show_condition( ch, victim );

    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
	if ( ( obj = get_eq_char( victim, iWear ) ) != NULL
	&&   can_see_obj( ch, obj ) )
	{
	    if ( !found )
	    {
		send_to_char( "\n\r", ch );
		act( AT_PLAIN, "$N is using:", ch, NULL, victim, TO_CHAR );
		found = TRUE;
	    }
	    send_to_char( where_name[iWear], ch );
	    send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
	    send_to_char( "\n\r", ch );
	}
    }

    /*
     * Crash fix here by Thoric
     */
    if ( IS_NPC(ch) || victim == ch )
      return;

    if ( number_percent( ) < ch->pcdata->learned[gsn_peek] )
    {
	send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
	show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
	learn_from_success( ch, gsn_peek );
    }
    else
      if ( ch->pcdata->learned[gsn_peek] )
        learn_from_failure( ch, gsn_peek );

    return;
}


void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    CHAR_DATA *rch;

    for ( rch = list; rch; rch = rch->next_in_room )
    {
	if ( rch == ch )
	    continue;

	if ( can_see( ch, rch ) )
	{
	    show_char_to_char_0( rch, ch );
	}
	else if ( room_is_dark( ch->in_room )
	&&        IS_AFFECTED(rch, AFF_INFRARED ) )
	{
	    set_char_color( AT_BLOOD, ch );
	    send_to_char( "The red form of a living creature is here.\n\r", ch );
	}
    }

    return;
}

void show_ships_to_char( SHIP_DATA *ship, CHAR_DATA *ch )
{
    SHIP_DATA *rship;

    send_to_char( "&C" , ch);
    for ( rship = ship; rship; rship = rship->next_in_room )
      if ( rship->owner && rship->owner[0] != '\0' && ( !IS_SET( rship->flags, SHIP_CLOAKED ) || IS_IMMORTAL( ch ) ) )
        if ( get_clan( rship->owner ) || is_online( rship->owner )
        || is_online( rship->pilot ) || is_online( rship->copilot ))
		ch_printf( ch , "%s%s\n\r", IS_SET( ship->flags, SHIP_CLOAKED ) ? "(cloaked) " : "", rship->name );  

    return;
}



bool check_blind( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT) )
	return TRUE;
	
    if ( IS_AFFECTED(ch, AFF_TRUESIGHT) )
      return TRUE;

    if ( IS_AFFECTED(ch, AFF_BLIND) )
    {
	send_to_char( "You can't see a thing!\n\r", ch );
	return FALSE;
    }

    return TRUE;
}

/*
 * Returns classical DIKU door direction based on text in arg	-Thoric
 */
int get_door( char *arg )
{
    int door;

	 if ( !str_cmp( arg, "n"  ) || !str_cmp( arg, "north"	  ) ) door = 0;
    else if ( !str_cmp( arg, "e"  ) || !str_cmp( arg, "east"	  ) ) door = 1;
    else if ( !str_cmp( arg, "s"  ) || !str_cmp( arg, "south"	  ) ) door = 2;
    else if ( !str_cmp( arg, "w"  ) || !str_cmp( arg, "west"	  ) ) door = 3;
    else if ( !str_cmp( arg, "u"  ) || !str_cmp( arg, "up"	  ) ) door = 4;
    else if ( !str_cmp( arg, "d"  ) || !str_cmp( arg, "down"	  ) ) door = 5;
    else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) ) door = 6;
    else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) ) door = 7;
    else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) ) door = 8;
    else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) ) door = 9;
    else door = -1;
    return door;
}

void do_look ( CHAR_DATA *ch, char *argument )
{
    char arg  [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *original;
    char *pdesc;
    bool doexaprog; 
    sh_int door;
    int number, cnt;

    if ( !ch->desc )
	return;

    if ( ch->position < POS_SLEEPING )
    {
	send_to_char( "You can't see anything but stars!\n\r", ch );
	return;
    }

    if ( ch->position == POS_SLEEPING )
    {
	send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
	return;
    }

    if ( !check_blind( ch ) )
	return;

    if ( !IS_NPC(ch)
    &&   !IS_SET(ch->act, PLR_HOLYLIGHT)
    &&   !IS_AFFECTED(ch, AFF_TRUESIGHT)
    &&   room_is_dark( ch->in_room ) )
    {
	set_char_color( AT_DGREY, ch );
	send_to_char( "It is pitch black ... \n\r", ch );
	show_char_to_char( ch->in_room->first_person, ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    doexaprog = str_cmp( "noprog", arg2 ) && str_cmp( "noprog", arg3 );

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
       SHIP_DATA * ship;
    
	/* 'look' or 'look auto' */
	set_char_color( AT_RMNAME, ch);
	send_to_char( ch->in_room->name, ch);
	send_to_char(" ", ch);

        if ( ! ch->desc->original )
        {
        
	if ((IS_IMMORTAL(ch)) && (IS_SET(ch->pcdata->flags, PCFLAG_ROOM)))
	 {
	  set_char_color(AT_BLUE, ch);       /* Added 10/17 by Kuran of */
	  send_to_char("{", ch);                     /* SWReality */
	  ch_printf(ch, "%d", ch->in_room->vnum);
	  send_to_char("}", ch);
	  set_char_color(AT_CYAN, ch);	 
	  send_to_char("[", ch); 
	  send_to_char(flag_string(ch->in_room->room_flags, r_flags), ch);
	  send_to_char("]", ch);
	 }
         
        }

	send_to_char( "\n\r", ch );
	set_char_color( AT_RMDESC, ch ); 	
	
	if ( arg1[0] == '\0'
	|| ( !IS_NPC(ch) && !IS_SET(ch->act, PLR_BRIEF) ) )
	    send_to_char( ch->in_room->description, ch );
 
	
	if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
	        do_exits( ch, "" );

	show_ships_to_char( ch->in_room->first_ship, ch );
	show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
	show_char_to_char( ch->in_room->first_person,  ch );
	
	if ( str_cmp( arg1, "auto" ) )
	if (   (ship = ship_from_cockpit(ch->in_room))  != NULL )
	{
	      set_char_color(  AT_WHITE, ch );
    	                 ch_printf( ch , "\n\rThrough the transparisteel windows you see:\n\r" );
    	           
	      if (ship->starsystem)
	      {
	           MISSILE_DATA *missile;
	           SHIP_DATA *target;
                   PLANET_DATA *planet;
                   ASTEROID_DATA *asteroid;
                   	           
    	           set_char_color(  AT_GREEN, ch );
    	           if ( ship->starsystem->star1 && str_cmp(ship->starsystem->star1,"") ) 
    	                 ch_printf(ch, "&YThe star, %s.\n\r" , 
    	                        ship->starsystem->star1);
    	           if ( ship->starsystem->star2 && str_cmp(ship->starsystem->star2,"")  ) 
    	                 ch_printf(ch, "&YThe star, %s.\n\r" , 
    	                        ship->starsystem->star2 );
    	           for( planet = ship->starsystem->first_planet; planet; planet = planet->next_in_system )
                   {
                         if ( !IS_SET( planet->flags, PLANET_HIDDEN ) )
    	                      ch_printf(ch, "&GThe planet, %s.\n\r" , planet->name );
                   }
		   for ( asteroid = ship->starsystem->first_roid ; asteroid ; asteroid = asteroid->next ) {
			ch_printf(ch, "&c%s\n\r", asteroid->name);
		   }
    	           for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
                   {       
                        if ( target != ship ) 
                           ch_printf(ch, "&C%s\n\r", 
                           	target->name);
                   }
    	           for ( missile = ship->starsystem->first_missile; missile; missile = missile->next_in_starsystem )
                           ch_printf(ch, "&RA missile.\n\r" ); 
                     
	      }
	      else if ( ship->location == ship->lastdoc )
	      {
	          ROOM_INDEX_DATA *to_room;
	      
	          if ( (to_room = ship->in_room ) != NULL )
	          {
	            ch_printf( ch, "\n\r" );
	            original = ch->in_room;
                    char_from_room( ch );
                    char_to_room( ch, to_room );
                    do_glance( ch, "" );	
                    char_from_room( ch );
                    char_to_room( ch, original );
                  }    
	      }
	      
	      
	}
	
	return;
    }

    if ( !str_cmp( arg1, "under" ) )
    {
	int count;

	/* 'look under' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look beneath what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}
	if ( ch->carry_weight + obj->weight > can_carry_w( ch ) )
	{
	    send_to_char( "It's too heavy for you to look under.\n\r", ch );
	    return;
	}
	count = obj->count;
	obj->count = 1;
	act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL, TO_CHAR );
	act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL, TO_ROOM );
	obj->count = count;
	if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
	   show_list_to_char( obj->first_content, ch, TRUE, TRUE );
	else
	   send_to_char( "Nothing.\n\r", ch );
	if ( doexaprog ) oprog_examine_trigger( ch, obj );
	return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
    {
	int count;

	/* 'look in' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look in what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}

	switch ( obj->item_type )
	{
	default:
	    send_to_char( "That is not a container.\n\r", ch );
	    break;

	case ITEM_DRINK_CON:
	    if ( obj->value[1] <= 0 )
	    {
		send_to_char( "It is empty.\n\r", ch );
	        if ( doexaprog ) oprog_examine_trigger( ch, obj );
		break;
	    }

	    ch_printf( ch, "It's %s full of a %s liquid.\n\r",
		obj->value[1] <     obj->value[0] / 4
		    ? "less than" :
		obj->value[1] < 3 * obj->value[0] / 4
		    ? "about"     : "more than",
		liq_table[obj->value[2]].liq_color
		);

	    if ( doexaprog ) oprog_examine_trigger( ch, obj );
	    break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	case ITEM_DROID_CORPSE:
	    if ( IS_SET(obj->value[1], CONT_CLOSED) )
	    {
		send_to_char( "It is closed.\n\r", ch );
		break;
	    }

	    count = obj->count;
	    obj->count = 1;
	    act( AT_PLAIN, "$p contains:", ch, obj, NULL, TO_CHAR );
	    obj->count = count;
	    show_list_to_char( obj->first_content, ch, TRUE, TRUE );
	    if ( doexaprog ) oprog_examine_trigger( ch, obj );
	    break;
	}
	return;
    }

    if ( (pdesc=get_extra_descr(arg1, ch->in_room->first_extradesc)) != NULL )
    {
	send_to_char( pdesc, ch );
	return;
    }

    door = get_door( arg1 );
    if ( ( pexit = find_door( ch, arg1, TRUE ) ) != NULL )
    {
      if ( pexit->keyword )
      {
	  if ( IS_SET(pexit->exit_info, EX_CLOSED)
	  &&  !IS_SET(pexit->exit_info, EX_WINDOW) )
	  {
	      if ( IS_SET(pexit->exit_info, EX_SECRET)
	      &&   door != -1 )
		send_to_char( "Nothing special there.\n\r", ch );
	      else
		act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	      return;
	  }
	  if ( IS_SET( pexit->exit_info, EX_BASHED ) )
	      act(AT_RED, "The $d has been bashed from its hinges!",ch, NULL, pexit->keyword, TO_CHAR);
      }

      if ( pexit->description && pexit->description[0] != '\0' )
  	send_to_char( pexit->description, ch );
      else
	send_to_char( "Nothing special there.\n\r", ch );

      /*
       * Ability to look into the next room			-Thoric
       */
      if ( pexit->to_room )
      {
        if ( room_is_private( ch, pexit->to_room )
        &&   !IS_IMMORTAL(ch) )
        {
	  set_char_color( AT_WHITE, ch );
	  send_to_char( "That room is private buster!\n\r", ch );
	  return;
        }
        original = ch->in_room;
           char_from_room( ch );
           char_to_room( ch, pexit->to_room );
        do_look( ch, "auto" );
        do_scan( ch, arg1 );
        char_from_room( ch );
        char_to_room( ch, original );
      }
      return;
    }
    else
    if ( door != -1 )
    {
	send_to_char( "Nothing special there.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	show_char_to_char_1( victim, ch );
	return;
    }


    /* finally fixed the annoying look 2.obj desc bug	-Thoric */
    number = number_argument( arg1, arg );
    for ( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content )
    {
	if ( can_see_obj( ch, obj ) )
	{
	    if ( (pdesc=get_extra_descr(arg, obj->first_extradesc)) != NULL )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		send_to_char( pdesc, ch );
	        if ( doexaprog ) oprog_examine_trigger( ch, obj );
		return;
	    }

	    if ( (pdesc=get_extra_descr(arg, obj->pIndexData->first_extradesc)) != NULL )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		send_to_char( pdesc, ch );
	        if ( doexaprog ) oprog_examine_trigger( ch, obj );
		return;
	    }
	    
	    if ( nifty_is_name_prefix( arg, obj->name ) )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
		if ( !pdesc )
		  pdesc = get_extra_descr( obj->name, obj->first_extradesc );
		if ( !pdesc )
		  send_to_char( "You see nothing special.\r\n", ch );
		else
		  send_to_char( pdesc, ch );
		if ( doexaprog ) oprog_examine_trigger( ch, obj );
		  return;
	    }
	}
    }

    for ( obj = ch->in_room->last_content; obj; obj = obj->prev_content )
    {
	if ( can_see_obj( ch, obj ) )
	{
	    if ( (pdesc=get_extra_descr(arg, obj->first_extradesc)) != NULL )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		send_to_char( pdesc, ch );
	        if ( doexaprog ) oprog_examine_trigger( ch, obj );
		return;
	    }

	    if ( (pdesc=get_extra_descr(arg, obj->pIndexData->first_extradesc)) != NULL )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		send_to_char( pdesc, ch );
	        if ( doexaprog ) oprog_examine_trigger( ch, obj );
		return;
	    }
	    if ( nifty_is_name_prefix( arg, obj->name ) )
	    {
		if ( (cnt += obj->count) < number )
		  continue;
		pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
		if ( !pdesc )
		  pdesc = get_extra_descr( obj->name, obj->first_extradesc );
		if ( !pdesc )
		  send_to_char( "You see nothing special.\r\n", ch );
		else
		  send_to_char( pdesc, ch );
		if ( doexaprog ) oprog_examine_trigger( ch, obj );
		  return;
	    }
	}
    }

    send_to_char( "You do not see that here.\n\r", ch );
    return;
}

void show_condition( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    int percent;

    if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
    else
        percent = -1;


    strcpy( buf, PERS(victim, ch) );

    if ( IS_NPC ( victim ) && IS_SET( victim->act , ACT_DROID ) )
    {
    
         if ( percent >= 100 ) strcat( buf, " is in perfect condition.\n\r"  );
    else if ( percent >=  90 ) strcat( buf, " is slightly scratched.\n\r" );
    else if ( percent >=  80 ) strcat( buf, " has a few scrapes.\n\r"     );
    else if ( percent >=  70 ) strcat( buf, " has some dents.\n\r"         );
    else if ( percent >=  60 ) strcat( buf, " has a couple holes in its plating.\n\r"    );
    else if ( percent >=  50 ) strcat( buf, " has a many broken pieces.\n\r" );
    else if ( percent >=  40 ) strcat( buf, " has many exposed circuits.\n\r"    );
    else if ( percent >=  30 ) strcat( buf, " is leaking oil.\n\r"   );
    else if ( percent >=  20 ) strcat( buf, " has smoke coming out of it.\n\r"       );
    else if ( percent >=  10 ) strcat( buf, " is almost completely broken.\n\r"        );
    else                       strcat( buf, " is about to EXPLODE.\n\r"              );
    
    }
    else  
    { 
    
         if ( percent >= 100 ) strcat( buf, " is in perfect health.\n\r"  );
    else if ( percent >=  90 ) strcat( buf, " is slightly scratched.\n\r" );
    else if ( percent >=  80 ) strcat( buf, " has a few bruises.\n\r"     );
    else if ( percent >=  70 ) strcat( buf, " has some cuts.\n\r"         );
    else if ( percent >=  60 ) strcat( buf, " has several wounds.\n\r"    );
    else if ( percent >=  50 ) strcat( buf, " has many nasty wounds.\n\r" );
    else if ( percent >=  40 ) strcat( buf, " is bleeding freely.\n\r"    );
    else if ( percent >=  30 ) strcat( buf, " is covered in blood.\n\r"   );
    else if ( percent >=  20 ) strcat( buf, " is leaking guts.\n\r"       );
    else if ( percent >=  10 ) strcat( buf, " is almost dead.\n\r"        );
    else                       strcat( buf, " is DYING.\n\r"              );
    
    }
    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );
    return;
}

/* A much simpler version of look, this function will show you only
the condition of a mob or pc, or if used without an argument, the
same you would see if you enter the room and have config +brief.
-- Narn, winter '96
*/
void do_glance( CHAR_DATA *ch, char *argument )
{
  char arg1 [MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int save_act;

  if ( !ch->desc )
    return;

  if ( ch->position < POS_SLEEPING )
  {
    send_to_char( "You can't see anything but stars!\n\r", ch );
    return;
  }

  if ( ch->position == POS_SLEEPING )
  {
    send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
    return;
  }

  if ( !check_blind( ch ) )
    return;

  argument = one_argument( argument, arg1 );

  if ( arg1[0] == '\0' )
  {
    save_act = ch->act;
    SET_BIT( ch->act, PLR_BRIEF );
    do_look( ch, "auto" );
    ch->act = save_act;
    return;
  }

  if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
  {
    send_to_char( "They're not here.", ch );
    return;
  }
  else
  {
    if ( can_see( victim, ch ) )
    {
      act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT    );
      act( AT_ACTION, "$n glances at $N.",  ch, NULL, victim, TO_NOTVICT );
    }

    show_condition( ch, victim );
    return;
  }

  return;
}


void do_examine( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    BOARD_DATA *board;
    sh_int dam;

    if ( !argument )
    {
	bug( "do_examine: null argument.", 0);
	return;
    }

    if ( !ch )
    {
	bug( "do_examine: null ch.", 0);
	return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Examine what?\n\r", ch );
	return;
    }

    sprintf( buf, "%s noprog", arg );
    do_look( ch, buf );

    /*
     * Support for looking at boards, checking equipment conditions,
     * and support for trigger positions by Thoric
     */
    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
	if ( (board = get_board( obj )) != NULL )
	{
	   if ( board->num_posts )
	     ch_printf( ch, "There are about %d notes posted here.  Type 'note list' to list them.\n\r", board->num_posts );
	   else
	     send_to_char( "There aren't any notes posted here.\n\r", ch );
	}

	switch ( obj->item_type )
	{
	default:
	    break;

	case ITEM_ARMOR:
	    if ( obj->value[1] == 0 )
	      obj->value[1] = obj->value[0];
	    if ( obj->value[1] == 0 )
	      obj->value[1] = 1;
	    dam = (sh_int) ((obj->value[0] * 10) / obj->value[1]);
	    strcpy( buf, "As you look more closely, you notice that it is ");
	    if (dam >= 10) strcat( buf, "in superb condition.");
       else if (dam ==  9) strcat( buf, "in very good condition.");
       else if (dam ==  8) strcat( buf, "in good shape.");
       else if (dam ==  7) strcat( buf, "showing a bit of wear.");
       else if (dam ==  6) strcat( buf, "a little run down.");
       else if (dam ==  5) strcat( buf, "in need of repair.");
       else if (dam ==  4) strcat( buf, "in great need of repair.");
       else if (dam ==  3) strcat( buf, "in dire need of repair.");
       else if (dam ==  2) strcat( buf, "very badly worn.");
       else if (dam ==  1) strcat( buf, "practically worthless.");
       else if (dam <=  0) strcat( buf, "broken.");
	    strcat( buf, "\n\r" );
	    send_to_char( buf, ch );
	    break;

	case ITEM_WEAPON:
	    dam = INIT_WEAPON_CONDITION - obj->value[0];
	    strcpy( buf, "As you look more closely, you notice that it is ");
	    if (dam ==  0) strcat( buf, "in superb condition.");
       else if (dam ==  1) strcat( buf, "in excellent condition.");
       else if (dam ==  2) strcat( buf, "in very good condition.");
       else if (dam ==  3) strcat( buf, "in good shape.");
       else if (dam ==  4) strcat( buf, "showing a bit of wear.");
       else if (dam ==  5) strcat( buf, "a little run down.");
       else if (dam ==  6) strcat( buf, "in need of repair.");
       else if (dam ==  7) strcat( buf, "in great need of repair.");
       else if (dam ==  8) strcat( buf, "in dire need of repair.");
       else if (dam ==  9) strcat( buf, "very badly worn.");
       else if (dam == 10) strcat( buf, "practically worthless.");
       else if (dam == 11) strcat( buf, "almost broken.");
       else if (dam == 12) strcat( buf, "broken.");
	    strcat( buf, "\n\r" );
	    send_to_char( buf, ch );
	    if (obj->value[3] == WEAPON_BLASTER )
	    {
		  if (obj->blaster_setting == BLASTER_FULL)
	    		ch_printf( ch, "It is set on FULL power.\n\r");
	  	  else if (obj->blaster_setting == BLASTER_HIGH)
	    		ch_printf( ch, "It is set on HIGH power.\n\r");
	  	  else if (obj->blaster_setting == BLASTER_NORMAL)
	    		ch_printf( ch, "It is set on NORMAL power.\n\r");
	  	  else if (obj->blaster_setting == BLASTER_HALF)
	    		ch_printf( ch, "It is set on HALF power.\n\r");
	  	  else if (obj->blaster_setting == BLASTER_LOW)
	    		ch_printf( ch, "It is set on LOW power.\n\r");
	  	  else if (obj->blaster_setting == BLASTER_STUN)
	    		ch_printf( ch, "It is set on STUN.\n\r");
	  	  ch_printf( ch, "It has from %d to %d shots remaining.\n\r", obj->value[4]/5 , obj->value[4] );
	    }
	    else if (     ( obj->value[3] == WEAPON_LIGHTSABER || 
		            obj->value[3] == WEAPON_VIBRO_BLADE ) )
	    {
		   ch_printf( ch, "It has %d/%d units of charge remaining.\n\r", obj->value[4] , obj->value[5] );
	    }        
	    break;

	case ITEM_FOOD:
	    if ( obj->timer > 0 && obj->value[1] > 0 )
	      dam = (obj->timer * 10) / obj->value[1];
	    else
	      dam = 10;
	    strcpy( buf, "As you examine it carefully you notice that it " );
	    if (dam >= 10) strcat( buf, "is fresh.");
       else if (dam ==  9) strcat( buf, "is nearly fresh.");
       else if (dam ==  8) strcat( buf, "is perfectly fine.");
       else if (dam ==  7) strcat( buf, "looks good.");
       else if (dam ==  6) strcat( buf, "looks ok.");
       else if (dam ==  5) strcat( buf, "is a little stale.");
       else if (dam ==  4) strcat( buf, "is a bit stale.");
       else if (dam ==  3) strcat( buf, "smells slightly off.");
       else if (dam ==  2) strcat( buf, "smells quite rank.");
       else if (dam ==  1) strcat( buf, "smells revolting.");
       else if (dam <=  0) strcat( buf, "is crawling with maggots.");
	    strcat( buf, "\n\r" );
	    send_to_char( buf, ch );
	    break;

        case ITEM_CORPSE_PC:
	case ITEM_CORPSE_NPC:
            {
		sh_int timerfrac = obj->timer;
		if ( obj->item_type == ITEM_CORPSE_PC )
		timerfrac = (int)obj->timer / 8 + 1; 

		switch (timerfrac)
		{
		    default:
			send_to_char( "This corpse has recently been slain.\n\r", ch );
			break;
		    case 4:
			send_to_char( "This corpse was slain a little while ago.\n\r", ch );
			break;
		    case 3:
			send_to_char( "A foul smell rises from the corpse, and it is covered in flies.\n\r", ch );
			break;
		    case 2:
			send_to_char( "A writhing mass of maggots and decay, you can barely go near this corpse.\n\r", ch );
			break;
		    case 1:
		    case 0:
			send_to_char( "Little more than bones, there isn't much left of this corpse.\n\r", ch );
			break;
		}
            }
	    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
	      break;
	    send_to_char( "When you look inside, you see:\n\r", ch );
	    sprintf( buf, "in %s noprog", arg );
	    do_look( ch, buf );
	    break;
	
	case ITEM_DROID_CORPSE:
            {
		sh_int timerfrac = obj->timer;
		
		switch (timerfrac)
		{
		    default:
			send_to_char( "These remains are still smoking.\n\r", ch );
			break;
		    case 4:
			send_to_char( "The parts of this droid have cooled down completely.\n\r", ch );
			break;
		    case 3:
			send_to_char( "The broken droid components are beginning to rust.\n\r", ch );
			break;
		    case 2:
			send_to_char( "The pieces are completely covered in rust.\n\r", ch );
			break;
		    case 1:
		    case 0:
			send_to_char( "All that remains of it is a pile of crumbling rust.\n\r", ch );
			break;
		}
            }
		
	case ITEM_CONTAINER:
	    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
	      break;

	case ITEM_DRINK_CON:
	    send_to_char( "When you look inside, you see:\n\r", ch );
	    sprintf( buf, "in %s noprog", arg );
	    do_look( ch, buf );
	}
	if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
	{
	    sprintf( buf, "under %s noprog", arg );
	    do_look( ch, buf );
	}
	oprog_examine_trigger( ch, obj );
	if( char_died(ch) || obj_extracted(obj) )
	  return;

    }
    return;
}


void do_exits( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;

    set_char_color( AT_EXITS, ch );
    buf[0] = '\0';
    fAuto  = !str_cmp( argument, "auto" );

    if ( !check_blind( ch ) )
	return;

    strcpy( buf, fAuto ? "Exits:" : "Obvious exits:\n\r" );

    found = FALSE;
    for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
    {
	if ( pexit->to_room
	&&  !IS_SET(pexit->exit_info, EX_HIDDEN) )
	{
	    found = TRUE;
	    if ( !fAuto )
	    {	
		if ( IS_SET(pexit->exit_info, EX_CLOSED) )
		{
		    sprintf( buf + strlen(buf), "%-5s - (closed)\n\r",
		    capitalize( dir_name[pexit->vdir] ) );
		}
		else if ( IS_SET(pexit->exit_info, EX_WINDOW) )
		{
		    sprintf( buf + strlen(buf), "%-5s - (window)\n\r",
		    capitalize( dir_name[pexit->vdir] ) );
		}
		else if ( IS_SET(pexit->exit_info, EX_xAUTO) )
		{
		   sprintf( buf + strlen(buf), "%-5s - %s\n\r",
		    capitalize( pexit->keyword ),
		    room_is_dark( pexit->to_room )
			?  "Too dark to tell"
			: pexit->to_room->name );
		}
		else
		    sprintf( buf + strlen(buf), "%-5s - %s\n\r",
		    capitalize( dir_name[pexit->vdir] ),
		    room_is_dark( pexit->to_room )
			?  "Too dark to tell"
			: pexit->to_room->name );
	    }
	    else
	    {
	        sprintf( buf + strlen(buf), " %s",
		    capitalize( dir_name[pexit->vdir] ) );
	    }
	}
    }

    if ( !found )
	strcat( buf, fAuto ? " none.\n\r" : "None.\n\r" );
    else
      if ( fAuto )
	strcat( buf, ".\n\r" );
    send_to_char( buf, ch );
    return;
}

char *	const	day_name	[] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

char *	const	month_name	[] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time( CHAR_DATA *ch, char *argument )
{
    extern char str_boot_time[];
    extern char reboot_time[];
    char *suf;
    int day;

    day     = time_info.day + 1;

	 if ( day > 4 && day <  20 ) suf = "th";
    else if ( day % 10 ==  1       ) suf = "st";
    else if ( day % 10 ==  2       ) suf = "nd";
    else if ( day % 10 ==  3       ) suf = "rd";
    else                             suf = "th";

    set_char_color( AT_YELLOW, ch );
    ch_printf( ch,
	"It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r"  
        "The mud started up at:    %s\r"
        "The system time (E.S.T.): %s\r"
        "Next Reboot is set for:   %s\r",

	(time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
	time_info.hour >= 12 ? "pm" : "am",
	day_name[day % 7],
	day, suf,
	month_name[time_info.month],
	str_boot_time,
	(char *) ctime( &current_time ),
	reboot_time
	);

    return;
}



void do_weather( CHAR_DATA *ch, char *argument )
{
    static char * const sky_look[4] =
    {
	"cloudless",
	"cloudy",
	"rainy",
	"lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE(ch) )
    {
	send_to_char( "You can't see the sky from here.\n\r", ch );
	return;
    }

    set_char_color( AT_BLUE, ch );
    ch_printf( ch, "The sky is %s and %s.\n\r",
	sky_look[weather_info.sky],
	weather_info.change >= 0
	? "a warm southerly breeze blows"
	: "a cold northern gust blows"
	);
    return;
}


/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing				-Thoric
 */
HELP_DATA *get_help( CHAR_DATA *ch, char *argument )
{
    char argall[MAX_INPUT_LENGTH];
    char argone[MAX_INPUT_LENGTH];
    char argnew[MAX_INPUT_LENGTH];
    HELP_DATA *pHelp;
    int lev;

    if ( argument[0] == '\0' )
	argument = "summary";

    if ( isdigit(argument[0]) )
    {
	lev = number_argument( argument, argnew );
	argument = argnew;
    }
    else
	lev = -2;
    /*
     * Tricky argument handling so 'help a b' doesn't match a.
     */
    argall[0] = '\0';
    while ( argument[0] != '\0' )
    {
	argument = one_argument( argument, argone );
	if ( argall[0] != '\0' )
	    strcat( argall, " " );
	strcat( argall, argone );
    }

    for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
    {
	if ( pHelp->level > get_trust( ch ) )
	    continue;
	if ( lev != -2 && pHelp->level != lev )
	    continue;

	if ( is_name( argall, pHelp->keyword ) )
	    return pHelp;
    }

    return NULL;
}


/*
 * Now this is cleaner
 */
void do_help( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;

    if ( (pHelp = get_help( ch, argument )) == NULL )
    {
	send_to_char( "No help on that word.\n\r", ch );
	return;
    }

    if ( pHelp->level >= 0 && str_cmp( argument, "imotd" ) )
    {
	send_to_pager( pHelp->keyword, ch );
	send_to_pager( "\n\r", ch );
    }

    if ( !IS_NPC(ch) && IS_SET( ch->act , PLR_SOUND ) )
        send_to_pager( "!!SOUND(help)", ch );

    /*
     * Strip leading '.' to allow initial blanks.
     */
    if ( pHelp->text[0] == '.' )
	send_to_pager_color( pHelp->text+1, ch );
    else
	send_to_pager_color( pHelp->text  , ch );
    return;
}

/*
 * Help editor							-Thoric
 */
void do_hedit( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;

    if ( !ch->desc )
    {
	send_to_char( "You have no descriptor.\n\r", ch );
	return;
    }

    switch( ch->substate )
    {
	default:
	  break;
	case SUB_HELP_EDIT:
	  if ( (pHelp = ch->dest_buf) == NULL )
	  {
		bug( "hedit: sub_help_edit: NULL ch->dest_buf", 0 );
		stop_editing( ch );
		return;
	  }
	  STRFREE( pHelp->text );
	  pHelp->text = copy_buffer( ch );
	  stop_editing( ch );
	  return;
    }
    if ( (pHelp = get_help( ch, argument )) == NULL )	/* new help */
    {
	char argnew[MAX_INPUT_LENGTH];
	int lev;

	if ( isdigit(argument[0]) )
	{
	    lev = number_argument( argument, argnew );
	    argument = argnew;
	}
	else
	    lev = get_trust(ch);
	CREATE( pHelp, HELP_DATA, 1 );
	pHelp->keyword = STRALLOC( strupper(argument) );
	pHelp->text    = STRALLOC( "" );
	pHelp->level   = lev;
	add_help( pHelp );
    }
    ch->substate = SUB_HELP_EDIT;
    ch->dest_buf = pHelp;
    start_editing( ch, pHelp->text );
}

/*
 * Stupid leading space muncher fix				-Thoric
 */
char *help_fix( char *text )
{
    char *fixed;

    if ( !text )
      return "";
    fixed = strip_cr(text);
    if ( fixed[0] == ' ' )
      fixed[0] = '.';
    return fixed;
}

void do_hset( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: hset <field> [value] [help page]\n\r",	ch );
	send_to_char( "\n\r",						ch );
	send_to_char( "Field being one of:\n\r",			ch );
	send_to_char( "  level keyword remove save addkey\n\r",		ch );
	return;
    }

    if ( !str_cmp( arg1, "save" ) )
    {
	save_help();
	return;
    }
    if ( str_cmp( arg1, "remove" ) )
	argument = one_argument( argument, arg2 );

    if ( (pHelp = get_help( ch, argument )) == NULL )
    {
	send_to_char( "Cannot find help on that subject.\n\r", ch );
	return;
    }
    if ( !str_cmp( arg1, "remove" ) )
    {
	UNLINK( pHelp, first_help, last_help, next, prev );
	STRFREE( pHelp->text );
	STRFREE( pHelp->keyword );
	DISPOSE( pHelp );
	send_to_char( "Removed.\n\r", ch );
	return;
    }
    if ( !str_cmp( arg1, "level" ) )
    {
	pHelp->level = atoi( arg2 );
	send_to_char( "Done.\n\r", ch );
	return;
    }
    if ( !str_cmp( arg1, "keyword" ) )
    {
	STRFREE( pHelp->keyword );
	pHelp->keyword = STRALLOC( strupper(arg2) );
	send_to_char( "Done.\n\r", ch );
	return;
    }

    if (str_cmp(arg1, "addkey") == FALSE)
    {
        sprintf(buf, "%s %s", pHelp->keyword, strupper(arg2));
        STRFREE(pHelp->keyword);
        pHelp->keyword = STRALLOC(buf);
        ch_printf(ch, "Done.\r\n");
        return;
    }

    do_hset( ch, "" );
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 */
void do_hlist( CHAR_DATA *ch, char *argument )
{
    int min, max, minlimit, maxlimit, cnt;
    char arg[MAX_INPUT_LENGTH];
    HELP_DATA *help;

    maxlimit = get_trust(ch);
    minlimit = IS_IMMORTAL(ch) ? -1 : 0;
    argument = one_argument( argument, arg );
    if ( arg[0] != '\0' )
    {
	min = URANGE( minlimit, atoi(arg), maxlimit );
	if ( argument[0] != '\0' )
	    max = URANGE( min, atoi(argument), maxlimit );
	else
	    max = maxlimit;
    }
    else
    {
	min = minlimit;
	max = maxlimit;
    }
    set_pager_color( AT_GREEN, ch );
    pager_printf( ch, "Help Topics in level range %d to %d:\n\r\n\r", min, max );
    for ( cnt = 0, help = first_help; help; help = help->next )
	if ( help->level >= min && help->level <= max )
	{
	    pager_printf( ch, "  %3d %s\n\r", help->level, help->keyword );
	    ++cnt;
	}
    if ( cnt )
	pager_printf( ch, "\n\r%d pages found.\n\r", cnt );
    else
	send_to_char( "None found.\n\r", ch );
}


/* 
 * New do_who
 
  with WHO REQUEST, clan and homepage support.  -Thoric
 *
 * Latest version of do_who eliminates redundant code by using linked lists.
 * Shows imms separately, indicates guest and retired immortals.
 * Narn, Oct/96
 */
void do_who( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char clan_name[MAX_INPUT_LENGTH];
    char invis_str[MAX_INPUT_LENGTH];
    char char_name[MAX_INPUT_LENGTH];
    char extra_title[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    bool fImmortalOnly;
    bool fShowHomepage;
    bool fClanMatch; /* SB who clan */
    CLAN_DATA *pClan;
    FILE *whoout;

    WHO_DATA *cur_who = NULL;
    WHO_DATA *next_who = NULL;
    WHO_DATA *first_mortal = NULL;
    WHO_DATA *first_imm = NULL;

    if(IS_NPC(ch)) {
	send_to_char("Players only.\n\r", ch);
	return;
    }

    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = 200;
    fImmortalOnly  = FALSE;
    fShowHomepage  = FALSE;
    fClanMatch	   = FALSE; /* SB who clan  */

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
	char arg[MAX_STRING_LENGTH];

	argument = one_argument( argument, arg );
	if ( arg[0] == '\0' )
	    break;
	if (!IS_IMMORTAL(ch)) {
		send_to_char("Immortals only.\n\r",ch);
		break;
	}

	if ( is_number( arg ) )
	{
	    switch ( ++nNumber )
	    {
	    case 1: iLevelLower = atoi( arg ); break;
	    case 2: iLevelUpper = atoi( arg ); break;
	    default:
		send_to_char( "Only two level numbers allowed.\n\r", ch );
		return;
	    }
	}
	else
	{
	    if ( strlen(arg) < 3 )
	    {
		send_to_char( "Be a little more specific please.\n\r", ch );
		return;
	    }

	    /*
	     * Look for classes to turn on.
	     */
            
	    if ( !str_cmp( arg, "imm" ) || !str_cmp( arg, "gods" ) )
		fImmortalOnly = TRUE;
	    else
	    if ( !str_cmp( arg, "www" ) )
		fShowHomepage = TRUE;
            else		 /* SB who clan (order), guild */
             if  ( ( pClan = get_clan (arg) ) )
	   	fClanMatch = TRUE;
	    else
	    {
		    send_to_char( "That's not an organization.\n\r", ch );
		    return;
	    }
	}
    }

    /*
     * Now find matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    if ( ch )
	set_pager_color( AT_GREEN, ch );
    else
    {
	if ( fShowHomepage )
	  whoout = fopen( WEBWHO_FILE, "w" );
	else
	  whoout = fopen( WHO_FILE, "w" );
    }

/* start from last to first to get it in the proper order */
    for ( d = last_descriptor; d; d = d->prev )
    {
	CHAR_DATA *wch;

	if ( (d->connected != CON_PLAYING && d->connected != CON_EDITING)
	|| ( !can_see( ch, d->character ) && IS_IMMORTAL( d->character ) )
	|| d->original)
	    continue;
	wch   = d->original ? d->original : d->character;
	if ( wch->top_level < iLevelLower
	||   wch->top_level > iLevelUpper 
	|| ( fImmortalOnly  && !IS_IMMORTAL(wch) )
	|| ( fClanMatch && ( pClan != wch->pcdata->clan ))  /* SB */ )
	    continue;

	nMatch++;

	if ( fShowHomepage
	&&   wch->pcdata->homepage
	&&   wch->pcdata->homepage[0] != '\0' )
	  sprintf( char_name, "<A HREF=\"%s\">%s</A>",
		show_tilde( wch->pcdata->homepage ), wch->name );
	else
	  strcpy( char_name, "" );

        if ( !nifty_is_name(wch->name, wch->pcdata->title) && ch->top_level > wch->top_level )
          sprintf( extra_title , " [%s]" , wch->name );
        else
          strcpy(extra_title, "");
        
	if ( wch->pcdata->clan )
	{
          CLAN_DATA *pclan = wch->pcdata->clan;
	    
	  strcpy( clan_name, " (" );

	    if ( is_leader(wch) )
              strcat( clan_name, "Leader, " );

	  strcat( clan_name, pclan->name );
	  strcat( clan_name, ")" );
	}
	else
	  clan_name[0] = '\0';


	if((wch->pcdata->clan != ch->pcdata->clan) && !IS_IMMORTAL(ch)) {
		clan_name[0] = '\0';
	}

	if ( IS_SET(wch->act, PLR_WIZINVIS) )
	  sprintf( invis_str, "(%d) ", wch->pcdata->wizinvis );
	else
	  invis_str[0] = '\0';

	if ( wch->desc->connected == CON_EDITING )
						strcat( invis_str, "[Writing] " );

	sprintf( buf, "%s%s%s%s %s%s%s\n\r",
	    invis_str,
	    IS_SET(wch->act, PLR_AFK) ? "[AFK] " : "",
	    char_name,
	    wch->pcdata->title,
            extra_title,
	    clan_name,
//	    IS_IMMORTAL(wch) ? "&R [&WCoder&R/&WImmortal&R]&W" : "&W" );
	    "&W" );

          /*
           * This is where the old code would display the found player to the ch.
           * What we do instead is put the found data into a linked list
           */

          /* First make the structure. */
          CREATE( cur_who, WHO_DATA, 1 );
          cur_who->text = str_dup( buf );


          if ( IS_OFFICIAL( wch ) )
            cur_who->type = WT_IMM;
          else
            cur_who->type = WT_MORTAL;
          
          /* Then put it into the appropriate list. */
          switch ( cur_who->type )
          {
            case WT_MORTAL:
              cur_who->next = first_mortal;
              first_mortal = cur_who;
              break;
            case WT_IMM:
              cur_who->next = first_imm;
              first_imm = cur_who;
              break;
            
          }

    }


    /* Ok, now we have three separate linked lists and what remains is to
     * display the information and clean up.
     */

     /* Deadly list removed for swr ... now only 2 lists */


    if ( first_mortal )
    {
      if ( !ch )
        fprintf( whoout,"\n\r--------------------------------[ Galactic Citizens ]-------------------------\n\r\n\r" );
      else
       send_to_pager( "\n\r&R--------------------------------[ &YGalactic Citizens&R ]-------------------------&W\n\r\n\r", ch );
    }

    for ( cur_who = first_mortal; cur_who; cur_who = next_who )
    {
      if ( !ch )
        fprintf( whoout, cur_who->text );
      else
        send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      DISPOSE( cur_who->text );
      DISPOSE( cur_who ); 
    } 

    if ( first_imm )
    {
      if ( !ch )
        fprintf( whoout, "\n\r--------------------------------[ Elected Officials ]-------------------------\n\r\n\r" );
      else
       send_to_pager(  "\n\r&R--------------------------------[ &YElected Officials&R ]-------------------------&W\n\r\n\r", ch );
    }

    for ( cur_who = first_imm; cur_who; cur_who = next_who )
    {
      if ( !ch )
        fprintf( whoout, cur_who->text );
      else
        send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      DISPOSE( cur_who->text );
      DISPOSE( cur_who ); 
    } 

    if ( !ch )
    {
	fprintf( whoout, "%d player%s.\n\r", nMatch, nMatch == 1 ? "" : "s" );
	fclose( whoout );
	return;
    }

    set_char_color( AT_YELLOW, ch );
    ch_printf( ch, "\n\r%d player%s.\n\r", nMatch, nMatch == 1 ? "" : "s" );
    ch_printf( ch, "\n\rAlltime Player High: %d players.\n\r", sysdata.alltimemax );
    return;
}


void do_compare( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Compare what to what?\n\r", ch );
	return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	for ( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content )
	{
	    if ( obj2->wear_loc != WEAR_NONE
	    &&   can_see_obj( ch, obj2 )
	    &&   obj1->item_type == obj2->item_type
	    && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
		break;
	}

	if ( !obj2 )
	{
	    send_to_char( "You aren't wearing anything comparable.\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( ( obj2 = get_obj_carry( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}
    }

    msg		= NULL;
    value1	= 0;
    value2	= 0;

    if ( obj1 == obj2 )
    {
	msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type )
    {
	msg = "You can't compare $p and $P.";
    }
    else
    {
	switch ( obj1->item_type )
	{
	default:
	    msg = "You can't compare $p and $P.";
	    break;

	case ITEM_ARMOR:
	    value1 = obj1->value[0];
	    value2 = obj2->value[0];
	    break;

	case ITEM_WEAPON:
	    value1 = obj1->value[1] + obj1->value[2];
	    value2 = obj2->value[1] + obj2->value[2];
	    break;
	}
    }

    if ( !msg )
    {
	     if ( value1 == value2 ) msg = "$p and $P look about the same.";
	else if ( value1  > value2 ) msg = "$p looks better than $P.";
	else                         msg = "$p looks worse than $P.";
    }

    act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
    return;
}



void do_where( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    if (!IS_IMMORTAL(ch))
    {
       send_to_char( "If only life were really that simple...\n\r" , ch);
       return;
    }    

    one_argument( argument, arg );

    set_pager_color( AT_PERSON, ch );
    if ( arg[0] == '\0' )
    {
        send_to_pager( "Players logged in:\n\r", ch );
	found = FALSE;
	for ( d = first_descriptor; d; d = d->next )
	    if ( (d->connected == CON_PLAYING || d->connected == CON_EDITING )
	    && ( victim = d->character ) != NULL
	    &&   !IS_NPC(victim)
	    &&   victim->in_room
	    &&   can_see( ch, victim ) )
	    {
		found = TRUE;
		pager_printf( ch, "%-28s %s\n\r",
		    victim->name, victim->in_room->name );
	    }
	if ( !found )
	    send_to_char( "None\n\r", ch );
    }
    else
    {
	found = FALSE;
	for ( victim = first_char; victim; victim = victim->next )
	    if ( victim->in_room
	    &&   !IS_AFFECTED(victim, AFF_HIDE)
	    &&   !IS_AFFECTED(victim, AFF_SNEAK)
	    &&   can_see( ch, victim )
	    &&   is_name( arg, victim->name ) )
	    {
		found = TRUE;
		pager_printf( ch, "%-28s %s\n\r",
		    PERS(victim, ch), victim->in_room->name );
		break;
	    }
	if ( !found )
	    act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }

    return;
}




void do_consider( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Consider killing whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They're not here.\n\r", ch );
	return;
    }

    diff = ( victim->hitroll - ch->hitroll ) * 5 ;
    diff += (int) (victim->max_hit - ch->max_hit);

	 if ( diff <= -200) msg = "$N looks like a feather!";
    else if ( diff <= -150) msg = "Hey! Where'd $N go?";
    else if ( diff <= -100) msg = "Easy as picking off womp rats at beggers canyon!";
    else if ( diff <=  -50) msg = "$N is a wimp.";
    else if ( diff <=    0) msg = "$N looks weaker than you.";
    else if ( diff <=   50) msg = "$N looks about as strong as you.";
    else if ( diff <=  100) msg = "It would take a bit of luck...";
    else if ( diff <=  150) msg = "It would take a lot of luck, and a really big blaster!";
    else if ( diff <=  200) msg = "Why don't you just attack a star destoyer with a vibroblade?";
    else                    msg = "$N is built like an AT-AT!";
    act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

    return;
}



void do_forget( CHAR_DATA *ch, char *argument )
{
    int sn;

    if ( IS_NPC(ch) || !ch->pcdata )
        return;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Forget what?\n\r", ch );
	return;
    }
        
    sn = skill_lookup( argument );
   
    if ( sn == -1 )
    {
        send_to_char( "No such skill...\n\r", ch );
	return;
    }

    if ( ch->pcdata->learned[sn] <= 0 ) 
    {
        send_to_char( "You don't know that skill...\n\r", ch );
	return;
    }

    if ( ch->pcdata->learned[sn] > 50 ) 
    {
        send_to_char( "You cannot forget something you know so well...\n\r", ch );
	return;
    }
	
    send_to_char( "Forget what?... *chuckle*\n\r", ch );
    ch->pcdata->learned[sn] = 0;
    ch->pcdata->num_skills--;
           
}

void do_teach( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int sn;
    char arg[MAX_INPUT_LENGTH];
    
    if ( IS_NPC(ch) )
	return;

    argument = one_argument(argument, arg);

    if ( argument[0] == '\0' )
    {
        send_to_char( "Teach who, what?\n\r", ch );
	return;
    }
    else
    {
	CHAR_DATA *victim;
	int adept;

	if ( !IS_AWAKE(ch) )
	{
	    send_to_char( "In your dreams, or what?\n\r", ch );
	    return;
	}
        
        if ( ( victim = get_char_room( ch, arg ) ) == NULL )
        {
	    send_to_char( "They don't seem to be here...\n\r", ch );
	    return;
	}
                
        if (IS_NPC(victim))
        {
	    send_to_char( "You can't teach that to them!\n\r", ch );
	    return;
	}
        
	sn = skill_lookup( argument );
   
        if ( sn == -1 )
        {
            act( AT_TELL, "You have no idea what that is.",
		victim, NULL, ch, TO_VICT );
	    return;
        }
	
	adept = 20;

	if ( victim->pcdata->learned[sn] >= adept )
	{
	    act( AT_TELL, "$n must practice that on their own.", victim, NULL, ch, TO_VICT );
	    return;
	}
	
	if ( victim->pcdata->num_skills >= get_curr_int(victim) )
	{
	    act( AT_TELL, "$n is not intelligent enough to learn any more skills.", victim, NULL, ch, TO_VICT );
	    return;
	}

	if ( (victim->pcdata->num_skills - victim->pcdata->adept_skills) >= 5 )
	{
	    act( AT_TELL, "$N needs to perfect some of the skills $E has before learning new ones.", ch, NULL, victim, TO_CHAR );
	    return;
	}

	if ( skill_table[sn]->type == SKILL_SPELL && ( get_curr_frc(victim) <= 0 || victim->max_mana  <= 0 ) )
	{
	    act( AT_TELL, "$n is not force capable.", victim, NULL, ch, TO_VICT );
	    return;
	}

	if( IS_SET( skill_table[sn]->flags, SF_NOTEACH ) )
	{
		act( AT_TELL, "You cannot teach that skill.", victim, NULL, ch, TO_VICT );
		return;
	}

	if ( ch->pcdata->learned[sn] < 100 )
	{
	    act( AT_TELL, "You must perfect that yourself before teaching others.", victim, NULL, ch, TO_VICT );
	    return;
	}
	else
	{
	    if ( victim->pcdata->learned[sn] <= 0 )
                  victim->pcdata->num_skills++;
	    victim->pcdata->learned[sn] += int_app[get_curr_int(ch)].learn;
	    sprintf( buf, "You teach %s $T.", victim->name );
	    act( AT_ACTION, buf,
		    ch, NULL, skill_table[sn]->name, TO_CHAR );
	    sprintf( buf, "%s teaches you $T.", ch->name );
	    act( AT_ACTION, buf,
		    victim, NULL, skill_table[sn]->name, TO_CHAR );
	}
    }
    return;
}


void do_wimpy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	wimpy = (int) ch->max_hit / 5;
    else
	wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
	send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
	return;
    }

    if ( wimpy > ch->max_hit )
    {
	send_to_char( "Such cowardice ill becomes you.\n\r", ch );
	return;
    }

    ch->wimpy	= wimpy;
    ch_printf( ch, "Wimpy set to %d percent.\n\r", wimpy );
    return;
}



void do_password( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
	return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: password <old> <new>.\n\r", ch );
	return;
    }

    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
	WAIT_STATE( ch, 40 );
	send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	return;
    }

    if ( strlen(arg2) < 5 )
    {
	send_to_char(
	    "New password must be at least five characters long.\n\r", ch );
	return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
	if ( *p == '~' )
	{
	    send_to_char(
		"New password not acceptable, try again.\n\r", ch );
	    return;
	}
    }

    DISPOSE( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    if ( IS_SET(sysdata.save_flags, SV_PASSCHG) )
	save_char_obj( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_socials( CHAR_DATA *ch, char *argument )
{
    int iHash;
    int col = 0;
    SOCIALTYPE *social;

    set_pager_color( AT_PLAIN, ch );
    for ( iHash = 0; iHash < 27; iHash++ )
	for ( social = social_index[iHash]; social; social = social->next )
	{
	    pager_printf( ch, "%-12s", social->name );
	    if ( ++col % 6 == 0 )
		send_to_pager( "\n\r", ch );
	}

    if ( col % 6 != 0 )
	send_to_pager( "\n\r", ch );
    return;
}


void do_commands( CHAR_DATA *ch, char *argument )
{
    int col;
    bool found;
    int hash;
    CMDTYPE *command;

    col = 0;
    set_pager_color( AT_PLAIN, ch );
    if ( argument[0] == '\0' )
    {
	for ( hash = 0; hash < 126; hash++ )
	    for ( command = command_hash[hash]; command; command = command->next )
		if ( command->level <= get_trust( ch )
		&&  (command->name[0] != 'm'
		&&   command->name[1] != 'p') )
		{
		    pager_printf( ch, "%-16s", command->name );
		    if ( ++col % 5 == 0 )
			send_to_pager( "\n\r", ch );
		}
	if ( col % 6 != 0 )
	    send_to_pager( "\n\r", ch );
    }
    else
    {
	found = FALSE;
	for ( hash = 0; hash < 126; hash++ )
	    for ( command = command_hash[hash]; command; command = command->next )
		if ( command->level <= get_trust( ch )
		&&  !str_prefix(argument, command->name)
		&&  (command->name[0] != 'm'
		&&   command->name[1] != 'p') )
		{
		    pager_printf( ch, "%-16s", command->name );
		    found = TRUE;
		    if ( ++col % 5 == 0 )
			send_to_pager( "\n\r", ch );
		}

	if ( col % 6 != 0 )
	    send_to_pager( "\n\r", ch );
	if ( !found )
	    ch_printf( ch, "No command found under %s.\n\r", argument);
    }
    return;
}


void do_channels( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_SILENCE) )
	{
	    send_to_char( "You are silenced.\n\r", ch );
	    return;
	}

	send_to_char( "Channels:", ch );

        if ( get_trust( ch ) > 2 && !NOT_AUTHED( ch ) )
        {
	 send_to_char( !IS_SET(ch->deaf, CHANNEL_AUCTION)
	    ? " +AUCTION"
	    : " -auction",
	    ch );
        }

        send_to_char( !IS_SET(ch->deaf, CHANNEL_OOC)
	    ? " +OOC"
	    : " -ooc",
	    ch );

        send_to_char( !IS_SET(ch->deaf, CHANNEL_PNET)
	    ? " +pnet"
	    : " -pnet",
	    ch );


        send_to_char( !IS_SET(ch->deaf, CHANNEL_GNET)
	    ? " +gnet"
	    : " -gnet",
	    ch );


	if ( !IS_NPC( ch ) && ch->pcdata->clan )
	{
 	    send_to_char( !IS_SET(ch->deaf, CHANNEL_CLAN)
	        ? " +CLAN"
	        : " -clan",
	        ch );
	}

	send_to_char( !IS_SET( ch->deaf, CHANNEL_TELLS )
	    ? " +TELLS"
	    : " -tells",
	    ch );

	if ( IS_IMMORTAL(ch) )
	{
	    send_to_char( !IS_SET(ch->deaf, CHANNEL_IMMTALK)
		? " +IMMTALK"
		: " -immtalk",
		ch );
	}


	send_to_char( !IS_SET(ch->deaf, CHANNEL_YELL)
	    ? " +YELL"
	    : " -yell",
	    ch );

	if ( IS_IMMORTAL(ch) )
	{
	    send_to_char( !IS_SET(ch->deaf, CHANNEL_MONITOR)
		? " +MONITOR"
		: " -monitor",
		ch );
	}

	send_to_char( !IS_SET(ch->deaf, CHANNEL_NEWBIE)
	      	? " +NEWBIE"
		: " -newbie",
		ch );

	if ( IS_IMMORTAL(ch) )
	{
	    send_to_char( !IS_SET(ch->deaf, CHANNEL_LOG)
		? " +LOG"
		: " -log",
		ch );

	    send_to_char( !IS_SET(ch->deaf, CHANNEL_BUILD)
		? " +BUILD"
		: " -build",
		ch );

            send_to_char( !IS_SET(ch->deaf, CHANNEL_COMM)
	        ? " +COMM"
	        : " -comm",
	        ch );
	}
	send_to_char( ".\n\r", ch );
    }
    else
    {
	bool fClear;
	bool ClearAll;
	int bit;

        bit=0;
        ClearAll = FALSE;

	     if ( arg[0] == '+' ) fClear = TRUE;
	else if ( arg[0] == '-' ) fClear = FALSE;
	else
	{
	    send_to_char( "Channels -channel or +channel?\n\r", ch );
	    return;
	}

	//who's bright idea was it to make it so anyone can add or remove any channel?
	//phel will fix!
	     if ( !str_cmp( arg+1, "auction"  ) ) bit = CHANNEL_AUCTION;
	else if ( !str_cmp( arg+1, "ooc"      ) ) bit = CHANNEL_OOC;
	else if ( !str_cmp( arg+1, "clan"     ) ) bit = CHANNEL_CLAN;
	else if ( !str_cmp( arg+1, "tells"    ) ) bit = CHANNEL_TELLS;
	else if ( !str_cmp( arg+1, "newbie"   ) ) bit = CHANNEL_NEWBIE;
	else if ( !str_cmp( arg+1, "yell"     ) ) bit = CHANNEL_YELL;
	else if ( !str_cmp( arg+1, "comm"     ) ) bit = CHANNEL_COMM;
	else if ( !str_cmp( arg+1, "pnet"     ) ) bit = CHANNEL_PNET;
	else if ( !str_cmp( arg+1, "gnet"     ) ) bit = CHANNEL_GNET;
	else if (IS_IMMORTAL(ch)) {
		if ( !str_cmp( arg+1, "immtalk"  ) ) bit = CHANNEL_IMMTALK;
		else if ( !str_cmp( arg+1, "all"      ) ) ClearAll = TRUE;
		else if ( !str_cmp( arg+1, "log"      ) ) bit = CHANNEL_LOG;
		else if ( !str_cmp( arg+1, "build"    ) ) bit = CHANNEL_BUILD;
		else if ( !str_cmp( arg+1, "monitor"  ) ) bit = CHANNEL_MONITOR;
	}
	else
	{
	    send_to_char( "Set or clear which channel?\n\r", ch );
	    return;
	}

	if (( fClear ) && ( ClearAll ))
	{
            REMOVE_BIT (ch->deaf, CHANNEL_AUCTION);
            REMOVE_BIT (ch->deaf, CHANNEL_YELL);

	    if ( IS_IMMORTAL(ch) )
	      REMOVE_BIT (ch->deaf, CHANNEL_COMM);

        } else if ((!fClear) && (ClearAll))
        {
            SET_BIT (ch->deaf, CHANNEL_AUCTION);
            SET_BIT (ch->deaf, CHANNEL_YELL);
          
	    if ( IS_IMMORTAL(ch) )
	      SET_BIT (ch->deaf, CHANNEL_COMM);

         } else if (fClear)
         {
	    REMOVE_BIT (ch->deaf, bit);
         } else
         {
	    SET_BIT    (ch->deaf, bit);
         }

	  send_to_char( "Ok.\n\r", ch );
    }

    return;
}

void do_wizlist( CHAR_DATA *ch, char *argument )
{
    ch_printf( ch , "\n\r" );
    ch_printf( ch , "&WGalactic Empires\n\r" );
    ch_printf( ch , "\n\r" );
    ch_printf( ch , "  SWR 2.0-alpha2 by\n\r" );
    ch_printf( ch , "  Sean Cooper (aka Durga)\n\r" );
    ch_printf( ch , "  specs@golden.net\n\r" );
    ch_printf( ch , "\n\r" );
    ch_printf( ch , "Elected Officials:\n\r" );
    ch_printf( ch , "\n\r" );
    ch_printf( ch , "  %s\n\r" , sysdata.officials );    
    ch_printf( ch , "\n\r" );    
}

/*
 * Contributed by Grodyn.
 */
void do_config( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    if ( IS_NPC(ch) )
	return;

    one_argument( argument, arg );

    set_char_color( AT_WHITE, ch );
    if ( arg[0] == '\0' )
    {
	send_to_char( "[ Keyword  ] Option\n\r", ch );

	send_to_char(  IS_SET(ch->act, PLR_FLEE)
	    ? "[+FLEE     ] You flee if you get attacked.\n\r"
	    : "[-flee     ] You fight back if you get attacked.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->pcdata->flags, PCFLAG_NORECALL)
	    ? "[+NORECALL ] You fight to the death, link-dead or not.\n\r"
	    : "[-norecall ] You try to recall if fighting link-dead.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_AUTOEXIT)
	    ? "[+AUTOEXIT ] You automatically see exits.\n\r"
	    : "[-autoexit ] You don't automatically see exits.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_AUTOLOOT)
	    ? "[+AUTOLOOT ] You automatically loot corpses.\n\r"
	    : "[-autoloot ] You don't automatically loot corpses.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_AUTOSAC)
	    ? "[+AUTOSAC  ] You automatically sacrifice corpses.\n\r"
	    : "[-autosac  ] You don't automatically sacrifice corpses.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_AUTOGOLD)
	    ? "[+AUTOCRED ] You automatically split credits from kills in groups.\n\r"
	    : "[-autocred ] You don't automatically split credits from kills in groups.\n\r"
	    , ch );

        send_to_char(  IS_SET(ch->pcdata->flags, PCFLAG_GAG)       
            ? "[+GAG      ] You see only necessary battle text.\n\r"
            : "[-gag      ] You see full battle text.\n\r"
            , ch );

        send_to_char(  IS_SET(ch->pcdata->flags, PCFLAG_PAGERON)
            ? "[+PAGER    ] Long output is page-paused.\n\r"
            : "[-pager    ] Long output scrolls to the end.\n\r"
            , ch );
	
	send_to_char(  IS_SET(ch->act, PLR_BLANK)
	    ? "[+BLANK    ] You have a blank line before your prompt.\n\r"
	    : "[-blank    ] You have no blank line before your prompt.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_BRIEF)
	    ? "[+BRIEF    ] You see brief descriptions.\n\r"
	    : "[-brief    ] You see long descriptions.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_COMBINE)
	    ? "[+COMBINE  ] You see object lists in combined format.\n\r"
	    : "[-combine  ] You see object lists in single format.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->pcdata->flags, PCFLAG_NOINTRO)
	    ? "[+NOINTRO  ] You don't see the ascii intro screen on login.\n\r"
	    : "[-nointro  ] You see the ascii intro screen on login.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_PROMPT)
	    ? "[+PROMPT   ] You have a prompt.\n\r"
	    : "[-prompt   ] You don't have a prompt.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_TELNET_GA)
	    ? "[+TELNETGA ] You receive a telnet GA sequence.\n\r"
	    : "[-telnetga ] You don't receive a telnet GA sequence.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_ANSI)
	    ? "[+ANSI     ] You receive ANSI color sequences.\n\r"
	    : "[-ansi     ] You don't receive receive ANSI colors.\n\r"
	    , ch );

	send_to_char(  IS_SET(ch->act, PLR_SOUND)
	    ? "[+SOUND     ] You have MSP support.\n\r"
	    : "[-sound     ] You don't have MSP support.\n\r"
	    , ch );


	send_to_char(  IS_SET(ch->act, PLR_SHOVEDRAG)
	      ? "[+SHOVEDRAG] You allow yourself to be shoved and dragged around.\n\r"
	      : "[-shovedrag] You'd rather not be shoved or dragged around.\n\r"
	      , ch );

	send_to_char(  IS_SET( ch->pcdata->flags, PCFLAG_NOSUMMON )
	      ? "[+NOSUMMON ] You do not allow other players to summon you.\n\r"
	      : "[-nosummon ] You allow other players to summon you.\n\r"
	      , ch );

	if ( IS_IMMORTAL( ch ) )
	  send_to_char(  IS_SET(ch->act, PLR_ROOMVNUM)
	      ? "[+VNUM     ] You can see the VNUM of a room.\n\r"
	      : "[-vnum     ] You do not see the VNUM of a room.\n\r"
	      , ch );

	if ( IS_IMMORTAL( ch ) )
	  send_to_char(  IS_SET(ch->act, PLR_AUTOMAP)    /* maps */
	      ? "[+MAP      ] You can see the MAP of a room.\n\r"
	      : "[-map      ] You do not see the MAP of a room.\n\r"
	      , ch );

	if ( IS_IMMORTAL( ch) )             /* Added 10/16 by Kuran of SWR */
	  send_to_char( IS_SET(ch->pcdata->flags, PCFLAG_ROOM)
	      ? "[+ROOMFLAGS] You will see room flags.\n\r"
	      : "[-roomflags] You will not see room flags.\n\r"
	      , ch );
	
	send_to_char(  IS_SET(ch->act, PLR_SILENCE)
	    ? "[+SILENCE  ] You are silenced.\n\r"
	    : ""
	    , ch );

	send_to_char( !IS_SET(ch->act, PLR_NO_EMOTE)
	    ? ""
	    : "[-emote    ] You can't emote.\n\r"
	    , ch );

	send_to_char( !IS_SET(ch->act, PLR_NO_TELL)
	    ? ""
	    : "[-tell     ] You can't use 'tell'.\n\r"
	    , ch );

	send_to_char( !IS_SET(ch->act, PLR_LITTERBUG)
	    ? ""
	    : "[-litter  ] A convicted litterbug. You cannot drop anything.\n\r"
	    , ch );
    }
    else
    {
	bool fSet;
	int bit = 0;

	     if ( arg[0] == '+' ) fSet = TRUE;
	else if ( arg[0] == '-' ) fSet = FALSE;
	else
	{
	    send_to_char( "Config -option or +option?\n\r", ch );
	    return;
	}

	     if ( !str_prefix( arg+1, "autoexit" ) ) bit = PLR_AUTOEXIT;
	else if ( !str_prefix( arg+1, "autoloot" ) ) bit = PLR_AUTOLOOT;
	else if ( !str_prefix( arg+1, "autosac"  ) ) bit = PLR_AUTOSAC;
	else if ( !str_prefix( arg+1, "autocred" ) ) bit = PLR_AUTOGOLD;
	else if ( !str_prefix( arg+1, "blank"    ) ) bit = PLR_BLANK;
	else if ( !str_prefix( arg+1, "brief"    ) ) bit = PLR_BRIEF;
	else if ( !str_prefix( arg+1, "combine"  ) ) bit = PLR_COMBINE;
	else if ( !str_prefix( arg+1, "prompt"   ) ) bit = PLR_PROMPT;
	else if ( !str_prefix( arg+1, "telnetga" ) ) bit = PLR_TELNET_GA;
	else if ( !str_prefix( arg+1, "ansi"     ) ) bit = PLR_ANSI;
	else if ( !str_prefix( arg+1, "sound"      ) ) bit = PLR_SOUND;
	else if ( !str_prefix( arg+1, "flee"     ) ) bit = PLR_FLEE;
	else if ( !str_prefix( arg+1, "nice"     ) ) bit = PLR_NICE;
	else if ( !str_prefix( arg+1, "shovedrag") ) bit = PLR_SHOVEDRAG;
	else if ( IS_IMMORTAL( ch )
	     &&   !str_prefix( arg+1, "vnum"     ) ) bit = PLR_ROOMVNUM;
	else if ( IS_IMMORTAL( ch )
	     &&   !str_prefix( arg+1, "map"      ) ) bit = PLR_AUTOMAP;     /* maps */

	if (bit)
        {
          
	  if ( fSet )
	    SET_BIT    (ch->act, bit);
	  else
	    REMOVE_BIT (ch->act, bit);
	  send_to_char( "Ok.\n\r", ch );
          return;
        }
        else
        {
	       if ( !str_prefix( arg+1, "norecall" ) ) bit = PCFLAG_NORECALL;
	  else if ( !str_prefix( arg+1, "nointro"  ) ) bit = PCFLAG_NOINTRO;
	  else if ( !str_prefix( arg+1, "nosummon" ) ) bit = PCFLAG_NOSUMMON;
          else if ( !str_prefix( arg+1, "gag"      ) ) bit = PCFLAG_GAG; 
          else if ( !str_prefix( arg+1, "pager"    ) ) bit = PCFLAG_PAGERON;
          else if ( !str_prefix( arg+1, "roomflags") 
                    && (IS_IMMORTAL(ch))) bit = PCFLAG_ROOM;
          else
	  {
	    send_to_char( "Config which option?\n\r", ch );
	    return;
    	  }

          if ( fSet )
	    SET_BIT    (ch->pcdata->flags, bit);
	  else
	    REMOVE_BIT (ch->pcdata->flags, bit);

	  send_to_char( "Ok.\n\r", ch );
          return;
        }
    }

    return;
}


void do_credits( CHAR_DATA *ch, char *argument )
{
  do_help( ch, "credits" );
}


extern int top_area;

void do_afk( CHAR_DATA *ch, char *argument )
{
     if ( IS_NPC(ch) )
     return;
     
     if IS_SET(ch->act, PLR_AFK)
     {
    	REMOVE_BIT(ch->act, PLR_AFK);
	send_to_char( "You are no longer afk.\n\r", ch );
	act(AT_GREY,"$n is no longer afk.", ch, NULL, NULL, TO_ROOM);
     }
     else
     {
	SET_BIT(ch->act, PLR_AFK);
	send_to_char( "You are now afk.\n\r", ch );
	act(AT_GREY,"$n is now afk.", ch, NULL, NULL, TO_ROOM);
	return;
     }
         
}

void do_slist( CHAR_DATA *ch, char *argument )
{
}

void do_whois( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  buf[0] = '\0';

  if(IS_NPC(ch))
    return;

  if(argument[0] == '\0')
  {
    send_to_char("You must input the name of a player online.\n\r", ch);
    return;
  }

  strcat(buf, "0.");
  strcat(buf, argument);
  if( ( ( victim = get_char_world(ch, buf) ) == NULL ))
  {
    send_to_char("No such player online.\n\r", ch);
    return;
  }

  if(IS_NPC(victim))
  {
    send_to_char("That's not a player!\n\r", ch);
    return;
  }

  ch_printf(ch, "%s is a %s character",
	victim->name, 
	victim->sex == SEX_MALE ? "male" : 
	victim->sex == SEX_FEMALE ? "female" : "neutral" );
  if (IS_IMMORTAL(ch))
    ch_printf(ch, " in room %d.\n\r",
	victim->in_room->vnum);
  else {
   ch_printf(ch, ".\n\r", ch);
  }

  if ( victim->pcdata->clan && (IS_IMMORTAL(ch) || victim->pcdata->clan == ch->pcdata->clan) )
  {
	ch_printf(ch, "%s belongs to %s", victim->name, victim->pcdata->clan->name);
  }
  send_to_char( ".\n\r", ch );

  if(victim->pcdata->homepage && victim->pcdata->homepage[0] != '\0')
    ch_printf(ch, "%s's homepage can be found at %s.\n\r", 
	victim->name,
	victim->pcdata->homepage);

  if(victim->pcdata->bio && victim->pcdata->bio[0] != '\0')
    ch_printf(ch, "%s's personal bio:\n\r%s",
	victim->name,
	victim->pcdata->bio);

  if(IS_IMMORTAL(ch))
  {  
    send_to_char("----------------------------------------------------\n\r", ch);

    send_to_char("Info for immortals:\n\r", ch);

    if ( victim->pcdata->authed_by && victim->pcdata->authed_by[0] != '\0' )
	ch_printf(ch, "%s was authorized by %s.\n\r",
		victim->name, victim->pcdata->authed_by);

    if ( victim->pcdata->pkills || victim->pcdata->pdeaths )
	ch_printf(ch, "%s has killed %d players, and been killed by a player %d times.\n\r",
		victim->name, victim->pcdata->pkills, victim->pcdata->pdeaths );

    ch_printf(ch, "%s is %shelled at the moment.\n\r",
	victim->name, 
	(victim->pcdata->release_date == 0) ? "not " : "");

    if(victim->pcdata->release_date != 0)
      ch_printf(ch, "%s was helled by %s, and will be released on %24.24s.\n\r",
	victim->sex == SEX_MALE ? "He" :
	victim->sex == SEX_FEMALE ? "She" : "It",
        victim->pcdata->helled_by,
	ctime(&victim->pcdata->release_date));

    if(get_trust(victim) < get_trust(ch))
    {
      sprintf(buf2, "list %s", buf);
    }

    if(IS_SET(victim->act, PLR_SILENCE) || IS_SET(victim->act, PLR_NO_EMOTE) 
    || IS_SET(victim->act, PLR_NO_TELL) )
    {
      sprintf(buf2, "This player has the following flags set:");
      if(IS_SET(victim->act, PLR_SILENCE)) 
        strcat(buf2, " silence");
      if(IS_SET(victim->act, PLR_NO_EMOTE)) 
        strcat(buf2, " noemote");
      if(IS_SET(victim->act, PLR_NO_TELL) )
        strcat(buf2, " notell");
      strcat(buf2, ".\n\r");
      send_to_char(buf2, ch);
    }
    if ( victim->desc && victim->desc->host[0]!='\0' )   /* added by Gorog */
    {
      sprintf (buf2, "%s's IP info: %s ", victim->name, victim->desc->hostip);
      if ( IS_IMMORTAL(ch) )
      {
        strcat (buf2, victim->desc->user);
        strcat (buf2, "@");
        strcat (buf2, victim->desc->host);
      }
      strcat (buf2, "\n\r");
      send_to_char(buf2, ch);
    }
    if ( IS_IMMORTAL(ch) && get_trust(ch) >= get_trust( victim ) && victim->pcdata )
    {
        sprintf (buf2, "Email: %s\n\r" , victim->pcdata->email );
        send_to_char(buf2, ch);
    }
  }
}

void do_pager( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  
  if ( IS_NPC(ch) )
    return;
  argument = one_argument(argument, arg);
  if ( !*arg )
  {
    if ( IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
      do_config(ch, "-pager");
    else
      do_config(ch, "+pager");
    return;
  }
  if ( !is_number(arg) )
  {
    send_to_char( "Set page pausing to how many lines?\n\r", ch );
    return;
  }
  ch->pcdata->pagerlen = atoi(arg);
  if ( ch->pcdata->pagerlen < 5 )
    ch->pcdata->pagerlen = 5;
  ch_printf( ch, "Page pausing set to %d lines.\n\r", ch->pcdata->pagerlen );
  return;
}

bool is_online( char * argument )
{
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
       return FALSE;
    
    for ( d = last_descriptor; d; d = d->prev )
    {
	CHAR_DATA *wch;

	if ( (d->connected != CON_PLAYING && d->connected != CON_EDITING)
	|| d->original)
	    continue;
	wch   = d->original ? d->original : d->character;
    
        if ( !str_cmp( argument , wch->name ) )
           return TRUE;
    }
    
    return FALSE;
    
}

void do_train(CHAR_DATA *ch, char *argument)
{
	bool checktoken = FALSE;
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	argument = one_argument( argument, arg );

	if( !IS_SET( ch->in_room->room_flags, ROOM_TRAIN ) )
	{
		send_to_char( "&BThis is not a attribute training room.&w\n\r", ch );
		return;
	}

	for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
	{
		if( obj->pIndexData->vnum == OBJ_VNUM_TOKEN )
		{
			checktoken = TRUE;
			break;
		}
	}

	if( checktoken == FALSE )
	{
		send_to_char( "&RYou need a training token to train.&w\n\r", ch );
		return;
	}

	if( !strcmp( arg, "str" ) )
	{
		if( ch->perm_str >= 25 )
		{
			send_to_char( "&RYou cannot be any stronger...&w\n\r", ch );
			return;
		}

		ch->perm_str++;
		ch->perm_str = UMIN( ch->perm_str, 25 );
		send_to_char( "The trainer makes you lift a lot of heavy weights.\n\r", ch );
		send_to_char( "&GYou feel a little bit stronger...&w\n\r", ch );
	}
	else if( !strcmp( arg, "con" ) )
	{
		if( ch->perm_con >= 25 )
		{
			send_to_char( "&RYou cannot be any tougher...&w\n\r", ch );
			return;
		}

		ch->perm_con++;
		ch->perm_con = UMIN( ch->perm_con, 25 );
		send_to_char( "The trainer makes you run a few miles.\n\r", ch );
		send_to_char( "&GYou feel a little bit healthier and a little bit tougher...&w\n\r", ch );
	}
	else if( !strcmp( arg, "dex" ) )
	{
		if( ch->perm_dex >= 25 )
		{
			send_to_char( "&RYou cannot be any more coordinated...&w\n\r", ch );
			return;
		}

		ch->perm_dex++;
		ch->perm_dex = UMIN( ch->perm_dex, 25 );
		send_to_char( "The trainer makes you build small models.\n\r", ch );
		send_to_char( "&GYou feel more coordinated than before.&w\n\r", ch );
	}
	else if( !strcmp( arg, "int" ) )
	{
		if( ch->perm_int >= 25 )
		{
			send_to_char( "&RYou cannot be any smarter...&w\n\r", ch );
			return;
		}

		ch->perm_int++;
		ch->perm_int = UMIN( ch->perm_int, 25 );
		send_to_char( "The trainer makes you read a lot of books.\n\r", ch );
		send_to_char( "&GYou feel a little bit smarter...&w\n\r", ch );
	}
	else if( !strcmp( arg, "wis" ) )
	{
		if( ch->perm_wis >= 25 )
		{
			send_to_char( "&RYou cannot be any wiser...&w\n\r", ch );
			return;
		}

		ch->perm_wis++;
		ch->perm_wis = UMIN( ch->perm_wis, 25 );
		send_to_char( "The trainer engages you in several philosphical conversations.\n\r", ch );
		send_to_char( "&GYou feel a little bit wiser...&w\n\r", ch );
	}
	else if( !strcmp( arg, "cha" ) )
	{
		if( ch->perm_cha >= 25 )
		{
			send_to_char( "&RYou cannot be any more charming...&w\n\r", ch );
			return;
		}

		ch->perm_cha++;
		ch->perm_cha = UMIN( ch->perm_cha, 25 );
		send_to_char( "The trainer teaches you how to hit on the ladies.\n\r", ch );
		send_to_char( "&GYou feel a little bit more charming...&w\n\r", ch );
	}
	else if( !strcmp( arg, "hp" ) )
	{
		if( ch->max_hit >= 300 )
		{
			send_to_char( "&RYou cannot have any more health.&w\n\r", ch );
			return;
		}

		ch->max_hit = ch->max_hit + 10;
		ch->max_hit = UMIN( ch->max_hit, 300 );
		send_to_char( "The trainer makes you drink a special brew.\n\r", ch );
		send_to_char( "&GYou feel a little bit more healthy...&w\n\r", ch );
	}
	else if( !strcmp( arg, "move" ) )
	{
		if( ch->max_move >= 2000 )
		{
			send_to_char( "&RYou cannot have any more stamina.&w\n\r", ch );
			return;
		}

		ch->max_move = ch->max_move + 50;
		ch->max_move = UMIN( ch->max_move, 2000 );
		send_to_char( "The trainer makes you jump rope for a while.\n\r", ch );
		send_to_char( "&GYou gain a little bit more stamina...&w\n\r", ch );
	}
	else
	{
		send_to_char( "&RUsage: train <attribute>\n\r", ch );
		send_to_char( "Attributes: str con dex int wis cha hp move&w\n\r", ch );
		return;
	}

	send_to_char( "The trainer bows before you and takes your token.\n\r", ch );
	separate_obj( obj );
	obj_from_char( obj );
	extract_obj( obj );
}

/**
 * A command that will list all the current users and their idle status.
 *
 * @author  Joel McBeth
 * @version 1.1     02/22/05
 *
 * @param   ch      The character calling the command.
 * @param   args    The arguments supplied with the command.
 */
void do_idle(CHAR_DATA* ch, char* args)
{
    CHAR_DATA* ref;
    char afk_buf[MAX_STRING_LENGTH], status_buf[MAX_STRING_LENGTH];
    int idle_time;
    int idle, afk, active;

    if (ch == NULL)
    {
        bug("do_idle: NULL ch.", 0);
        return;
    }

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

    idle = 0;
    afk = 0;
    active = 0;

    ch_printf(ch, "&W%-67s%-7s%s\r\n", "Player", "AFK", "Status&G");
    for (ref = first_char; ref != NULL; ref = ref->next)
    {
        if ((IS_NPC(ref) == FALSE) && (ref->desc != NULL))            
        {
            idle_time = (ref->desc->idle / 4);

            /* To get the string for idle people and to count them. */
            if ((IS_IMMORTAL(ref) == TRUE) && (IS_IMMORTAL(ch) == FALSE))
            {
                strcpy(status_buf, "??????");
            }
            else if (idle_time > TIME_TILL_IDLE)
            {
                strcpy(status_buf, "Idle");
                idle++;
            }
            else
            {
                strcpy(status_buf, "Active");
                active++;
            }

            /* To get string for AFK people and to count them. */
            if (IS_SET(ref->act, PLR_AFK))
            {
                strcpy(afk_buf, "Yes");
                afk++;
            }
            else
            {
               strcpy(afk_buf, "No");
            }
            
            /* Check if the immortal is wizinvis. */
            if ((IS_SET(ref->act, PLR_WIZINVIS) == FALSE) ||
                ((IS_IMMORTAL(ch) == TRUE) && (ch->pcdata->wizinvis >= ref->pcdata->wizinvis)))
            {
                ch_printf(ch, "%-67s%-7s%s\r\n", ref->name, afk_buf,  status_buf);
            }
        }
    }

    ch_printf(ch, "\r\n");
    ch_printf(ch, "&WActive Players: &G%d\r\n", active);
    ch_printf(ch, "&WIdle Players:   &G%d\r\n", idle);
    ch_printf(ch, "&WAFK Players:    &G%d\r\n", afk);
}
