#include <math.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

SHIP_DATA * first_ship;
SHIP_DATA * last_ship;

MISSILE_DATA * first_missile;
MISSILE_DATA * last_missile;

SPACE_DATA * first_starsystem;
SPACE_DATA * last_starsystem;


/* local routines */
void	fread_ship	args( ( SHIP_DATA *ship, FILE *fp ) );
bool	lload_ship_file	args( ( char *shipfile ) );
void	write_ship_list	args( ( void ) );
void    fread_starsystem      args( ( SPACE_DATA *starsystem, FILE *fp ) );
bool    load_starsystem  args( ( char *starsystemfile ) );
void    write_starsystem_list args( ( void ) );
void    resetship args( ( SHIP_DATA *ship ) );
void    landship args( ( SHIP_DATA *ship, char *arg ) );
void    launchship args( ( SHIP_DATA *ship ) );
bool    land_bus args( ( SHIP_DATA *ship, int destination ) );
void    launch_bus args( ( SHIP_DATA *ship ) );
void    echo_to_room_dnr args( ( int ecolor , ROOM_INDEX_DATA *room ,  char *argument ) );
ch_ret drive_ship( CHAR_DATA *ch, SHIP_DATA *ship, EXIT_DATA  *exit , int fall );
bool    autofly(SHIP_DATA *ship);
bool is_facing( SHIP_DATA *ship , SHIP_DATA *target );
void sound_to_ship( SHIP_DATA *ship , char *argument );

/* from comm.c */
bool    write_to_descriptor     args( ( int desc, char *txt, int length ) );

/* from act_info.c */
bool 	is_online		args( ( char * argument ) );

// The names of the flags for ship_data
char * const ship_flags[] = { "cloaked", "reserved1", "reserved2", "reserved3", "reserved4",
							  "reserved5", "reserved6", "reserved7", "reserved8", "reserved9",
							  "reserved10", "reserved11", "reserved12", "reserved13", "reserved14", 
							  "reserved15", "reserved16", "reserved17", "reserved18", "reserved19", 
							  "reserved20", "reserved21", "reserved22", "reserved23", "reserved24",
							  "reserved25", "reserved26", "reserved27", "reserved28", "reserved29",
							  "reserved30", "reserved31" };

// The names of the flags for space_data
char * const space_flags[] = { "noasteroids", "reserved2", "reserved3", "reserved4", "reserved5",
                               "reserved6", "reserved7", "reserved8", "reserved9", "reserved10",
                               "reserved11", "reserved12", "reserved13", "reserved14", "reserved15",
                               "reserved16", "reserved17", "reserved18", "reserved19", "reserved20",
                               "reserved21", "reserved22", "reserved23","reserved24","reserved25",
                               "reserved26", "reserved27", "reserved28","reserved29","reserved30",
                               "reserved31", "reserved32" };

// Returns the flag given an array of flag names and the flag name
int get_flag( char * flag, char * const flagsList[] )
{
   int x;

   for ( x = 0; x < 32; x++ )
      if ( !str_cmp( flag, flagsList[x] ) )
         return x;

   return -1;
}

// Returns a string list of an array of flag names
char * get_flags( char * const flagsList[] )
{
   char * buf = malloc( MAX_STRING_LENGTH );
   int i = 0;

   strcpy( buf, flagsList[0] );

   for ( i = 1; i < 32; i++ )
      sprintf( buf, "%s, %s", buf, flagsList[i] );

   return buf;
}

// Returns a list of the names of the flags set
char * get_setflags( int flags, char * const flagsList[] )
{
   char * buf = malloc( MAX_STRING_LENGTH );
   bool isFirst = TRUE;
   int i = 0;

   strcpy( buf, "None" );

   for ( i = 0; i < 32; i++ )
   {
      if ( IS_SET( flags, 1 << i ) )
      {
         if ( isFirst )
         {
            strcpy( buf, flagsList[i] );
            isFirst = FALSE;
         }
         else         
            sprintf( buf, "%s, %s", buf, flagsList[i] );         
      }
   }

   return buf;
}

int find_range( int x1, int y1, int z1, int x2, int y2, int z2 )
{
        /**
         * This uses long int because the sum of the squares do become larger than a 32 bit int.
         * pow() probably doesn't return a long so if there are really large coordinates there is
         * a problem there as well.
         */
	long int x, y, z, sum;

	x = pow( x1 - x2, 2);
	y = pow( y1 - y2, 2);
	z = pow( z1 - z2, 2);

        sum = x + y + z;

	return sqrt( sum );
}

void echo_to_room_dnr ( int ecolor , ROOM_INDEX_DATA *room ,  char *argument ) 
{
    CHAR_DATA *vic;
    
    if ( room == NULL )
    	return;
    	
    for ( vic = room->first_person; vic; vic = vic->next_in_room )
    {
	set_char_color( ecolor, vic );
	send_to_char( argument, vic );
    }
}


void move_ships( )
{
   SHIP_DATA *ship;
   SHIP_DATA *nextship;
   MISSILE_DATA *missile;
   MISSILE_DATA *m_next;
   SHIP_DATA *target;
   float dx, dy, dz, change;
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *ch;
   bool ch_found = FALSE;
   PLANET_DATA * planet;
      
   for ( missile = first_missile; missile; missile = m_next )
   {
      m_next = missile->next;
           
      ship = missile->fired_from;
      target = missile->target;
              
      if ( target && target->starsystem && target->starsystem == missile->starsystem )
      {
         if ( missile->mx < target->vx ) 
            missile->mx += UMIN( missile->speed/5 , target->vx - missile->mx );
         else if ( missile->mx > target->vx ) 
            missile->mx -= UMIN( missile->speed/5 , missile->mx - target->vx );  

         if ( missile->my < target->vy ) 
	        missile->my += UMIN( missile->speed/5 , target->vy - missile->my );
         else if ( missile->my > target->vy ) 
            missile->my -= UMIN( missile->speed/5 , missile->my - target->vy );

         if ( missile->mz < target->vz ) 
               missile->mz += UMIN( missile->speed/5 , target->vz - missile->mz );
         else if ( missile->mz > target->vz ) 
            missile->mz -= UMIN( missile->speed/5 , missile->mz - target->vz );  
      }
      else
      {
         extract_missile( missile );
         continue;
      }


      if ( find_range( missile->mx, missile->my, missile->mz, target->vx, target->vy, target->vz ) <= 20 )							
      {
         if ( target->chaff_released <= 0)
         { 
            echo_to_room( AT_YELLOW , ship->gunseat , "Your missile hits its target dead on!" );

            echo_to_cockpit( AT_BLOOD, target, "The ship is hit by a missile.");

            echo_to_ship( AT_RED , target , "A loud explosion shakes thee ship violently!" );

            sprintf( buf, "You see a small explosion as %s is hit by a missile" , target->name );
            echo_to_system( AT_ORANGE , target , buf , ship );

            for ( ch = first_char; ch; ch = ch->next )
            {
               if ( !IS_NPC( ch ) && nifty_is_name( missile->fired_by, ch->name ) )
               {   
                  ch_found = TRUE; 
                  damage_ship_ch( target , 30 , 50 , ch );
               }
            }

            if ( !ch_found )
            {
               damage_ship( target , 30 , 50 );   
               extract_missile( missile );
            }
         }
         else
         {
            echo_to_room( AT_YELLOW , ship->gunseat , "Your missile explodes harmlessly in a cloud of chaff!" );
            echo_to_cockpit( AT_YELLOW, target, "A missile explodes in your chaff.");
            extract_missile( missile );
         }

         continue;
      }
      else
      {
         missile->age++;

         if (missile->age >= 50)
         {
            extract_missile( missile );
            continue;
         }
      }
   }
   
   for ( ship = first_ship; ship; ship = nextship )
   {
     nextship = ship->next;
     
     if ( !ship->starsystem )
        continue;
          
     if ( ship->currspeed > 0 )
        {
          
          change = sqrt( ship->hx*ship->hx + ship->hy*ship->hy + ship->hz*ship->hz );

           if (change > 0)
           {
             dx = ship->hx/change;     
             dy = ship->hy/change;     
             dz = ship->hz/change;
             ship->vx += (dx * ship->currspeed/5);
             ship->vy += (dy * ship->currspeed/5);
             ship->vz += (dz * ship->currspeed/5);
	     if(ship->towing != NULL) {
		ship->towing->x = ship->vx;
		ship->towing->y = ship->vy;
		ship->towing->z = ship->vz;
	     }
           }
           
        } 

        if ( autofly(ship) )
           continue;

		if ( ship->starsystem->star1
			&& strcmp( ship->starsystem->star1, "")
			&& find_range( ship->vx, ship->vy, ship->vz, ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z ) < 20 )
		{      
            echo_to_cockpit( AT_BLOOD+AT_BLINK, ship, "You fly directly into the sun.");
            sprintf( buf , "%s flys directly into %s!", ship->name, ship->starsystem->star1); 
            echo_to_system( AT_ORANGE , ship , buf , NULL );
            destroy_ship(ship, NULL);
            continue;
        }

		if ( ship->starsystem->star2
			&& strcmp( ship->starsystem->star2, "")
			&& find_range( ship->vx, ship->vy, ship->vz, ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z ) < 20 )
		{
            echo_to_cockpit( AT_BLOOD+AT_BLINK, ship, "You fly directly into the sun.");
            sprintf( buf , "%s flys directly into %s!", ship->name, ship->starsystem->star2); 
            echo_to_system( AT_ORANGE , ship , buf , NULL );
            destroy_ship(ship , NULL);
            continue;
        }

        if ( ship->currspeed > 0 )
        { 
			for ( planet = ship->starsystem->first_planet ; planet ; planet = planet->next_in_system )      
				if ( find_range( ship->vx, ship->vy, ship->vz, planet->x, planet->y, planet->z ) < 10 )
				{
					sprintf( buf , "You begin orbitting %s.", planet->name); 
					echo_to_cockpit( AT_YELLOW, ship, buf);
					sprintf( buf , "%s begins orbiting %s.", ship->name, planet->name ); 
					echo_to_system( AT_ORANGE , ship , buf , NULL );
					ship->currspeed = 0;
					continue;
				}            
        }
   }

   for ( ship = first_ship; ship; ship = nextship )
   {    
       nextship = ship->next;
       if (ship->collision) 
       {
           echo_to_cockpit( AT_WHITE+AT_BLINK , ship,  "You have collided with another ship!" );
           echo_to_ship( AT_RED , ship , "A loud explosion shakes the ship violently!" );   
           damage_ship( ship , ship->collision , ship->collision );
           ship->collision = 0;
       }
   }
}   

void recharge_ships( )
{
	SHIP_DATA *ship;
	SHIP_DATA *nextship;
	char buf[MAX_STRING_LENGTH];
	TURRET_DATA *turret;
   
	for ( ship = first_ship; ship; ship = nextship )
	{                
		nextship = ship->next;

		for( turret = ship->first_turret ; turret ; turret = turret->next )
		{
			if (turret->laserstate > 0)
			{
				ship->energy -= turret->laserstate;
				turret->laserstate = 0;
			}
		}
   
		if (ship->laserstate > 0)
		{
			ship->energy -= ship->laserstate;
			ship->laserstate = 0;
		}

		if (ship->missilestate == MISSILE_RELOAD_2)
		{
			ship->missilestate = MISSILE_READY;
			if ( ship->missiles > 0 )
				echo_to_room( AT_YELLOW, ship->gunseat, "Missile launcher reloaded.");
		}

		if (ship->missilestate == MISSILE_RELOAD )		
			ship->missilestate = MISSILE_RELOAD_2;		

		if (ship->missilestate == MISSILE_FIRED )
			ship->missilestate = MISSILE_RELOAD;
        
		if ( autofly(ship) )
		{
			if ( ship->starsystem )
			{
				if (ship->target && ship->laserstate != LASER_DAMAGED )
				{
					int chance = 100;
					SHIP_DATA * target = ship->target;
					int shots;
		            
					for ( shots=0 ; shots <= ship->lasers ; shots++ ) 
					{   
						if ( !ship->target )
							break;
		        
						if ( ship->shipstate != SHIP_HYPERSPACE
							&& ship->energy > 25
							&& ship->target->starsystem == ship->starsystem
							&& find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) <= 1000
							&& ship->laserstate < ship->lasers )
						{                                                      
							if ( ship->class == SPACE_STATION || is_facing ( ship , target ) )
							{
								chance += target->model*10;
								chance -= target->manuever/10;
								chance -= target->currspeed/20;
								chance -= ( abs(target->vx - ship->vx)/70 );
								chance -= ( abs(target->vy - ship->vy)/70 );
								chance -= ( abs(target->vz - ship->vz)/70 );
								chance = URANGE( 10 , chance , 90 );

								if ( number_percent( ) > chance )
								{	  
           							sprintf( buf , "%s fires at you but misses." , ship->name);  
             						echo_to_cockpit( AT_ORANGE , target , buf );
      								sprintf( buf, "Laserfire from %s barely misses %s." , ship->name , target->name );
									echo_to_system( AT_ORANGE , target , buf , NULL );	
								} 
								else
								{
             						sprintf( buf, "Laserfire from %s hits %s." , ship->name, target->name );
             						echo_to_system( AT_ORANGE , target , buf , NULL );
									sprintf( buf , "You are hit by lasers from %s!" , ship->name);  
									echo_to_cockpit( AT_BLOOD , target , buf );           
									echo_to_ship( AT_RED , target , "A small explosion vibrates through the ship." );           
									if ( autofly( target ) )
										target->target = ship;
									damage_ship( target , 5 , 10 );
								}

								ship->laserstate++;
							}
						}
					}
				}
			}
		}       
   }
}

/**
 * This goes through and handles updating all the ships.
 */
void update_space()
{
    SHIP_DATA *ship;
    SHIP_DATA *nextship;
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];
    int too_close, target_too_close;
    int recharge, drain, energy_buffer;
       
    for (ship = first_ship; ship; ship = nextship)
    {    
        nextship = ship->next;      

        /**
         * If you try to perform an action that will take you below less than 5% energy
         * it will fail to happen to protect you from running out of energy.
         */
        energy_buffer = ship->maxenergy * .05;

   
        /* Check if the ship is in space. */
        if (ship->starsystem != NULL)
        {
            /* The normal amount that is recharged. */
            ship->energy += (5 + ship->model * 2);

            /* If the ship is disabled then it will loose energy. */
            if ((ship->shipstate == SHIP_DISABLED) && (ship->class != SPACE_STATION))
                ship->energy = UMIN(0, ship->energy - 100);

            /* Slowly drain the shields. */      
            ship->shield = UMAX(0, ship->shield - 1 - ship->model/5);      

            if ((ship->autorecharge == TRUE) && (ship->shield < ship->maxshield))
            {
                /* Get the max amount of shields to recharge. */
                recharge = UMIN(ship->maxshield - ship->shield, 10 + (ship->model * 2));

                /* If there is enough energy then recharge the shields and drain the energy. */
                if ((ship->energy - recharge) > energy_buffer)
                {
                    ship->shield += recharge;
                    ship->energy -= recharge;
                }
            }

            /* If there was chaff released then it expires. */
            if (ship->chaff_released > 0)
                ship->chaff_released--;

            /* Calculate the ammount of energy that is drained from being cloaked. */
            drain = 10 + (ship->maxhull / 600);      

            if (IS_SET(ship->flags, SHIP_CLOAKED))
            {         
                ship->energy = UMAX( 0, ship->energy - drain );

                /**
                 * If there is not enough energy to maintain cloak for atleast two more updates then
                 * display a warning.
                 */
                if ((ship->energy - (drain * 2)) < energy_buffer)
                {
                    echo_to_cockpit(AT_RED, ship, "Insufficent energy to maintain cloak.");
                }

                if (ship->energy < energy_buffer)
                {
                    REMOVE_BIT(ship->flags, SHIP_CLOAKED);

                    sprintf(buf, "The ship fades into view.");
                    echo_to_cockpit(AT_YELLOW, ship, buf);

                    sprintf(buf, "%s fades into view.", ship->name);
                    echo_to_system(AT_YELLOW, ship, buf, NULL);            
                }
            }  

            /* If the ship is moving display the speed and coords. */
            if (ship->currspeed > 0)
            {
                sprintf(buf, "%d", ship->currspeed);
                echo_to_room_dnr(AT_BLUE , ship->pilotseat, "Speed: ");
                echo_to_room_dnr(AT_LBLUE , ship->pilotseat, buf);
                sprintf(buf, "%.0f %.0f %.0f", ship->vx, ship->vy, ship->vz);
                echo_to_room_dnr( AT_BLUE , ship->pilotseat, "  Coords: ");
                echo_to_room( AT_LBLUE , ship->pilotseat, buf );
            }

            too_close = ship->currspeed + 50;

            /* Check if any ships are too close. */
            for (target = ship->starsystem->first_ship; target; target = target->next_in_starsystem)
            { 
                /* So it doesn't try to see if it is to close to itself. */
                if (target == ship)
                    continue;

                target_too_close = too_close + target->currspeed;

                /* TODO: Ship collisions. */

                /* If the other ship isn't cloaked display a proximity warning. */
                if ((target != ship) && !IS_SET(target->flags, SHIP_CLOAKED)
                    && (find_range(ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz) < target_too_close))
                {
                    sprintf(buf, "Proximity alert: %s  %.0f %.0f %.0f", target->name, target->vx, target->vy, target->vz);
                    echo_to_room(AT_RED , ship->pilotseat,  buf);    
                }
            }    

            too_close = ship->currspeed + 100;

            /* See if the ship is too close to the first star. */
            if ((ship->starsystem->star1 != NULL)
                && strcmp(ship->starsystem->star1, "")
                && (find_range(ship->vx, ship->vy, ship->vz, ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z) < too_close))
            {
                sprintf(buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star1, ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z);
                echo_to_room(AT_RED , ship->pilotseat,  buf);
            }

            /* See if the ship is to close to the second star. */
            if ((ship->starsystem->star2 != NULL)
                && strcmp(ship->starsystem->star2, "")
                && (find_range(ship->vx, ship->vy, ship->vz, ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z ) < too_close))
            {
                sprintf(buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star2, ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z);
                echo_to_room( AT_RED , ship->pilotseat,  buf );            
            }

            if (ship->target)
            {
                sprintf(buf, "%s   %.0f %.0f %.0f", ship->target->name, ship->target->vx, ship->target->vy, ship->target->vz);
                echo_to_room_dnr(AT_BLUE , ship->pilotseat, "Target: ");
                echo_to_room(AT_LBLUE , ship->pilotseat, buf);

                /* If your target isn't in the system then reset your target. */
                if (ship->starsystem != ship->target->starsystem)
                    ship->target = NULL;
            }

            /* Display a warning if the ship is low on fuel. */
   	    if (ship->energy < energy_buffer)   	
   	        echo_to_cockpit(AT_RED, ship, "Warning: Ship fuel low.");

            /* Make sure your energy doesn't drop below 0. */
            ship->energy = URANGE(0 , ship->energy, ship->maxenergy);
        }

        if (ship->shipstate == SHIP_HYPERSPACE)
        {
            ship->hyperdistance -= ship->hyperspeed * 2;

            /* Your hyperdrive destination has been reached and you should come out of hyperspace now. */
            if (ship->hyperdistance <= 0)
            {
                ship_to_starsystem (ship, ship->currjump);
                            
                if (ship->starsystem == NULL)
                {
                    echo_to_cockpit(AT_RED, ship, "Ship lost in Hyperspace. Make new calculations.");
                }
                else
                {
                    echo_to_room(AT_YELLOW, ship->pilotseat, "Hyperjump complete.");
                    echo_to_ship(AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace.");

                    sprintf(buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz);
                    echo_to_system(AT_YELLOW, ship, buf , NULL);

                    ship->shipstate = SHIP_READY;

                    STRFREE(ship->home);
                    ship->home = STRALLOC(ship->starsystem->name);
                }
            }
            else
            {            
                sprintf(buf, "%d", ship->hyperdistance);
                echo_to_room_dnr(AT_YELLOW, ship->pilotseat, "Remaining jump distance: ");            
                echo_to_room(AT_WHITE, ship->pilotseat,  buf);
            }
        }

        /**
        * following was originaly to fix ships that lost their pilot 
        * in the middle of a manuever and are stuck in a busy state 
        * but now used for timed manouevers such as turning
        */    
        if (ship->shipstate == SHIP_BUSY_3)
        {
            echo_to_room( AT_YELLOW, ship->pilotseat, "Manuever complete.");  
            ship->shipstate = SHIP_READY;
        }

        if (ship->shipstate == SHIP_BUSY_2)
            ship->shipstate = SHIP_BUSY_3;
        if (ship->shipstate == SHIP_BUSY)
            ship->shipstate = SHIP_BUSY_2;

        if (ship->shipstate == SHIP_LAND_2)
            landship(ship, ship->dest);
        if (ship->shipstate == SHIP_LAND)
            ship->shipstate = SHIP_LAND_2;

        if (ship->shipstate == SHIP_LAUNCH_2)
            launchship(ship);
        if (ship->shipstate == SHIP_LAUNCH)
            ship->shipstate = SHIP_LAUNCH_2;               
   } 

   for ( ship = first_ship; ship; ship = ship->next )
   {
       
       if (ship->autotrack && ship->target && ship->class < SPACE_STATION )    
       {
           target = ship->target;
           too_close = ship->currspeed + 10; 
           target_too_close = too_close+target->currspeed;
		   if ( target != ship
			   && ship->shipstate == SHIP_READY
			   && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) < target_too_close )
           {
              ship->hx = 0-(ship->target->vx - ship->vx);
              ship->hy = 0-(ship->target->vy - ship->vy);
              ship->hz = 0-(ship->target->vz - ship->vz);
              ship->energy -= ship->currspeed/10;
              echo_to_room( AT_RED , ship->pilotseat, "Autotrack: Evading to avoid collision!\n\r" );  
    	      if ( ship->manuever > 100 )
        	ship->shipstate = SHIP_BUSY_3;
              else if ( ship->manuever > 50  )
        	ship->shipstate = SHIP_BUSY_2;
    	      else
        	ship->shipstate = SHIP_BUSY;     
           }
           else if  ( !is_facing(ship, ship->target) )
           {
              ship->hx = ship->target->vx - ship->vx;
              ship->hy = ship->target->vy - ship->vy;
              ship->hz = ship->target->vz - ship->vz;
              ship->energy -= ship->currspeed/10;
              echo_to_room( AT_BLUE , ship->pilotseat, "Autotracking target ... setting new course.\n\r" );      
    	      if ( ship->manuever > 100  )
        	ship->shipstate = SHIP_BUSY_3;
              else if ( ship->manuever > 50  )
        	ship->shipstate = SHIP_BUSY_2;
    	      else
        	ship->shipstate = SHIP_BUSY;     
           }
       }

       if ( autofly(ship) )
       {
          if ( ship->starsystem )
          {
             if (ship->target)
             {                 
                 int chance = 100;
             
                 target = ship->target;
                 ship->autotrack = TRUE;
                 if( ship->class != SPACE_STATION )
                      ship->currspeed = ship->realspeed;
                 if ( ship->energy >200  )
                    ship->autorecharge=TRUE;
                 
				 if ( ship->shipstate != SHIP_HYPERSPACE
					 && ship->energy > 200
					 && ship->hyperspeed > 0
					 && ship->target->starsystem == ship->starsystem
					 && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 5000
					 && number_bits(2) == 0 )
                 {
                        ship->currjump = ship->starsystem;
                        ship->hyperdistance = 1;
                 
                     	sprintf( buf ,"%s disapears from your scanner." , ship->name );
                     	echo_to_system( AT_YELLOW, ship, buf , NULL );

    			ship_from_starsystem( ship , ship->starsystem );
    			ship->shipstate = SHIP_HYPERSPACE;
        
    			ship->energy -= 100;
    
    			ship->jx = target->vx;
    			ship->jy = target->vy;
    			ship->jz = target->vz;
                 }

                 
                 if (ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25 
                 && ship->missilestate == MISSILE_READY && ship->target->starsystem == ship->starsystem
		 && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) <= 1200
                 && ship->missiles > 0 )
                 {
                   if ( ship->class == SPACE_STATION || is_facing( ship , target ) )
                   {
             		chance -= target->manuever/5;
                        chance -= target->currspeed/20;
                        chance += target->model*target->model*2;
                        chance -= ( abs(target->vx - ship->vx)/100 );
                        chance -= ( abs(target->vy - ship->vy)/100 );
                        chance -= ( abs(target->vz - ship->vz)/100 );                          
                        chance += ( 30 );
                        chance = URANGE( 10 , chance , 90 );

             		if ( number_percent( ) > chance )
             		{
             		} 
                        else
                        {       
                                new_missile( ship , target , NULL );
            		 	ship->missiles-- ;
             		        sprintf( buf , "Incoming missile from %s." , ship->name);  
             		        echo_to_cockpit( AT_BLOOD , target , buf );
             		        sprintf( buf, "%s fires a missile towards %s." , ship->name, target->name );
             		        echo_to_system( AT_ORANGE , target , buf , NULL );

            		 	if ( ship->class == SPACE_STATION )
                                    ship->missilestate = MISSILE_RELOAD_2;
                                else
                                    ship->missilestate = MISSILE_FIRED;
                        }
                   }
                 }
             
                 if( ship->missilestate ==  MISSILE_DAMAGED )
                     ship->missilestate =  MISSILE_READY;
                 if( ship->laserstate ==  LASER_DAMAGED )
                     ship->laserstate =  LASER_READY;
                 if( ship->shipstate ==  SHIP_DISABLED )
                     ship->shipstate =  SHIP_READY;
                                 
             }
             else
             {
                 CLAN_DATA * clan = NULL;
                 CLAN_DATA * shipclan = NULL;
                 ROOM_INDEX_DATA * room;
                 CHAR_DATA *passenger;
                 int targetok;
                 
                 ship->currspeed = 0;
                 
                 for ( clan = first_clan ; clan ; clan = clan->next )
                     if ( !str_cmp( ship->owner , clan->name ) )
                         shipclan = clan;

                 if ( shipclan )
                   for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
                   {
                     targetok = 0;                   

                     if ( IS_SET( target->flags, SHIP_CLOAKED ) )
                        continue;
                     
                     if ( autofly(target) && !str_cmp( ship->owner , target->owner ) )
                        targetok = 1;                                                         
                        
                     for ( room = target->first_room ; room ; room = room->next_in_ship )       
                     {  
                           
                          for ( passenger = room->first_person ; passenger ; passenger = passenger->next_in_room )
                              if ( !IS_NPC(passenger) && passenger->pcdata )
                              {
                                 if ( passenger->pcdata->clan == shipclan )
                                     targetok = 1;
                                 else if ( passenger->pcdata->clan && passenger->pcdata->clan != shipclan 
                                 && targetok == 0 )
                                     if ( nifty_is_name(passenger->pcdata->clan->name , shipclan->atwar ) )
                                           targetok = -1;
                              }
                     }
                     
                     if ( targetok == 1 && target->target )     
                     { 
                           ship->target = target->target; 
                           sprintf( buf , "You are being targetted by %s." , ship->name);  
                           echo_to_cockpit( AT_BLOOD , target->target , buf );
                           break;
                     }

		     if ( targetok == 0 && target->target ) 
                     {
                          if ( !str_cmp( target->target->owner , shipclan->name ) )
			      targetok = -1;
                          else if ( nifty_is_name( target->owner , shipclan->atwar ) )
			      targetok = -1;
                          else 
                            for ( room = target->target->first_room ; room ; room = room->next_in_ship )       
                              for ( passenger = room->first_person ; passenger ; passenger = passenger->next_in_room )
                                if ( !IS_NPC(passenger) && passenger->pcdata && passenger->pcdata->clan == shipclan )
                                     targetok = -1;
                     }
                     
                     if ( targetok >= 0 ) 
                             continue;
                      
                      ship->target = target; 
                      sprintf( buf , "You are being scanned by %s." , ship->name);  
                      echo_to_cockpit( AT_WHITE , target , buf );
                      sprintf( buf , "You are being targetted by %s." , ship->name);  
                      echo_to_cockpit( AT_BLOOD , target , buf );

                      break;

                   }
                 
             }
          }   
          else if ( ( ship->class == SPACE_STATION || ship->type == MOB_SHIP ) && ship->home )
          {
               if ( number_range(1, 25) == 25 )
               {
          	  ship_to_starsystem(ship, starsystem_from_name(ship->home) );  
          	  ship->vx = number_range( -5000 , 5000 );
          	  ship->vy = number_range( -5000 , 5000 );
          	  ship->vz = number_range( -5000 , 5000 );
                  ship->hx = 1;
                  ship->hy = 1;
                  ship->hz = 1;
                  ship->autopilot = TRUE;
               }
          }   
       }
       
       if ( ship->class == SPACE_STATION 
       && ship->target == NULL )
       {
          if( ship->missiles < ship->maxmissiles )
             ship->missiles++;
       }
   }
}

//void update_space( )
//{
//
//   SHIP_DATA *ship;
//   SHIP_DATA *nextship;
//   SHIP_DATA *target;
//   char buf[MAX_STRING_LENGTH];
//   int too_close, target_too_close;
//   int recharge, drain;
// 
//   for ( ship = first_ship; ship; ship = nextship )
//   {    
//        nextship = ship->next;
//   
//        if (ship->starsystem)
//        {
//          if ((ship->energy > 0) && (ship->shipstate == SHIP_DISABLED) && (ship->class != SPACE_STATION))
//          {
//             ship->energy -= 100;
//          }
//          else if ( ship->energy > 0 )
//             ship->energy += ( 5 + ship->model*2 );
//        }
//        
//        if ( ship->chaff_released > 0 )
//           ship->chaff_released--;
//                
//        if (ship->shipstate == SHIP_HYPERSPACE)
//        {
//            ship->hyperdistance -= ship->hyperspeed*2;
//            if (ship->hyperdistance <= 0)
//            {
//            	ship_to_starsystem (ship, ship->currjump);
//            	
//            	if (ship->starsystem == NULL)
//            	{
//            	    echo_to_cockpit( AT_RED, ship, "Ship lost in Hyperspace. Make new calculations.");
//            	}
//            	else
//            	{
//            	    echo_to_room( AT_YELLOW, ship->pilotseat, "Hyperjump complete.");
//            	    echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace.");
//            	    sprintf( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
//            	    echo_to_system( AT_YELLOW, ship, buf , NULL );
//            	    ship->shipstate = SHIP_READY;
//            	    STRFREE( ship->home );
//            	    ship->home = STRALLOC( ship->starsystem->name );
//                    
//            	}
//            }
//            else
//            {
//               sprintf( buf ,"%d" , ship->hyperdistance );
//               echo_to_room_dnr( AT_YELLOW , ship->pilotseat, "Remaining jump distance: " );
//               echo_to_room( AT_WHITE , ship->pilotseat,  buf );
//               
//            }
//        }
//        
//        //  following was originaly to fix ships that lost their pilot 
//        //  in the middle of a manuever and are stuck in a busy state 
//        //  but now used for timed manouevers such as turning 
//    
//    	if (ship->shipstate == SHIP_BUSY_3)
//           {
//              echo_to_room( AT_YELLOW, ship->pilotseat, "Manuever complete.");  
//              ship->shipstate = SHIP_READY;
//           }
//        if (ship->shipstate == SHIP_BUSY_2)
//           ship->shipstate = SHIP_BUSY_3;
//        if (ship->shipstate == SHIP_BUSY)
//           ship->shipstate = SHIP_BUSY_2;
//        
//        if (ship->shipstate == SHIP_LAND_2)
//           landship( ship , ship->dest );
//        if (ship->shipstate == SHIP_LAND)
//           ship->shipstate = SHIP_LAND_2;
//        
//        if (ship->shipstate == SHIP_LAUNCH_2)
//           launchship( ship );
//        if (ship->shipstate == SHIP_LAUNCH)
//           ship->shipstate = SHIP_LAUNCH_2;
//        
//        
//        ship->shield = UMAX( 0 , ship->shield - 1 - ship->model/5);
//                
//        if (ship->autorecharge && ship->maxshield > ship->shield && ship->energy > 100)
//        {
//           recharge  = UMIN( ship->maxshield-ship->shield, 10 + ship->model*2 );           
//           recharge  = UMIN( recharge , ship->energy-100 );
//           recharge  = UMAX( 1, recharge );
//           ship->shield += recharge;
//           ship->energy -= recharge;        
//        }
//
//		drain = 10 + ( ship->maxhull / 600 );
//                if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
//			ship->energy = UMAX( 0, ship->energy - drain );
//
//		if ( IS_SET( ship->flags, SHIP_CLOAKED ) && ship->energy < 200 )
//		{
//			sprintf( buf, "The ship fades into view." );
//			echo_to_cockpit( AT_YELLOW, ship, buf );
//
//			sprintf( buf, "%s fades into view.", ship->name );
//			echo_to_system( AT_YELLOW, ship, buf, NULL );
//
//			REMOVE_BIT( ship->flags, SHIP_CLOAKED );
//		}
//        
//        if (ship->shield > 0)
//        {
//          if (ship->energy < 200)
//          {
//          	ship->shield = 0;
//          	echo_to_cockpit( AT_RED, ship,"The ships shields fizzle and die.");
//                ship->autorecharge = FALSE;
//          }
//        }        
//        
//        if ( ship->starsystem && ship->currspeed > 0 )
//        {
//               sprintf( buf, "%d",
//                          ship->currspeed );
//               echo_to_room_dnr( AT_BLUE , ship->pilotseat,  "Speed: " );
//               echo_to_room_dnr( AT_LBLUE , ship->pilotseat,  buf );
//               sprintf( buf, "%.0f %.0f %.0f",
//                           ship->vx , ship->vy, ship->vz );
//               echo_to_room_dnr( AT_BLUE , ship->pilotseat,  "  Coords: " );
//               echo_to_room( AT_LBLUE , ship->pilotseat,  buf );
//        } 
//
//        if ( ship->starsystem )
//        {
//          too_close = ship->currspeed + 50;
//          for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem)
//          { 
//            target_too_close = too_close+target->currspeed;
//            if ( target != ship
//                 && !IS_SET( target->flags, SHIP_CLOAKED )
//                 && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) < target_too_close )
//            {
//                sprintf( buf, "Proximity alert: %s  %.0f %.0f %.0f"
//                            , target->name, target->vx, target->vy, target->vz);
//                echo_to_room( AT_RED , ship->pilotseat,  buf );    
//            }                
//          }    
//          too_close = ship->currspeed + 100;
//		  if ( ship->starsystem->star1
//			  && strcmp( ship->starsystem->star1, "" )
//			  && find_range( ship->vx, ship->vy, ship->vz, ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z ) < too_close )
//          {
//                sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star1,
//                         ship->starsystem->s1x, ship->starsystem->s1y, ship->starsystem->s1z);
//                echo_to_room( AT_RED , ship->pilotseat,  buf );
//          }
//
//		  if ( ship->starsystem->star2
//			  && strcmp( ship->starsystem->star2, "" )
//			  && find_range( ship->vx, ship->vy, ship->vz, ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z ) < too_close )
//          {
//                sprintf( buf, "Proximity alert: %s  %d %d %d", ship->starsystem->star2,
//                         ship->starsystem->s2x, ship->starsystem->s2y, ship->starsystem->s2z);
//                echo_to_room( AT_RED , ship->pilotseat,  buf );
//          }               
//        }
//       
//
//        if (ship->target)
//        {
//               sprintf( buf, "%s   %.0f %.0f %.0f", ship->target->name,
//                          ship->target->vx , ship->target->vy, ship->target->vz );
//               echo_to_room_dnr( AT_BLUE , ship->pilotseat , "Target: " );
//               echo_to_room( AT_LBLUE , ship->pilotseat ,  buf );
//               if (ship->starsystem != ship->target->starsystem)
//               		ship->target = NULL;
//         }
//         
//         
//   	if (ship->energy < 100 && ship->starsystem )
//   	{
//   	    echo_to_cockpit( AT_RED , ship,  "Warning: Ship fuel low." );
//        }
//                    
//        ship->energy = URANGE( 0 , ship->energy, ship->maxenergy );
//   } 
//
//   for ( ship = first_ship; ship; ship = ship->next )
//   {
//       
//       if (ship->autotrack && ship->target && ship->class < SPACE_STATION )    
//       {
//           target = ship->target;
//           too_close = ship->currspeed + 10; 
//           target_too_close = too_close+target->currspeed;
//		   if ( target != ship
//			   && ship->shipstate == SHIP_READY
//			   && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) < target_too_close )
//           {
//              ship->hx = 0-(ship->target->vx - ship->vx);
//              ship->hy = 0-(ship->target->vy - ship->vy);
//              ship->hz = 0-(ship->target->vz - ship->vz);
//              ship->energy -= ship->currspeed/10;
//              echo_to_room( AT_RED , ship->pilotseat, "Autotrack: Evading to avoid collision!\n\r" );  
//    	      if ( ship->manuever > 100 )
//        	ship->shipstate = SHIP_BUSY_3;
//              else if ( ship->manuever > 50  )
//        	ship->shipstate = SHIP_BUSY_2;
//    	      else
//        	ship->shipstate = SHIP_BUSY;     
//           }
//           else if  ( !is_facing(ship, ship->target) )
//           {
//              ship->hx = ship->target->vx - ship->vx;
//              ship->hy = ship->target->vy - ship->vy;
//              ship->hz = ship->target->vz - ship->vz;
//              ship->energy -= ship->currspeed/10;
//              echo_to_room( AT_BLUE , ship->pilotseat, "Autotracking target ... setting new course.\n\r" );      
//    	      if ( ship->manuever > 100  )
//        	ship->shipstate = SHIP_BUSY_3;
//              else if ( ship->manuever > 50  )
//        	ship->shipstate = SHIP_BUSY_2;
//    	      else
//        	ship->shipstate = SHIP_BUSY;     
//           }
//       }
//
//       if ( autofly(ship) )
//       {
//          if ( ship->starsystem )
//          {
//             if (ship->target)
//             {                 
//                 int chance = 100;
//             
//                 target = ship->target;
//                 ship->autotrack = TRUE;
//                 if( ship->class != SPACE_STATION )
//                      ship->currspeed = ship->realspeed;
//                 if ( ship->energy >200  )
//                    ship->autorecharge=TRUE;
//                 
//				 if ( ship->shipstate != SHIP_HYPERSPACE
//					 && ship->energy > 200
//					 && ship->hyperspeed > 0
//					 && ship->target->starsystem == ship->starsystem
//					 && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 5000
//					 && number_bits(2) == 0 )
//                 {
//                        ship->currjump = ship->starsystem;
//                        ship->hyperdistance = 1;
//                 
//                     	sprintf( buf ,"%s disapears from your scanner." , ship->name );
//                     	echo_to_system( AT_YELLOW, ship, buf , NULL );
//
//    			ship_from_starsystem( ship , ship->starsystem );
//    			ship->shipstate = SHIP_HYPERSPACE;
//        
//    			ship->energy -= 100;
//    
//    			ship->jx = target->vx;
//    			ship->jy = target->vy;
//    			ship->jz = target->vz;
//                 }
//
//                 
//                 if (ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25 
//                 && ship->missilestate == MISSILE_READY && ship->target->starsystem == ship->starsystem
//		 && find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) <= 1200
//                 && ship->missiles > 0 )
//                 {
//                   if ( ship->class == SPACE_STATION || is_facing( ship , target ) )
//                   {
//             		chance -= target->manuever/5;
//                        chance -= target->currspeed/20;
//                        chance += target->model*target->model*2;
//                        chance -= ( abs(target->vx - ship->vx)/100 );
//                        chance -= ( abs(target->vy - ship->vy)/100 );
//                        chance -= ( abs(target->vz - ship->vz)/100 );                          
//                        chance += ( 30 );
//                        chance = URANGE( 10 , chance , 90 );
//
//             		if ( number_percent( ) > chance )
//             		{
//             		} 
//                        else
//                        {       
//                                new_missile( ship , target , NULL );
//            		 	ship->missiles-- ;
//             		        sprintf( buf , "Incoming missile from %s." , ship->name);  
//             		        echo_to_cockpit( AT_BLOOD , target , buf );
//             		        sprintf( buf, "%s fires a missile towards %s." , ship->name, target->name );
//             		        echo_to_system( AT_ORANGE , target , buf , NULL );
//
//            		 	if ( ship->class == SPACE_STATION )
//                                    ship->missilestate = MISSILE_RELOAD_2;
//                                else
//                                    ship->missilestate = MISSILE_FIRED;
//                        }
//                   }
//                 }
//             
//                 if( ship->missilestate ==  MISSILE_DAMAGED )
//                     ship->missilestate =  MISSILE_READY;
//                 if( ship->laserstate ==  LASER_DAMAGED )
//                     ship->laserstate =  LASER_READY;
//                 if( ship->shipstate ==  SHIP_DISABLED )
//                     ship->shipstate =  SHIP_READY;
//                                 
//             }
//             else
//             {
//                 CLAN_DATA * clan = NULL;
//                 CLAN_DATA * shipclan = NULL;
//                 ROOM_INDEX_DATA * room;
//                 CHAR_DATA *passenger;
//                 int targetok;
//                 
//                 ship->currspeed = 0;
//                 
//                 for ( clan = first_clan ; clan ; clan = clan->next )
//                     if ( !str_cmp( ship->owner , clan->name ) )
//                         shipclan = clan;
//
//                 if ( shipclan )
//                   for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
//                   {
//                     targetok = 0;                   
//
//                     if ( IS_SET( target->flags, SHIP_CLOAKED ) )
//                        continue;
//                     
//                     if ( autofly(target) && !str_cmp( ship->owner , target->owner ) )
//                        targetok = 1;                                                         
//                        
//                     for ( room = target->first_room ; room ; room = room->next_in_ship )       
//                     {  
//                           
//                          for ( passenger = room->first_person ; passenger ; passenger = passenger->next_in_room )
//                              if ( !IS_NPC(passenger) && passenger->pcdata )
//                              {
//                                 if ( passenger->pcdata->clan == shipclan )
//                                     targetok = 1;
//                                 else if ( passenger->pcdata->clan && passenger->pcdata->clan != shipclan 
//                                 && targetok == 0 )
//                                     if ( nifty_is_name(passenger->pcdata->clan->name , shipclan->atwar ) )
//                                           targetok = -1;
//                              }
//                     }
//                     
//                     if ( targetok == 1 && target->target )     
//                     { 
//                           ship->target = target->target; 
//                           sprintf( buf , "You are being targetted by %s." , ship->name);  
//                           echo_to_cockpit( AT_BLOOD , target->target , buf );
//                           break;
//                     }
//
//		     if ( targetok == 0 && target->target ) 
//                     {
//                          if ( !str_cmp( target->target->owner , shipclan->name ) )
//			      targetok = -1;
//                          else if ( nifty_is_name( target->owner , shipclan->atwar ) )
//			      targetok = -1;
//                          else 
//                            for ( room = target->target->first_room ; room ; room = room->next_in_ship )       
//                              for ( passenger = room->first_person ; passenger ; passenger = passenger->next_in_room )
//                                if ( !IS_NPC(passenger) && passenger->pcdata && passenger->pcdata->clan == shipclan )
//                                     targetok = -1;
//                     }
//                     
//                     if ( targetok >= 0 ) 
//                             continue;
//                      
//                      ship->target = target; 
//                      sprintf( buf , "You are being scanned by %s." , ship->name);  
//                      echo_to_cockpit( AT_WHITE , target , buf );
//                      sprintf( buf , "You are being targetted by %s." , ship->name);  
//                      echo_to_cockpit( AT_BLOOD , target , buf );
//
//                      break;
//
//                   }
//                 
//             }
//          }   
//          else if ( ( ship->class == SPACE_STATION || ship->type == MOB_SHIP ) && ship->home )
//          {
//               if ( number_range(1, 25) == 25 )
//               {
//          	  ship_to_starsystem(ship, starsystem_from_name(ship->home) );  
//          	  ship->vx = number_range( -5000 , 5000 );
//          	  ship->vy = number_range( -5000 , 5000 );
//          	  ship->vz = number_range( -5000 , 5000 );
//                  ship->hx = 1;
//                  ship->hy = 1;
//                  ship->hz = 1;
//                  ship->autopilot = TRUE;
//               }
//          }   
//       }
//       
//       if ( ship->class == SPACE_STATION 
//       && ship->target == NULL )
//       {
//          if( ship->missiles < ship->maxmissiles )
//             ship->missiles++;
//       }
//   }
//}



void write_starsystem_list( )
{
    SPACE_DATA *tstarsystem;
    FILE *fpout;
    char filename[256];
    
    sprintf( filename, "%s%s", SPACE_DIR, SPACE_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open starsystem.lst for writing!\n\r", 0 );
         return;
    }
    for ( tstarsystem = first_starsystem; tstarsystem; tstarsystem = tstarsystem->next )
    fprintf( fpout, "%s\n", tstarsystem->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}
                                                                    

/*
 * Get pointer to space structure from starsystem name.
 */
SPACE_DATA *starsystem_from_name( char *name )
{
    SPACE_DATA *starsystem;
    
    for ( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
       if ( !str_cmp( name, starsystem->name ) )
         return starsystem;
    
    for ( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
       if ( !str_prefix( name, starsystem->name ) )
         return starsystem;
    
    return NULL;
}

/*
 * Get pointer to space structure from the dock vnun.
 */
SPACE_DATA *starsystem_from_room( ROOM_INDEX_DATA * room )
{
    SHIP_DATA * ship;
    ROOM_INDEX_DATA * sRoom;
        
    if ( room == NULL )
        return NULL;
    
    if ( room->area != NULL && room->area->planet != NULL )
          return room->area->planet->starsystem;

    for ( ship = first_ship; ship; ship = ship->next )
        for ( sRoom = ship->first_room ; sRoom ; sRoom = sRoom->next_in_ship )
           if ( room == sRoom )
              return ship->starsystem;
                                
    return NULL;
}


/*
 * Save a starsystem's data to its data file
 */
void save_starsystem( SPACE_DATA *starsystem )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !starsystem )
    {
	bug( "save_starsystem: null starsystem pointer!", 0 );
	return;
    }
        
    if ( !starsystem->filename || starsystem->filename[0] == '\0' )
    {
	sprintf( buf, "save_starsystem: %s has no filename", starsystem->name );
	bug( buf, 0 );
	return;
    }
 
    sprintf( filename, "%s%s", SPACE_DIR, starsystem->filename );
    
    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_starsystem: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#SPACE\n" );
	fprintf( fp, "Name         %s~\n",	starsystem->name	);
	fprintf( fp, "Filename     %s~\n",	starsystem->filename	);
	fprintf( fp, "Star1        %s~\n",	starsystem->star1	);
	fprintf( fp, "Star2        %s~\n",	starsystem->star2	);
	fprintf( fp, "S1x          %d\n",       starsystem->s1x         );
	fprintf( fp, "S1y          %d\n",       starsystem->s1y         );
	fprintf( fp, "S1z          %d\n",       starsystem->s1z         );
	fprintf( fp, "S2x          %d\n",       starsystem->s2x         );
	fprintf( fp, "S2y          %d\n",       starsystem->s2y         );
	fprintf( fp, "S2z          %d\n",       starsystem->s2z         );
	fprintf( fp, "Flags        %d\n",       starsystem->flags       );
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual starsystem data.
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

void fread_starsystem( SPACE_DATA *starsystem, FILE *fp )
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

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!starsystem->name)
		  starsystem->name		= STRALLOC( "" );
		if (!starsystem->star1)
		  starsystem->star1            = STRALLOC( "" );  
		if (!starsystem->star2)
		  starsystem->star2            = STRALLOC( "" );
		return;
	    }
	    break;
	    
	case 'F':
	    KEY( "Filename",	starsystem->filename,		fread_string_nohash( fp ) );
            KEY( "Flags", starsystem->flags, fread_number( fp ) );
	    break;
        
	case 'N':
	    KEY( "Name",	starsystem->name,		fread_string( fp ) );
	    break;
        
       	
       	case 'S':
       	     KEY( "Star1",	starsystem->star1,	fread_string( fp ) );
	     KEY( "Star2",	starsystem->star2,	fread_string( fp ) );
	     KEY( "S1x",  starsystem->s1x,          fread_number( fp ) ); 
             KEY( "S1y",  starsystem->s1y,          fread_number( fp ) ); 
             KEY( "S1z",  starsystem->s1z,          fread_number( fp ) ); 
             KEY( "S2x",  starsystem->s2x,          fread_number( fp ) ); 
             KEY( "S2y",  starsystem->s2y,          fread_number( fp ) );
             KEY( "S2z",  starsystem->s2z,          fread_number( fp ) );
	    break;
       	}
	
	if ( !fMatch )
	{
	    sprintf( buf, "Fread_starsystem: no match: %s", word );
	    bug( buf, 0 );
	}
    }
}

/*
 * Load a starsystem file
 */

bool load_starsystem( char *starsystemfile )
{
    char filename[256];
    SPACE_DATA *starsystem;
    FILE *fp;
    bool found;

    CREATE( starsystem, SPACE_DATA, 1 );
    starsystem->first_planet = NULL;
    starsystem->last_planet = NULL;
    starsystem->first_ship = NULL;
    starsystem->last_ship = NULL;
    starsystem->first_missile = NULL;
    starsystem->last_missile = NULL;
    starsystem->first_roid = NULL;
    starsystem->last_roid = NULL;
    starsystem->flags = 0;

    found = FALSE;
    sprintf( filename, "%s%s", SPACE_DIR, starsystemfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
        LINK( starsystem, first_starsystem, last_starsystem, next, prev );
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
		bug( "Load_starsystem_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "SPACE"	) )
	    {
	    	fread_starsystem( starsystem, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		char buf[MAX_STRING_LENGTH];

		sprintf( buf, "Load_starsystem_file: bad section: %s.", word );
		bug( buf, 0 );
		break;
	    }
	}
	fclose( fp );
    }

    if ( !(found) )
      DISPOSE( starsystem );

    return found;
}

/*
 * Load in all the starsystem files.
 */
void load_space( )
{
    FILE *fpList;
    char *filename;
    char starsystemlist[256];
    char buf[MAX_STRING_LENGTH];
    
    
    first_starsystem	= NULL;
    last_starsystem	= NULL;

    log_string( "Loading space..." );

    sprintf( starsystemlist, "%s%s", SPACE_DIR, SPACE_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( starsystemlist, "r" ) ) == NULL )
    {
	perror( starsystemlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	if ( filename[0] == '$' )
	  break;
	  
       
	if ( !load_starsystem( filename ) )
	{
	  sprintf( buf, "Cannot load starsystem file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done starsystems " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_setstarsystem( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    SPACE_DATA *starsystem;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg2[0] == '\0' || arg1[0] == '\0' )
    {
	send_to_char( "Usage: setstarsystem <starsystem> <field> <values>\n\r", ch );
	send_to_char( "\n\rField being one of:\n\r", ch );
	send_to_char( "name filename\n\r", ch );
	send_to_char( "star1 s1x s1y s1z\n\r", ch );
	send_to_char( "star2 s2x s2y s2z\n\r", ch );
   ch_printf( ch, "flags\r\n" );
	send_to_char( "", ch );
	return;
    }

    starsystem = starsystem_from_name( arg1 );
    if ( !starsystem )
    {
	send_to_char( "No such starsystem.\n\r", ch );
	return;
    }
    
    if ( !str_cmp( arg2, "s1x" ) )
    {
	starsystem->s1x = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    if ( !str_cmp( arg2, "s1y" ) )
    {
	starsystem->s1y = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    if ( !str_cmp( arg2, "s1z" ) )
    {
	starsystem->s1z = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }

    if ( !str_cmp( arg2, "s2x" ) )
    {
	starsystem->s2x = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    if ( !str_cmp( arg2, "s2y" ) )
    {
	starsystem->s2y = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    if ( !str_cmp( arg2, "s2z" ) )
    {
	starsystem->s2z = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }


    if ( !str_cmp( argument, "name" ) )
    {
	STRFREE( starsystem->name );
	starsystem->name = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    
    if ( !str_cmp( arg2, "star1" ) )
    {
	STRFREE( starsystem->star1 );
	starsystem->star1 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }
    
    if ( !str_cmp( arg2, "star2" ) )
    {
	STRFREE( starsystem->star2 );
	starsystem->star2 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_starsystem( starsystem );
	return;
    }

	if ( !str_cmp( arg2, "flags" ) )
	{      
      int tempnum;
		if ( argument[0] == '\0' || ( tempnum = get_flag( argument, space_flags ) ) == -1 )		
			ch_printf( ch, "That is an invalid flag, possible flags are:\r\n%s\r\n", get_flags( space_flags ) );		
		else
		{		
			TOGGLE_BIT( starsystem->flags, 1 << tempnum );

			ch_printf( ch, "Done.\n\r" );
		}

		return;
	}
    
    do_setstarsystem( ch, "" );
    return;
}

void showstarsystem( CHAR_DATA *ch , SPACE_DATA *starsystem )
{   
    PLANET_DATA * planet;
    
    ch_printf( ch, "Starsystem:%s     Filename: %s\n\r",
    			starsystem->name,
    			starsystem->filename);
    ch_printf( ch, "Star1: %s   Coordinates: %d %d %d\n\r",
    			starsystem->star1,
    			starsystem->s1x , starsystem->s1y, starsystem->s1z);
    ch_printf( ch, "Star2: %s   Coordinates: %d %d %d\n\r",
    			starsystem->star2,
    			starsystem->s2x , starsystem->s2y, starsystem->s2z);

    // TODO: Implement flags here

    for ( planet = starsystem->first_planet ; planet ; planet = planet->next_in_system ) 
        ch_printf( ch, "Planet: %s   Coordinates: %d %d %d\n\r",
    			planet->name, 
    			planet->x , planet->y, planet->z);

   ch_printf( ch, "Flags: %s\n\r", get_setflags( starsystem->flags, space_flags ) );

    return;
}

void do_showstarsystem( CHAR_DATA *ch, char *argument )
{
   SPACE_DATA *starsystem;

   starsystem = starsystem_from_name( argument );
   
   if ( starsystem == NULL )
      send_to_char("&RNo such starsystem.\n\r",ch);
   else
      showstarsystem(ch , starsystem);
   
}

void do_starsystems( CHAR_DATA *ch, char *argument )
{
    SPACE_DATA *starsystem;
    int count = 0;

    for ( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
    {
        set_char_color( AT_NOTE, ch );
        ch_printf( ch, "%s\n\r", starsystem->name );
        count++;
    }

    if ( !count )
    {
        send_to_char( "There are no starsystems currently formed.\n\r", ch );
	return;
    }
}
   
void echo_to_ship( int color , SHIP_DATA *ship , char *argument )
{
     ROOM_INDEX_DATA * room;
     
     for ( room = ship->first_room ; room ; room = room->next_in_ship )
         echo_to_room( color , room , argument );
     
}

void echo_to_cockpit( int color , SHIP_DATA *ship , char *argument )
{
     TURRET_DATA * turret;

     echo_to_room( color , ship->pilotseat , argument );
     if ( ship->pilotseat != ship->gunseat )
        echo_to_room( color , ship->gunseat , argument );
     if ( ship->pilotseat != ship->viewscreen && ship->viewscreen != ship->gunseat )
        echo_to_room( color , ship->viewscreen , argument );

     for ( turret = ship->first_turret ; turret ; turret = turret->next )
        if ( turret->room )
            echo_to_room( color , turret->room , argument );
}

void echo_to_system( int color , SHIP_DATA *ship , char *argument , SHIP_DATA *ignore )
{
     SHIP_DATA *target;
     
     if (!ship->starsystem)
        return;
      
     for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
     {
       if (target != ship && target != ignore )  
         echo_to_cockpit( color , target , argument );
     }  
     
}

bool is_facing( SHIP_DATA *ship , SHIP_DATA *target )
{
    	float dy, dx, dz, hx, hy, hz;
        float cosofa;
     	     	
     	hx = ship->hx;
     	hy = ship->hy;
     	hz = ship->hz;
     	     				
     	     					dx = target->vx - ship->vx;
     	     						dy = target->vy - ship->vy;
     	     							dz = target->vz - ship->vz;
     	     							
     	    cosofa = ( hx*dx + hy*dy + hz*dz ) 
     	     		/ ( sqrt(hx*hx+hy*hy+hz*hz) + sqrt(dx*dx+dy*dy+dz*dz) );
     	     								               
     	           	if ( cosofa > 0.75 )
     	                  	             return TRUE;
     	     								               	             
     	     								 return FALSE;
     	}
     	     								               	             	

void write_ship_list( )
{
    SHIP_DATA *tship;
    FILE *fpout;
    char filename[256];
    
    sprintf( filename, "%s%s", SHIP_DIR, SHIP_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open ship.lst for writing!\n\r", 0 );
         return;
    }
    for ( tship = first_ship; tship; tship = tship->next )
    if ( tship->type != MOB_SHIP && tship->owner && tship->owner[0] != '\0' )
      fprintf( fpout, "%s\n", tship->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}
                                                                    
SHIP_DATA * ship_in_room( ROOM_INDEX_DATA *room, char *name )
{
    SHIP_DATA *ship;

    if ( !room )
     return NULL;
     
     
    for ( ship = room->first_ship ; ship ; ship = ship->next_in_room )
     if ( !str_cmp( name, ship->name ) )
      if ( ship->owner && ship->owner[0] != '\0' )
       if ( get_clan( ship->owner ) || is_online( ship->owner )
       || is_online( ship->pilot ) || is_online( ship->copilot ))  
         return ship;
    
    for ( ship = room->first_ship ; ship ; ship = ship->next_in_room )
     if ( nifty_is_name( name, ship->name ) )
      if ( ship->owner && ship->owner[0] != '\0' )
       if ( get_clan( ship->owner ) || is_online( ship->owner )
       || is_online( ship->pilot ) || is_online( ship->copilot ))  
         return ship;

    for ( ship = room->first_ship ; ship ; ship = ship->next_in_room )
     if ( !str_prefix( name, ship->name ) )
      if ( ship->owner && ship->owner[0] != '\0' )
       if ( get_clan( ship->owner ) || is_online( ship->owner )
       || is_online( ship->pilot ) || is_online( ship->copilot ))  
         return ship;
    
    for ( ship = room->first_ship ; ship ; ship = ship->next_in_room )
     if ( nifty_is_name_prefix( name, ship->name ) )
      if ( ship->owner && ship->owner[0] != '\0' )
       if ( get_clan( ship->owner ) || is_online( ship->owner )
       || is_online( ship->pilot ) || is_online( ship->copilot ))  
         return ship;
    
    return NULL;    
}

/*
 * Get pointer to ship structure from ship name.
 */
SHIP_DATA *get_ship( char *name )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
       if ( !str_cmp( name, ship->name ) )
         return ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
       if ( nifty_is_name( name, ship->name ) )
         return ship;

    for ( ship = first_ship; ship; ship = ship->next )
       if ( !str_prefix( name, ship->name ) )
         return ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
       if ( nifty_is_name_prefix( name, ship->name ) )
         return ship;
    
    return NULL;
}

SHIP_DATA *get_ship_filename( char *name )
{
    SHIP_DATA *ship;

    for( ship = first_ship; ship; ship = ship->next )
	 if( !str_cmp( name, ship->filename ) )
		return ship;

    return NULL;
}

/*
 * Checks if ships in a starsystem and returns poiner if it is.
 */
SHIP_DATA *get_ship_here( char *name , SPACE_DATA *starsystem)
{
    SHIP_DATA *ship;
    
    if ( starsystem == NULL )
         return NULL;
    
    for ( ship = starsystem->first_ship ; ship; ship = ship->next_in_starsystem )
       if ( !str_cmp( name, ship->name ) )
         return ship;
    
    for ( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
       if ( nifty_is_name( name, ship->name ) )
         return ship;

    for ( ship = starsystem->first_ship ; ship; ship = ship->next_in_starsystem )
       if ( !str_prefix( name, ship->name ) )
         return ship;
    
    for ( ship = starsystem->first_ship; ship; ship = ship->next_in_starsystem )
       if ( nifty_is_name_prefix( name, ship->name ) )
         return ship;
    
    return NULL;
}

ASTEROID_DATA *get_asteroid_here( char *name , SPACE_DATA *starsystem)
{
    ASTEROID_DATA *asteroid;
    
    if ( starsystem == NULL )
         return NULL;
    
    for ( asteroid = starsystem->first_roid ; asteroid; asteroid = asteroid->next )
       if ( !str_cmp( name, asteroid->name ) )
         return asteroid;
    
    for ( asteroid = starsystem->first_roid; asteroid; asteroid = asteroid->next )
       if ( nifty_is_name( name, asteroid->name ) )
         return asteroid;

    for ( asteroid = starsystem->first_roid ; asteroid; asteroid = asteroid->next )
       if ( !str_prefix( name, asteroid->name ) )
         return asteroid;
    
    for ( asteroid = starsystem->first_roid; asteroid; asteroid = asteroid->next )
       if ( nifty_is_name_prefix( name, asteroid->name ) )
         return asteroid;
    
    return NULL;
} //phel

SHIP_DATA *ship_from_pilot( char *name )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
       if ( !str_cmp( name, ship->pilot ) )
         return ship;
       if ( !str_cmp( name, ship->copilot ) )
         return ship;
       if ( !str_cmp( name, ship->owner ) )
         return ship;  
    return NULL;
}


/*
 * Get pointer to ship structure from cockpit, or entrance ramp vnum.
 */
 
SHIP_DATA *ship_from_room( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;
   ROOM_INDEX_DATA *sRoom;
   
   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
     for ( sRoom = ship->first_room ; sRoom ; sRoom = sRoom->next_in_ship ) 
        if ( room == sRoom )
              return ship;

    return NULL;
}

SHIP_DATA *ship_from_cockpit( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;
   TURRET_DATA *turret;
   
   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
   {
        if ( room == ship->pilotseat )
              return ship;
        if ( room == ship->gunseat )
              return ship;
        if ( room == ship->viewscreen )
              return ship;
        for ( turret = ship->first_turret ; turret ; turret = turret->next )
            if ( room == turret->room )
                  return ship;
   }
   
   return NULL;
}

SHIP_DATA *ship_from_pilotseat( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;

   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
        if ( room == ship->pilotseat )
              return ship;
   
   return NULL;
}

SHIP_DATA *ship_from_gunseat( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;

   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
        if ( room == ship->gunseat )
              return ship;
   
   return NULL;
}

SHIP_DATA *ship_from_entrance( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;

   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
        if ( room == ship->entrance )
              return ship;

    return NULL;
}
SHIP_DATA *ship_from_engine( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;

   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
        if ( room == ship->engine )
              return ship;

    return NULL;
}

SHIP_DATA *ship_from_turret( ROOM_INDEX_DATA * room )
{
   SHIP_DATA *ship;
   TURRET_DATA *turret;
   
   if ( room == NULL )
      return NULL;
   
   for ( ship = first_ship; ship; ship = ship->next )
   {
        if ( room == ship->gunseat )
              return ship;
        for ( turret = ship->first_turret ; turret ; turret = turret->next )
            if ( room == turret->room )
                  return ship;
   }
   
    return NULL;
}

// This returns the ship given a the current room, if the current room is a hangar.
// It will return NULL if it can't find the ship from the room and/or the room isn't a hangar.
SHIP_DATA * ship_from_hangar( ROOM_INDEX_DATA * room )
{
   SHIP_DATA * ship;
   HANGAR_DATA * hangar;

   // Check if the room is NULL, no need to continue if it is.
   if ( room == NULL )
      return NULL;

   // Go through all the ships, which I don't think is very efficient.
   for ( ship = first_ship; ship; ship = ship->next ) 
   {
      // Go through every hangar on the current ship.
      for ( hangar = ship->first_hangar; hangar; hangar = hangar->next )
      {
         // Check if the room is a hangar on the current ship, if it is return the ship.
         if ( room == hangar->room )
            return ship;
      }
   }

   // If it gets here then it wasn't found.
   return NULL;
}



void save_ship( SHIP_DATA *ship )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *room;
    SHIP_DATA *onship;

    onship = ship_from_room( ship->in_room );
    if ( !ship )
    {
	bug( "save_ship: null ship pointer!", 0 );
	return;
    }

    if ( ship->type == MOB_SHIP )
       return;

    if ( !ship->filename || ship->filename[0] == '\0' )
    {
	sprintf( buf, "save_ship: %s has no filename", ship->name );
	bug( buf, 0 );
	return;
    }

    sprintf( filename, "%s%s", SHIP_DIR, ship->filename );

    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_ship: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#SHIP\n" );
	fprintf( fp, "Name         %s~\n",	ship->name		);
	fprintf( fp, "Filename     %s~\n",	ship->filename		);
        for ( room = ship->first_room ; room ; room = room->next_in_ship )
            fprintf( fp, "Description  %s~\n",	room->description );
	fprintf( fp, "Owner        %s~\n",	ship->owner		);
	fprintf( fp, "Pilot        %s~\n",      ship->pilot             );
	fprintf( fp, "Copilot      %s~\n",      ship->copilot           );
	fprintf( fp, "Model        %d\n",	ship->model		);
        if( ship->model == CUSTOM_SHIP )
              fprintf( fp, "Custom       %s~\n",       ship->customshipstring  );
	fprintf( fp, "Class        %d\n",	ship->class		);
	fprintf( fp, "Tractorbeam  %d\n",	ship->tractorbeam	);
	fprintf( fp, "Shipyard     %d\n",	ship->shipyard		);
	fprintf( fp, "Laserstate   %d\n",	ship->laserstate	);
	fprintf( fp, "Lasers       %d\n",	ship->lasers    	);
	fprintf( fp, "Missiles     %d\n",	ship->missiles		);
	fprintf( fp, "Maxmissiles  %d\n",	ship->maxmissiles	);
	fprintf( fp, "Lastdoc      %d\n",	ship->lastdoc		);
	fprintf( fp, "Shield       %d\n",	ship->shield		);
	fprintf( fp, "Bounty	  %ld\n", 	ship->bounty);
	fprintf( fp, "Maxshield    %d\n",	ship->maxshield		);
	fprintf( fp, "Hull         %d\n",	ship->hull		);
	fprintf( fp, "Maxhull      %d\n",	ship->maxhull		);
	fprintf( fp, "Maxenergy    %d\n",	ship->maxenergy		);
	fprintf( fp, "Hyperspeed   %d\n",	ship->hyperspeed	);
	fprintf( fp, "Chaff        %d\n",	ship->chaff		);
	fprintf( fp, "Maxchaff     %d\n",	ship->maxchaff		);
	fprintf( fp, "Realspeed    %d\n",	ship->realspeed		);
	fprintf( fp, "Type         %d\n",	ship->type		);
	fprintf( fp, "Shipstate    %d\n",	ship->shipstate		);
	fprintf( fp, "Missilestate %d\n",	ship->missilestate	);
	fprintf( fp, "Energy       %d\n",	ship->energy		);
	fprintf( fp, "Manuever     %d\n",       ship->manuever          );
	fprintf( fp, "Home         %s~\n",      ship->home              );	
	if( onship )
		fprintf( fp, "Onship        %s~\n", onship->filename );
	else
		fprintf( fp, "Onship        none~\n");
	fprintf( fp, "Price        %d\n",       ship->price             );
	fprintf( fp, "Modules      %d\n",       ship->modules           );
	fprintf( fp, "Modnums      %d %d %d %d %d\n",
		ship->module[0], ship->module[1], ship->module[2],
                ship->module[3], ship->module[4] );
	fprintf( fp, "Flags        %d\n", ship->flags );
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual ship data.
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

void fread_ship( SHIP_DATA *ship, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int dIndex = 0;

	ship->flags = 0;
 
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
        

        case 'B':
		KEY("Bounty", ship->bounty, fread_number(fp) );
		break;

	case 'C':
             KEY( "Class",       ship->class,            fread_number( fp ) );
             KEY( "Copilot",     ship->copilot,          fread_string( fp ) );
             KEY( "Chaff",       ship->chaff,      fread_number( fp ) );
             KEY( "Custom",      ship->customshipstring,  fread_string( fp ) );
             break;


	case 'D':
	    if ( dIndex < MAX_SHIP_ROOMS )
	         KEY( "Description",	   ship->description[dIndex++],	fread_string( fp ) );
	    break;

	case 'E':
	    KEY( "Energy",      ship->energy,        fread_number( fp ) );
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!ship->home)
		  ship->home		= STRALLOC( "" );
		if (!ship->name)
		  ship->name		= STRALLOC( "" );
		if (!ship->owner)
		  ship->owner		= STRALLOC( "" );
		if (!ship->copilot)
		  ship->copilot 	= STRALLOC( "" );
		if (!ship->pilot)
		  ship->pilot   	= STRALLOC( "" );
                if( !ship->customshipstring )
                  ship->customshipstring = STRALLOC( "" );
		if (ship->shipstate != SHIP_DISABLED)
		  ship->shipstate = SHIP_DOCKED;
		if (ship->laserstate != LASER_DAMAGED)
		  ship->laserstate = LASER_READY;
		if (ship->missilestate != MISSILE_DAMAGED)
		  ship->missilestate = MISSILE_READY;
		if (ship->shipyard <= 0)
		  ship->shipyard = ROOM_LIMBO_SHIPYARD;
		if (ship->lastdoc <= 0) 
		  ship->lastdoc = ship->shipyard;
                if( ship->modules != 5 )
		{
                	ship->modules = 5;
			ship->module[0] = MODULE_NONE;
			ship->module[1] = MODULE_NONE;
			ship->module[2] = MODULE_NONE;
			ship->module[3] = MODULE_NONE;
			ship->module[4] = MODULE_NONE;
		}
		ship->autopilot   = FALSE;
		ship->hatchopen = FALSE;
		ship->starsystem = NULL;
		ship->energy = ship->maxenergy;
		ship->hull = ship->maxhull;
		ship->in_room=NULL;
                ship->next_in_room=NULL;
                ship->prev_in_room=NULL;
                ship->first_turret=NULL;
                ship->last_turret=NULL;
                ship->first_hangar=NULL;
                ship->last_hangar=NULL;
		create_ship_rooms(ship);
		return;
	    }
	    break;
	    
	case 'F':
	    KEY( "Filename",	ship->filename,		fread_string_nohash( fp ) );		
		KEY( "Flags", ship->flags, fread_number( fp ) );
            break;
        
        case 'H':
            KEY( "Home" , ship->home, fread_string( fp ) );
            KEY( "Hyperspeed",   ship->hyperspeed,      fread_number( fp ) );
            KEY( "Hull",      ship->hull,        fread_number( fp ) );
            break;
        
        case 'L':
            KEY( "Lasers",   ship->lasers,      fread_number( fp ) );
            KEY( "Laserstate",   ship->lasers,      fread_number( fp ) );
            KEY( "Lastdoc",    ship->lastdoc,       fread_number( fp ) );
            break;

        case 'M':
            KEY( "Model",   ship->model,      fread_number( fp ) );
            KEY( "Manuever",   ship->manuever,      fread_number( fp ) );
            KEY( "Maxmissiles",   ship->maxmissiles,      fread_number( fp ) );
            KEY( "Missiles",   ship->missiles,      fread_number( fp ) );
            KEY( "Maxshield",      ship->maxshield,        fread_number( fp ) );
            KEY( "Maxenergy",      ship->maxenergy,        fread_number( fp ) );
            KEY( "Missilestate",   ship->missilestate,        fread_number( fp ) );
            KEY( "Maxhull",      ship->maxhull,        fread_number( fp ) );
            KEY( "Maxchaff",       ship->maxchaff,      fread_number( fp ) );
	    KEY( "Modules",        ship->modules,       fread_number( fp ) );
	    if ( !str_cmp( word, "Modnums" ) )
	    {
		ship->module[0] = fread_number( fp );
		ship->module[1] = fread_number( fp );
		ship->module[2] = fread_number( fp );
		ship->module[3] = fread_number( fp );
		ship->module[4] = fread_number( fp );
		fMatch = TRUE;
		break;
	    }

            break;

	case 'N':
	    KEY( "Name",	ship->name,		fread_string( fp ) );
            break;

        case 'O':
            KEY( "Owner",            ship->owner,            fread_string( fp ) );
	    KEY( "Onship",	     ship->onship,	     fread_string(fp) );
            break;

        case 'P':
            KEY( "Pilot",            ship->pilot,            fread_string( fp ) );
            KEY( "Price",            ship->price,            fread_number( fp ) );
            break;

        case 'R':
            KEY( "Realspeed",   ship->realspeed,       fread_number( fp ) );
            break;
       
        case 'S':
            KEY( "Shipyard",    ship->shipyard,      fread_number( fp ) );
            KEY( "Shield",      ship->shield,        fread_number( fp ) );
            KEY( "Shipstate",   ship->shipstate,        fread_number( fp ) );
            break;

	case 'T':
	    KEY( "Type",	ship->type,	fread_number( fp ) );
	    KEY( "Tractorbeam", ship->tractorbeam,      fread_number( fp ) );
	    break;
	}
	
	if ( !fMatch )
	{
	    sprintf( buf, "Fread_ship: no match: %s", word );
	    bug( buf, 0 );
	}
    }
}

/*
 * Load a ship file
 */

bool load_ship_file( char *shipfile )
{
    char filename[256];
    SHIP_DATA *ship;
    SHIP_DATA *onship;
    FILE *fp;
    bool found;
    CLAN_DATA *clan;
        
    CREATE( ship, SHIP_DATA, 1 );

    found = FALSE;
    sprintf( filename, "%s%s", SHIP_DIR, shipfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
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
		bug( "Load_ship_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "SHIP"	) )
	    {
	    	fread_ship( ship, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		char buf[MAX_STRING_LENGTH];

		sprintf( buf, "Load_ship_file: bad section: %s.", word );
		bug( buf, 0 );
		break;
	    }
	}
	fclose( fp );
    }
    if ( !(found) )
      DISPOSE( ship );
    else
    {      
       LINK( ship, first_ship, last_ship, next, prev );
       if ( ship->class == SPACE_STATION || ship->type == MOB_SHIP )
       {
       
     	ship->currspeed=0;
    	ship->energy=ship->maxenergy;
     	ship->chaff=ship->maxchaff;
     	ship->hull=ship->maxhull;
     	ship->shield=0;
     
     	ship->laserstate = LASER_READY; 
    	ship->missilestate = LASER_READY;
       
     	ship->currjump=NULL;
     	ship->target=NULL;
     
     	ship->hatchopen = FALSE;
     
     	ship->missiles = ship->maxmissiles;
     	ship->autorecharge = FALSE;
     	ship->autotrack = FALSE;
     	ship->autospeed = FALSE;
          
          ship_to_starsystem(ship, starsystem_from_name(ship->home) );  
          ship->vx = number_range( -5000 , 5000 );
          ship->vy = number_range( -5000 , 5000 );
          ship->vz = number_range( -5000 , 5000 );
          ship->hx = 1;
          ship->hy = 1;
          ship->hz = 1;
          ship->shipstate = SHIP_READY;
          ship->autopilot = TRUE;
          ship->autorecharge = TRUE;
          ship->shield = ship->maxshield;
       
       }
       else if( str_cmp( ship->onship, "none" ) )
       {
	  onship = get_ship_filename( ship->onship );
	    if( onship )
	    {
		ship_to_hangar( ship, onship->first_hangar->room );
		ship->lastdoc = 0;
		ship->location = ship->lastdoc;
		return found;
	    }
       }
       else
       {
          ship_to_room( ship , ship->lastdoc );
          ship->location = ship->lastdoc;
       }
              
       if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
       {
          if ( ship->class <= SPACE_STATION )
             clan->spacecraft++;
          else
             clan->vehicles++;
       }  
         
    }
    
    return found;
}

/*
 * Load in all the ship files.
 */
void load_ships( )
{
    FILE *fpList;
    char *filename;
    char shiplist[256];
    char buf[MAX_STRING_LENGTH];
    
    
    first_ship	= NULL;
    last_ship	= NULL;
    first_missile = NULL;
    last_missile = NULL;
    
    log_string( "Loading ships..." );

    sprintf( shiplist, "%s%s", SHIP_DIR, SHIP_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( shiplist, "r" ) ) == NULL )
    {
	perror( shiplist );
	exit( 1 );
    }

    for ( ; ; )
    {
    
	filename = feof( fpList ) ? "$" : fread_word( fpList );

	if ( filename[0] == '$' )
	  break;
	         
	if ( !load_ship_file( filename ) )
	{
	  sprintf( buf, "Cannot load ship file: %s", filename );
	  bug( buf, 0 );
	}

    }
    fclose( fpList );
    log_string(" Done ships " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void resetship( SHIP_DATA *ship )
{
     ship->shipstate = SHIP_READY;
     
     if ( ship->class != SPACE_STATION && ship->type != MOB_SHIP )
     {
           extract_ship( ship );
           ship_to_room( ship , ship->lastdoc ); 
           ship->location = ship->lastdoc;
           ship->shipstate = SHIP_DOCKED;
           STRFREE( ship->home );
           ship->home = STRALLOC( "" );
     }
     
     if (ship->starsystem)
        ship_from_starsystem( ship, ship->starsystem );  
  
     ship->currspeed=0;
     ship->energy=ship->maxenergy;
     ship->chaff=ship->maxchaff;
     ship->hull=ship->maxhull;
     ship->shield=0;
     
     ship->laserstate = LASER_READY; 
     ship->missilestate = LASER_READY;
       
     ship->currjump=NULL;
     ship->target=NULL;
     
     ship->hatchopen = FALSE;
     
     ship->missiles = ship->maxmissiles;
     ship->autorecharge = FALSE;
     ship->autotrack = FALSE;
     ship->autospeed = FALSE;
     
     save_ship(ship);               
}

void do_resetship( CHAR_DATA *ch, char *argument )
{    
     SHIP_DATA *ship;
     
     ship = get_ship( argument );
     if (ship == NULL)
     {
        send_to_char("&RNo such ship!",ch);
        return;
     } 
     
     resetship( ship ); 
     
     if ( ( ship->class == SPACE_STATION || ship->type == MOB_SHIP ) 
          && ship->home )
     {
          ship_to_starsystem(ship, starsystem_from_name(ship->home) );  
          ship->vx = number_range( -5000 , 5000 );
          ship->vy = number_range( -5000 , 5000 );
          ship->vz = number_range( -5000 , 5000 );
          ship->shipstate = SHIP_READY;
          ship->autopilot = TRUE;
          ship->autorecharge = TRUE;
          ship->shield = ship->maxshield;
     }
         
}

void do_setship( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    SHIP_DATA *ship;
    int  tempnum;
    ROOM_INDEX_DATA *roomindex;
    
    if ( IS_NPC(ch) || !ch->pcdata )
    	return;

    if ( !ch->desc )
	return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg1[0] == '\0' )
    {
	send_to_char( "Usage: setship <ship> <field> <values>\n\r", ch );
	send_to_char( "\n\rField being one of:\n\r", ch );
	send_to_char( "name owner copilot pilot home\n\r", ch );
	send_to_char( "manuever speed hyperspeed tractorbeam\n\r", ch );
	send_to_char( "lasers missiles shield hull energy chaff\n\r", ch );
	send_to_char( "lastdoc class flags\n\r", ch );
	return;
    }

    ship = get_ship( arg1 );
    if ( !ship )
    {
	send_to_char( "No such ship.\n\r", ch );
	return;
    }
    
    if ( !str_cmp( arg2, "owner" ) )
    {
         CLAN_DATA *clan;
         if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
         {
          if ( ship->class <= SPACE_STATION )
             clan->spacecraft--;
          else
             clan->vehicles--;
         }
	STRFREE( ship->owner );
	ship->owner = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
         {
          if ( ship->class <= SPACE_STATION )
             clan->spacecraft++;
          else
             clan->vehicles++;
         }
	return;
    }
    
    if ( !str_cmp( arg2, "home" ) )
    {
	STRFREE( ship->home );
	ship->home = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
    
    if ( !str_cmp( arg2, "pilot" ) )
    {
	STRFREE( ship->pilot );
	ship->pilot = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "copilot" ) )
    {
	STRFREE( ship->copilot );
	ship->copilot = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
   

    if ( !str_cmp( arg2, "shipyard" ) )
    {   
        tempnum = atoi(argument); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.",ch);
    	   return;
    	} 
	ship->shipyard = tempnum;
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "name" ) )
    {
	STRFREE( ship->name );
	ship->name = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "manuever" ) )
    {
	ship->manuever = URANGE( 0, atoi(argument) , 255 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "lasers" ) )
    {   
	ship->lasers = URANGE( 0, atoi(argument) , 10 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
    
    if ( !str_cmp( arg2, "class" ) )
    {   
	ship->class = URANGE( 0, atoi(argument) , 9 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "lastdoc" ) )
    {   
	ship->lastdoc = URANGE( 0, atoi(argument) , 9 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }


    if ( !str_cmp( arg2, "missiles" ) )
    {   
	ship->maxmissiles = URANGE( 0, atoi(argument) , 255 );
	ship->missiles = URANGE( 0, atoi(argument) , 255 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "speed" ) )
    {   
	ship->realspeed = URANGE( 0, atoi(argument) , 255 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "tractorbeam" ) )
    {
	ship->tractorbeam = URANGE( 0, atoi(argument) , 255 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "hyperspeed" ) )
    {   
	ship->hyperspeed = URANGE( 0, atoi(argument) , 255 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "shield" ) )
    {   
	ship->maxshield = URANGE( 0, atoi(argument) , 1000 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "hull" ) )
    {   
	ship->hull = URANGE( 1, atoi(argument) , 20000 );
	ship->maxhull = URANGE( 1, atoi(argument) , 20000 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "energy" ) )
    {   
	ship->energy = URANGE( 1, atoi(argument) , 30000 );
	ship->maxenergy = URANGE( 1, atoi(argument) , 30000 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "chaff" ) )
    {   
	ship->chaff = URANGE( 0, atoi(argument) , 25 );
	ship->maxchaff = URANGE( 0, atoi(argument) , 25 );
	send_to_char( "Done.\n\r", ch );
	save_ship( ship );
	return;
    }

	if ( !str_cmp( arg2, "flags" ) )
	{      
		if ( argument[0] == '\0' || ( tempnum = get_flag( argument, ship_flags ) ) == -1 )		
			ch_printf( ch, "That is an invalid flag, possible flags are:\r\n%s\r\n", get_flags( ship_flags ) );		
		else
		{		
			TOGGLE_BIT( ship->flags, 1 << tempnum );

			ch_printf( ch, "Done.\n\r" );
		}

		return;
	}
	
 
    do_setship( ch, "" );
    return;
}

void do_showship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showship <ship>\n\r", ch );
	return;
    }

    ship = get_ship( argument );
    if ( !ship )
    {
	send_to_char( "No such ship.\n\r", ch );
	return;
    }
    set_char_color( AT_YELLOW, ch );
    ch_printf( ch, "%s %s : %s\n\rFilename: %s\n\r",
		        ship->type == PLAYER_SHIP ? "Player" : "Mob" ,
		        ship->class == SPACECRAFT ? model[ship->model].name :
		       (ship->class == SPACE_STATION ? "Space Station" : 
		       (ship->class == AIRCRAFT ? "Aircraft" : 
		       (ship->class == BOAT ? "Boat" : 
		       (ship->class == SUBMARINE ? "Submarine" : 
		       (ship->class == LAND_VEHICLE ? "land vehicle" : "Unknown" ) ) ) ) ), 
    			ship->name,
    			ship->filename);
    ch_printf( ch, "Home: %s\n\rOwner: %s   Pilot: %s   Copilot: %s\n\r",
    			ship->home,
    			ship->owner, ship->pilot,  ship->copilot );
    ch_printf( ch, "Location: %d   Lastdoc: %d   Shipyard: %d\n\r",
    			ship->location,
    			ship->lastdoc,
    			ship->shipyard);
    ch_printf( ch, "Tractor Beam: %d  ",
    			ship->tractorbeam);
    ch_printf( ch, "Lasers: %d  Laser Condition: %s\n\r",
    			ship->lasers,
    			ship->laserstate == LASER_DAMAGED ? "Damaged" : "Good");		
    ch_printf( ch, "Missiles: %d/%d  Condition: %s\n\r",
       			ship->missiles,
    			ship->maxmissiles,
    			ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good");		
    ch_printf( ch, "Hull: %d/%d  Ship Condition: %s\n\r",
                        ship->hull,
    		        ship->maxhull,	
    			ship->shipstate == SHIP_DISABLED ? "Disabled" : "Running");
    		
    ch_printf( ch, "Shields: %d/%d   Energy(fuel): %d/%d   Chaff: %d/%d\n\r",
                        ship->shield,
    		        ship->maxshield,
    		        ship->energy,
    		        ship->maxenergy,
    		        ship->chaff,
    		        ship->maxchaff);
    ch_printf( ch, "Current Coordinates: %.0f %.0f %.0f\n\r",
                        ship->vx, ship->vy, ship->vz );
    ch_printf( ch, "Current Heading: %.0f %.0f %.0f\n\r",
                        ship->hx, ship->hy, ship->hz );
    ch_printf( ch, "Speed: %d/%d   Hyperspeed: %d\n\r  Manueverability: %d\r\n",
                        ship->currspeed, ship->realspeed, ship->hyperspeed , ship->manuever );

	ch_printf(ch, "Modules: %d %d %d %d %d\r\n",
		ship->module[0], ship->module[1], ship->module[2],
		ship->module[3], ship->module[4] );	

	ch_printf( ch, "Flags: %s\n\r", get_setflags( ship->flags, ship_flags ) );

    return;
}

void do_ships( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count;
    
    if ( !IS_NPC(ch) )
    {
      count = 0;
      send_to_char( "&YThe following ships are owned by you or by your organization:\n\r", ch );
      send_to_char( "\n\r&WShip                               Location\n\r",ch);
      for ( ship = first_ship; ship; ship = ship->next )
      {   
        if ( str_cmp(ship->owner, ch->name) )
        {
           if ( !ch->pcdata || !ch->pcdata->clan || str_cmp(ship->owner,ch->pcdata->clan->name) || ship->class > SPACE_STATION )
               continue;
        }
         
        if (ship->type == MOB_SHIP)
           continue;
        set_char_color( AT_BLUE, ch );
        
        if  ( ship->in_room )       
        {
          if ( ship->in_room->area && ship->in_room->area->planet )
             ch_printf( ch, "%s  -%s (%s)\n\r", ship->name, ship->in_room->name , ship->in_room->area->planet->name );
          else
             ch_printf( ch, "%s  -%s\n\r", ship->name, ship->in_room->name );
        }
        else 
          ch_printf( ch, "%s\n\r", ship->name );
        
        count++;
      }

      if ( !count )
      {
        send_to_char( "There are no ships owned by you.\n\r", ch );
      }
    
    }

    
}

void do_speeders( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count;

    if ( !IS_NPC(ch) )
    {
      count = 0;
      send_to_char( "&YThe following are owned by you or by your organization:\n\r", ch );
      send_to_char( "\n\r&WVehicle                            Location\n\r",ch);
      for ( ship = first_ship; ship; ship = ship->next )
      {   
        if ( str_cmp(ship->owner, ch->name) )
        {
           if ( !ch->pcdata || !ch->pcdata->clan || str_cmp(ship->owner,ch->pcdata->clan->name) || ship->class <= SPACE_STATION )
               continue;
        }
        if ( ship->location != ch->in_room->vnum || ship->class <= SPACE_STATION)
               continue;
               
        if (ship->type == MOB_SHIP)
           continue;
        set_char_color( AT_BLUE, ch );
                
        if  ( ship->in_room )       
        {
          if ( ship->in_room->area && ship->in_room->area && ship->in_room->area->planet )
             ch_printf( ch, "%s  -%s (%s)\n\r", ship->name, ship->in_room->name, ship->in_room->area->planet->name );
          else
             ch_printf( ch, "%s  -%s\n\r", ship->name, ship->in_room->name );
        }
        else 
          ch_printf( ch, "%s\n\r", ship->name );
        
        count++;
      }

      if ( !count )
      {
        send_to_char( "There are no land or air vehicles owned by you.\n\r", ch );
      }
    
    }

    
}

void do_allspeeders( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count = 0;

      count = 0;
      send_to_char( "&Y\n\rThe following sea/land/air vehicles are currently formed:\n\r", ch );
    
      send_to_char( "\n\r&WVehicle                            Owner\n\r", ch );
      for ( ship = first_ship; ship; ship = ship->next )
      {   
        if ( ship->class <= SPACE_STATION ) 
           continue; 
      
        if (ship->type == MOB_SHIP)
           continue;
        set_char_color( AT_BLUE, ch );
        
        
        ch_printf( ch, "%-35s %-15s\n\r", ship->name, ship->owner );

        count++;
      }
    
      if ( !count )
      {
        send_to_char( "There are none currently formed.\n\r", ch );
	return;
      }
    
}

void do_allships( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count = 0;

      count = 0;
      send_to_char( "&Y\n\rThe following ships are currently formed:\n\r", ch );
    
      send_to_char( "\n\r&WShip                               Owner\n\r", ch );
      
      for ( ship = first_ship; ship; ship = ship->next )
      {   
        if ( ship->class > SPACE_STATION ) 
           continue; 
      
        if (ship->type == MOB_SHIP)
           continue;
        set_char_color( AT_BLUE, ch );
        
        ch_printf( ch, "%-35s %-15s\n\r", ship->name, ship->owner );
        
        count++;
      }
    
      if ( !count )
      {
        send_to_char( "There are no ships currently formed.\n\r", ch );
	return;
      }
    
}


void ship_to_starsystem( SHIP_DATA *ship , SPACE_DATA *starsystem )
{
     if ( starsystem == NULL )
        return;
     
     if ( ship == NULL )
        return;
     
     LINK ( ship, starsystem->first_ship , starsystem->last_ship , next_in_starsystem , prev_in_starsystem );
     
     ship->starsystem = starsystem;
        
}

void new_missile( SHIP_DATA *ship , SHIP_DATA *target , CHAR_DATA *ch  )
{
     SPACE_DATA *starsystem;
     MISSILE_DATA *missile;

     if ( ship  == NULL )
        return;

     if ( target  == NULL )
        return;

     if ( ( starsystem = ship->starsystem ) == NULL )
        return;
          
     CREATE( missile, MISSILE_DATA, 1 );
     LINK( missile, first_missile, last_missile, next, prev );
     
     missile->target = target; 
     missile->fired_from = ship;
     if ( ch )
        missile->fired_by = STRALLOC( ch->name );
     else 
        missile->fired_by = STRALLOC( "" );
     missile->age =0;
     missile->speed = 200;
     
     missile->mx = ship->vx;
     missile->my = ship->vy;
     missile->mz = ship->vz;
            
     if ( starsystem->first_missile == NULL )
        starsystem->first_missile = missile;
     
     if ( starsystem->last_missile )
     {
         starsystem->last_missile->next_in_starsystem = missile;
         missile->prev_in_starsystem = starsystem->last_missile;
     }
     
     starsystem->last_missile = missile;
     
     missile->starsystem = starsystem;
        
}

void ship_from_starsystem( SHIP_DATA *ship , SPACE_DATA *starsystem )
{

     if ( starsystem == NULL )
        return;
     
     if ( ship == NULL )
        return;

     UNLINK ( ship, starsystem->first_ship , starsystem->last_ship , next_in_starsystem , prev_in_starsystem );
     
     ship->starsystem = NULL;

}

void extract_missile( MISSILE_DATA *missile )
{
    SPACE_DATA *starsystem;

     if ( missile == NULL )
        return;

     if ( ( starsystem = missile->starsystem ) != NULL )
     {
     
      if ( starsystem->last_missile == missile )
        starsystem->last_missile = missile->prev_in_starsystem;
        
      if ( starsystem->first_missile == missile )
        starsystem->first_missile = missile->next_in_starsystem;
        
      if ( missile->prev_in_starsystem )
        missile->prev_in_starsystem->next_in_starsystem = missile->next_in_starsystem;
     
      if ( missile->next_in_starsystem)
        missile->next_in_starsystem->prev_in_starsystem = missile->prev_in_starsystem;
        
      missile->starsystem = NULL;
      missile->next_in_starsystem = NULL;
      missile->prev_in_starsystem = NULL;   

     }
     
     UNLINK( missile, first_missile, last_missile, next, prev );
     
     missile->target = NULL; 
     missile->fired_from = NULL;
     if (  missile->fired_by )
        STRFREE( missile->fired_by );
     
     DISPOSE( missile );
          
}

bool check_ship_pilot( CHAR_DATA *ch , SHIP_DATA *ship )
{        
   /* Check if the character is the pilot, or copilot of the ship. */
   if ( !str_cmp( ch->name, ship->pilot ) ||
        !str_cmp( ch->name, ship->copilot ) )
      return TRUE;

   /* Check if the characer is the owner of the ship. */
   if ( check_ship_owner( ch, ship ) )
      return TRUE;

   /**
    * Check if the character is bestowed with pilot and the ship is owned
    * by the clan the character is in.
    */
   if ( !IS_NPC( ch ) &&
        ch->pcdata && ch->pcdata->clan &&
        !str_cmp( ch->pcdata->clan->name, ship->owner ) &&
        is_empowered(ch, POWER_PILOT))
      return TRUE;
    
   /* If it gets here then the character isn't a pilot. */
   return FALSE;
}

/**
 * Checks if a character is the owner of a ship or the leader of a clan that owns the ship.
 *
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch      The character to check if its the owner.
 * @param       ship    The ship to check if the character owns.
 *
 * @returns     A string with all the leaders.
 */
bool check_ship_owner( CHAR_DATA * ch, SHIP_DATA * ship )
{
   /* Check if the character is the owner, pilot, or copilot of the ship. */
   if ( !str_cmp( ch->name, ship->owner ) )
      return TRUE;

   /* Check if the character is the leader of the clan that owns the ship. */
   if ( !IS_NPC( ch ) &&
        ch->pcdata && ch->pcdata->clan &&
        !str_cmp( ch->pcdata->clan->name, ship->owner ) &&
        is_leader(ch) )
      return TRUE;

   /* If it got here then the character is not a owner of the ship. */
   return FALSE;
}

bool extract_ship( SHIP_DATA *ship )
{   
    ROOM_INDEX_DATA *room;
    
    if ( ( room = ship->in_room ) != NULL )
    {               
        UNLINK( ship, room->first_ship, room->last_ship, next_in_room, prev_in_room );
        ship->in_room = NULL;
    }
    return TRUE;
}

void damage_ship_ch( SHIP_DATA *ship , int min , int max , CHAR_DATA *ch )
{
    int damage , shield_dmg;

    damage = number_range( min , max );

    if ( ship->shield > 0 )
    {
        shield_dmg = UMIN( ship->shield , damage );
    	damage -= shield_dmg;
    	ship->shield -= shield_dmg;
    	if ( ship->shield == 0 )
    	  echo_to_cockpit( AT_BLOOD , ship , "Shields down..." );
    }

    if ( damage > 0 )
    {
        if ( number_range(1, 100) <= 5 && ship->shipstate != SHIP_DISABLED )
        {
           echo_to_cockpit( AT_BLOOD + AT_BLINK , ship , "Ships Drive DAMAGED!" );
           ship->shipstate = SHIP_DISABLED;
        }

        if ( number_range(1, 100) <= 5 && ship->missilestate != MISSILE_DAMAGED && ship->maxmissiles > 0 )
        {
           echo_to_room( AT_BLOOD + AT_BLINK , ship->gunseat , "Ships Missile Launcher DAMAGED!" );
           ship->missilestate = MISSILE_DAMAGED;
        }

        if ( number_range(1, 100) <= 2 && ship->laserstate != LASER_DAMAGED )
        {
           echo_to_room( AT_BLOOD + AT_BLINK , ship->gunseat , "Lasers DAMAGED!" );
           ship->laserstate = LASER_DAMAGED;
        }

    }

    ship->hull -= damage*5;

    if ( ship->hull <= 0 )
    {
       destroy_ship( ship , ch );
       return;
    }

    if ( ship->hull <= ship->maxhull/20 )
       echo_to_cockpit( AT_BLOOD+ AT_BLINK , ship , "WARNING! Ship hull severely damaged!" );

}

void damage_ship( SHIP_DATA *ship , int min , int max )
{
    int damage , shield_dmg;

    damage = number_range( min , max );

    if ( ship->shield > 0 )
    {
        shield_dmg = UMIN( ship->shield , damage );
    	damage -= shield_dmg;
    	ship->shield -= shield_dmg;
    	if ( ship->shield == 0 )
    	  echo_to_cockpit( AT_BLOOD , ship , "Shields down..." );
    }

    if ( damage > 0 )
    {

        if ( number_range(1, 100) <= 5 && ship->shipstate != SHIP_DISABLED )
        {
           echo_to_cockpit( AT_BLOOD + AT_BLINK , ship , "Ships Drive DAMAGED!" );
           ship->shipstate = SHIP_DISABLED;
        }

        if ( number_range(1, 100) <= 5 && ship->missilestate != MISSILE_DAMAGED && ship->maxmissiles > 0 )
        {
           echo_to_room( AT_BLOOD + AT_BLINK , ship->gunseat , "Ships Missile Launcher DAMAGED!" );
           ship->missilestate = MISSILE_DAMAGED;
        }


    }

    ship->hull -= damage*5;

    if ( ship->hull <= 0 )
    {
       destroy_ship( ship , NULL );
       return;
    }

    if ( ship->hull <= ship->maxhull/20 )
       echo_to_cockpit( AT_BLOOD+ AT_BLINK , ship , "WARNING! Ship hull severely damaged!" );

}

void destroy_ship( SHIP_DATA *ship , CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *room;
    OBJ_DATA *robj;
    CHAR_DATA *rch;
    bool survived;
    int dIndex;
    SHIP_DATA *att;
    MISSILE_DATA * missile;
    MISSILE_DATA * m_next;
    TURRET_DATA * turret;

    sprintf( buf , "%s explodes in a blinding flash of light!", ship->name );
    echo_to_system( AT_WHITE + AT_BLINK , ship , buf , NULL );

    sprintf( buf , "%s destroyed by %s", ship->name , ch ? ch->name : "(none)" );
    log_string( buf );

    echo_to_ship( AT_WHITE , ship , "The ship is shaken by a FATAL explosion. You realize its escape or perish.");
    echo_to_ship( AT_WHITE , ship , "The last thing you remember is reaching for the escape pod release lever.");
    echo_to_ship( AT_WHITE + AT_BLINK , ship , "A blinding flash of light.");
    echo_to_ship( AT_WHITE, ship , "And then darkness....");

    /* If a character killed the ship and the ship has a bounty then award the character the bounty. */
    if ((ch != NULL) && !IS_NPC(ch) && (ship->bounty > 0))
    {
        ch->pcdata->bank += ship->bounty;
        ch_printf(ch, "&GYou receive %ld credits for destroying %s.\n\r", ship->bounty, ship->name);        
    }

	for ( room = ship->first_room ; room ; room = room->next_in_ship )
     {
         if(!ship->first_room) {
            sprintf(buf, "%s has no first room.", ship->name);
            log_string(buf);
            return;
         }

         rch = room->first_person;   
         while ( rch )
         {
            survived = FALSE;

            // This picks a planet in the system at random to escape to, that isn't hidden
			if ( !IS_NPC( rch ) && ship->starsystem )
			{
				PLANET_DATA * planet;
				ROOM_INDEX_DATA * pRoom;
				OBJ_DATA * scraps;          
				int chance;

				for ( planet = ship->starsystem->first_planet; planet; planet = planet->next_in_system )
				{
					if ( IS_SET( planet->flags, PLANET_HIDDEN ) || !planet->area )
						continue;

					// TODO: Make the chance based on the number of visible planets in the
					//       system.					
					// 50% chance of landing on this planet.
					if ( number_percent() <= 50 )
						break;                         
				}

				if ( planet )
				{					
					for ( pRoom = planet->area->first_room; pRoom; pRoom = pRoom->next_in_area )
					{
						if ( pRoom->sector_type != SECT_UNDERGROUND && pRoom->sector_type != SECT_DUNNO
							 && pRoom->sector_type != SECT_INSIDE )
							continue;

						// number_percent returns an integer, so, if the chance is less than one
						// it will truncate it to 0, so just make it one :'( So if the planet is
						// larger than 100 you have a inflated chance of landing in rooms :-/
						chance = UMAX( 1, planet->size / 100 );
						if ( number_percent() <= chance )
						{
							char_from_room(rch);
							char_to_room( rch, pRoom );
							if ( !IS_IMMORTAL( rch ) )
								rch->hit = -1;
							update_pos( rch );
							echo_to_room( AT_WHITE , rch->in_room , "There is loud explosion as an escape pod hits the earth." );
						    
							scraps = create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 );
							if(scraps != NULL)
							{
								scraps->timer = 15;
								STRFREE( scraps->short_descr );
								scraps->short_descr = STRALLOC( "a battered escape pod" );
								STRFREE( scraps->description );
								scraps->description = STRALLOC( "The smoking shell of an escape pod litters the earth.\n\r" );
								obj_to_room( scraps, pRoom);
							}

							survived = TRUE;                               
							break;
						}
					}
				}
			}
/*         
            if ( !IS_NPC( rch ) && ship->starsystem
            && ship->starsystem->last_planet && ship->starsystem->last_planet->area )
            {
                ROOM_INDEX_DATA * pRoom;
                OBJ_DATA * scraps;
                int rnum = 0;
                int tnum;
                
                tnum = number_range( 0 , ship->starsystem->last_planet->wilderness + ship->starsystem->last_planet->farmland );
                
                for ( pRoom = ship->starsystem->last_planet->area->first_room ; pRoom ; pRoom = pRoom->next_in_area )
                    if ( pRoom->sector_type != SECT_CITY && pRoom->sector_type != SECT_DUNNO
                    && pRoom->sector_type != SECT_INSIDE && pRoom->sector_type != SECT_UNDERGROUND )
                    {
                       if ( rnum++ < tnum )
                          continue;
                       
                       char_from_room(rch);
                       char_to_room( rch, pRoom );
                       if ( !IS_IMMORTAL( rch ) )
                          rch->hit = -1;
                       update_pos( rch );
                       echo_to_room( AT_WHITE , rch->in_room , "There is loud explosion as an escape pod hits the earth." );
                       
                       scraps        = create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 );
                       if(scraps != NULL) {
	                       scraps->timer = 15;
        	               STRFREE( scraps->short_descr );
                	       scraps->short_descr = STRALLOC( "a battered escape pod" );
	                       STRFREE( scraps->description );
        	               scraps->description = STRALLOC( "The smoking shell of an escape pod litters the earth.\n\r" );
                	       obj_to_room( scraps, pRoom);
		       }
                       survived = TRUE;     
                       break;   
                    }       
            }
*/
            
            if ( !survived && IS_IMMORTAL(rch) )
            {
                 char_from_room(rch);
                 char_to_room( rch, get_room_index(wherehome(rch)) );
                 survived = TRUE;
            }

            if ( !survived )
            {
              if ( ch )
                  raw_kill( ch , rch );
               else
                  raw_kill( rch , rch );
            }
            rch = room->first_person;     
         }
        
         for ( robj = room->first_content ; robj ; robj = robj->next_content )
         {
           separate_obj( robj );
           extract_obj( robj );
         }
     }

     
     if (ship->starsystem)
        ship_from_starsystem( ship, ship->starsystem );  
  
     extract_ship( ship );
       
     if ( ship->type != MOB_SHIP )
     {
        CLAN_DATA *clan;
        
        if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
        {
          if ( ship->class <= SPACE_STATION )
             clan->spacecraft--;
          else
             clan->vehicles--;
        }
     }        
     STRFREE( ship->owner );
     STRFREE( ship->pilot );
     STRFREE( ship->copilot );
     STRFREE( ship->home );
     ship->owner = STRALLOC( "" );
     ship->pilot = STRALLOC( "" );
     ship->copilot = STRALLOC( "" );
     ship->home = STRALLOC( "" );     
     ship->target = NULL;
     for ( turret = ship->first_turret ; turret ; turret = turret->next )
                turret->target = NULL;
     for ( dIndex = 0 ; dIndex < MAX_SHIP_ROOMS ; dIndex++ )
       if ( ship->description[dIndex] );
          STRFREE( ship->description[dIndex] );

     UNLINK ( ship, first_ship , last_ship , next , prev );

     for ( att = first_ship ; att ; att = att->next )
     {
         if ( att->target == ship )
            att->target = NULL;
         for ( turret = att->first_turret ; turret ; turret = turret->next )
            if ( turret->target == ship )
                turret->target = NULL;
           
     }
     
     for ( missile = first_missile; missile; missile = m_next )
     {
         m_next = missile->next;
         
         if ( missile->target && missile->target == ship )
            missile->target = NULL;
         if ( missile->fired_from && missile->fired_from == ship )
            missile->target = NULL;            
     }
     
     write_ship_list();
}

bool ship_to_room(SHIP_DATA *ship , long vnum )
{
    ROOM_INDEX_DATA *shipto;
    
    if ( (shipto=get_room_index(vnum)) == NULL )
            return FALSE;
            
    LINK( ship, shipto->first_ship, shipto->last_ship, next_in_room, prev_in_room );
    ship->in_room = shipto; 
    return TRUE;
}

bool ship_to_hangar(SHIP_DATA *ship, ROOM_INDEX_DATA *hangar)
{
    if( !hangar || !ship )
	return FALSE;

    

    LINK( ship, hangar->first_ship, hangar->last_ship, next_in_room, prev_in_room );
    ship->in_room = hangar;
    return TRUE;
}

void do_board( CHAR_DATA *ch, char *argument )
{
   ROOM_INDEX_DATA *fromroom;
   ROOM_INDEX_DATA *toroom;
   SHIP_DATA *ship;
   
   if ( !argument || argument[0] == '\0')
   {
       send_to_char( "Board what?\n\r", ch );
       return;
   }
   
   if ( ( ship = ship_in_room( ch->in_room , argument ) ) == NULL )
   {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
           return;
   }

   fromroom = ch->in_room;

        if ( ( toroom = ship->entrance )  != NULL )
   	{
   	   if ( ! ship->hatchopen )
   	   {
   	      send_to_char( "&RThe hatch is closed!\n\r", ch);
   	      return;
   	   }
   	
           if ( toroom->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;
	        
	       for ( ctmp = toroom->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( ++count >= toroom->tunnel )
	       {
                  send_to_char( "There is no room for you in there.\n\r", ch );
		  return;
	       }
           }
            if ( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
            {
                 send_to_char("&rThat ship has already started launching!\n\r",ch);
                 return;
            }
            
            act( AT_PLAIN, "$n enters $T.", ch,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You enter $T.", ch,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , toroom );
   	    act( AT_PLAIN, "$n enters the ship.", ch,
		NULL, argument , TO_ROOM );
            do_look( ch , "auto" );

        }                                                                  
        else
          send_to_char("That ship has no entrance!\n\r", ch);
}

void do_leaveship( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *fromroom;
    ROOM_INDEX_DATA *toroom;
    SHIP_DATA *ship;
    
    fromroom = ch->in_room;
    
    if  ( (ship = ship_from_entrance(fromroom)) == NULL )
    {
        send_to_char( "I see no exit here.\n\r" , ch );
        return;
    }   
    
    if  ( ship->class == SPACE_STATION )
    {
        send_to_char( "You can't do that here.\n\r" , ch );
        return;
    }   
    
    if ( ship->lastdoc != ship->location )
    {
        send_to_char("&rMaybe you should wait until the ship lands.\n\r",ch);
        return;
    }
    
    if ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
    {
        send_to_char("&rPlease wait till the ship is properly docked.\n\r",ch);
        return;
    }
    
    if ( ! ship->hatchopen )
    {
    	send_to_char("&RYou need to open the hatch first" , ch );
    	return;
    }
    
    if ( ( toroom = ship->in_room ) != NULL )
    {
            act( AT_PLAIN, "$n exits the ship.", ch,
		NULL, argument , TO_ROOM );
	    act( AT_PLAIN, "You exit the ship.", ch,
		NULL, argument , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , toroom );
   	    act( AT_PLAIN, "$n steps out of a ship.", ch,
		NULL, argument , TO_ROOM );
            do_look( ch , "auto" );
     }       
     else
        send_to_char ( "The exit doesn't seem to be working properly.\n\r", ch );  
}

void do_launch ( CHAR_DATA *ch, char *argument )
{
    int chance; 
    long price = 0;
    SHIP_DATA *ship;
    char buf[MAX_STRING_LENGTH];
            
    	        if ( (ship = ship_from_pilotseat(ch->in_room)) == NULL )  
    	        {
    	            send_to_char("&RYou must be in the pilots seat of a ship to do that!\n\r",ch);
    	            return;
    	        }
    	        
    	        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
    	        if ( autofly(ship) )
    	        {
    	            send_to_char("&RThe ship is set on autopilot, you'll have to turn it off first.\n\r",ch);
    	            return;
    	        }
    	        
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "You can't do that here.\n\r" , ch );
                   return;
                }   
    
    	        if ( !check_ship_pilot( ch , ship ) )
    	        {
    	            send_to_char("&RHey, thats not your ship! Try renting a public one.\n\r",ch);
    	            return;
    	        }
    	        
    	        if ( ship->lastdoc != ship->location )
                {
                     send_to_char("&rYou don't seem to be docked right now.\n\r",ch);
                     return;
                }
    
    	        if ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
    	        {
    	            send_to_char("The ship is not docked right now.\n\r",ch);
    	            return;
    	        }
                
                chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
                if ( number_percent( ) < chance )
    		{  
                       price=20;
                      
                     price += ( ship->maxhull-ship->hull );
                     if (ship->missiles )
     	                 price += ( 50 * (ship->maxmissiles-ship->missiles) );
                     
                     if (ship->shipstate == SHIP_DISABLED )
                            price += 200;
                     if ( ship->missilestate == MISSILE_DAMAGED )
                            price += 100;
                     if ( ship->laserstate == LASER_DAMAGED )
                            price += 50;
                
    	          if ( ch->pcdata && ch->pcdata->clan && !str_cmp(ch->pcdata->clan->name,ship->owner) ) 
                  {
                   if ( ch->pcdata->clan->funds < price )
                   {
                       ch_printf(ch, "&R%s doesn't have enough funds to prepare this ship for launch.\n\r", ch->pcdata->clan->name );
                       return;
                   }
    
                   ch->pcdata->clan->funds -= price;
                   ch_printf(ch, "&GIt costs %s %ld credits to ready this ship for launch.\n\r", ch->pcdata->clan->name, price );   
                  }
                  else
                  {
                   if ( ch->gold < price )
                   {
                       ch_printf(ch, "&RYou don't have enough funds to prepare this ship for launch.\n\r");
                       return;
                   }
    
                   ch->gold -= price;
                   ch_printf(ch, "&GYou pay %ld credits to ready the ship for launch.\n\r", price );   
                
                  }
                
                  ship->energy = ship->maxenergy;
                  ship->chaff = ship->maxchaff;
                  ship->missiles = ship->maxmissiles;
       		  ship->shield = 0;
       		  ship->autorecharge = FALSE;
       		  ship->autotrack = FALSE;
       		  ship->autospeed = FALSE;
       		  ship->hull = ship->maxhull;
       
       		  ship->missilestate = MISSILE_READY;
       		  ship->laserstate = LASER_READY;
       		  ship->shipstate = SHIP_DOCKED;
                
    		   if (ship->hatchopen)
    		   {
    		     ship->hatchopen = FALSE;
    		     sprintf( buf , "The hatch on %s closes." , ship->name);  
       	             echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf );
       	             echo_to_room( AT_YELLOW , ship->entrance , "The hatch slides shut." );
       	             sound_to_room( ship->entrance , "!!SOUND(door)" );
      		     sound_to_room( get_room_index(ship->location) , "!!SOUND(door)" );
       	           }
    		   set_char_color( AT_GREEN, ch );
    		   send_to_char( "Launch sequence initiated.\n\r", ch);
    		   act( AT_PLAIN, "$n starts up the ship and begins the launch sequence.", ch,
		        NULL, argument , TO_ROOM );
		   echo_to_ship( AT_YELLOW , ship , "The ship hums as it lifts off the ground.");
    		   sprintf( buf, "%s begins to launch.", ship->name );
    		   echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf );
    		   ship->shipstate = SHIP_LAUNCH;
    		   ship->currspeed = ship->realspeed;
                   learn_from_success( ch, gsn_spacecraft );
                   return;   	   	
                }
                set_char_color( AT_RED, ch );
	        send_to_char("You fail to work the controls properly!\n\r",ch);
    	   	return;	
    	
}

void launchship( SHIP_DATA *ship )
{   
    char buf[MAX_STRING_LENGTH];
    SHIP_DATA *source;
    int plusminus;
    ROOM_INDEX_DATA * room;
    
    ship_to_starsystem( ship, starsystem_from_room( ship->in_room ) );
        
    if ( ship->starsystem == NULL )
    {
       echo_to_room( AT_YELLOW , ship->pilotseat , "Launch path blocked .. Launch aborted.");
       echo_to_ship( AT_YELLOW , ship , "The ship slowly sets back back down on the landing pad.");
       sprintf( buf ,  "%s slowly sets back down." ,ship->name );
       echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf ); 
       ship->shipstate = SHIP_DOCKED;
       return;
    }    
    
    source = ship_from_room( ship->in_room );
    extract_ship(ship);    
    
    ship->location = 0;
    
    if (ship->shipstate != SHIP_DISABLED)
       ship->shipstate = SHIP_READY;
    
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hx = 1;
    else
        ship->hx = -1;
    
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hy = 1;
    else
        ship->hy = -1;
        
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hz = 1;
    else
        ship->hz = -1;
    
    if ( ( room = get_room_index(ship->lastdoc) ) != NULL && 
       room->area && room->area->planet && room->area->planet->starsystem 
       && room->area->planet->starsystem == ship->starsystem )
    {
           ship->vx = room->area->planet->x;
           ship->vy = room->area->planet->y;
           ship->vz = room->area->planet->z;
    }
    else if ( source )
    {
             ship->vx = source->vx;
             ship->vy = source->vy;
             ship->vz = source->vz;
    }
    
    ship->energy -= (100+10*ship->model);
         
    ship->vx += (ship->hx*ship->currspeed*2);
    ship->vy += (ship->hy*ship->currspeed*2);
    ship->vz += (ship->hz*ship->currspeed*2);
    
    echo_to_room( AT_GREEN , get_room_index(ship->location) , "Launch complete.\n\r");	
    echo_to_ship( AT_YELLOW , ship , "The ship leaves the platform far behind as it flies into space." );
    sprintf( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
    echo_to_system( AT_YELLOW, ship, buf , NULL ); 
    sprintf( buf, "%s lifts off into space.", ship->name );
    echo_to_room( AT_YELLOW , get_room_index(ship->lastdoc) , buf );
                 
}


void do_land( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    PLANET_DATA * planet;
    bool pfound = FALSE;
    ROOM_INDEX_DATA * room;
    bool rfound = FALSE;

    	        strcpy( arg , argument );        
                argument = one_argument( argument , arg1 );
                
    	        if ( (ship = ship_from_pilotseat(ch->in_room)) == NULL )
    	        {
    	            send_to_char("&RYou must be in the pilots seat of a ship to do that!\n\r",ch);
    	            return;
    	        }
    	        
    	        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }

   if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
   {
      ch_printf( ch, "&RYou would have to decloak first.\r\n" );
      return;
   }
    	        
    	        if ( autofly(ship) )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }
    	        
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RYou can't land space stations.\n\r" , ch );
                   return;
                }   
    
    	        if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to land.\n\r",ch);
    	            return;
    	        }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RThe ship is already docked!\n\r",ch);
    	            return;
    	        }
    	                
               if (ship->shipstate == SHIP_HYPERSPACE)
               {
                  send_to_char("&RYou can only do that in realspace!\n\r",ch);
                  return;   
               }

    	        if (ship->shipstate != SHIP_READY)
    	        {
    	            send_to_char("&RPlease wait until the ship has finished its current manouver.\n\r",ch);
    	            return;
    	        }
    	        if ( ship->starsystem == NULL )
    	        {
    	            send_to_char("&RThere's nowhere to land around here!",ch);
    	            return;
    	        }
    	        
    	        if ( ship->energy < (25 + 5*ship->model) )
    	        {
    	           send_to_char("&RTheres not enough fuel!\n\r",ch);
    	           return;
    	        }
    	        
    	        if ( arg[0] == '\0' )
    	        {  
    	           set_char_color(  AT_CYAN, ch );
    	           ch_printf(ch, "%s" , "Land where?\n\r\n\rChoices: \n\r");   	           
    	           
    	           for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
                   {       
                        if ( ( target->class == SPACE_STATION || target->first_hangar ) && target != ship && !IS_SET( target->flags, SHIP_CLOAKED) ) 
                           ch_printf(ch, "%s    %.0f %.0f %.0f\n\r         ", 
                           	target->name,
                           	target->vx,
                           	target->vy,
                           	target->vz);
                   }  
                   for ( planet = ship->starsystem->first_planet ; planet ; planet = planet->next_in_system ) 
			if(!IS_SET(planet->flags, PLANET_HIDDEN) || IS_IMMORTAL(ch)) {
        	               ch_printf( ch, "%s   %d %d %d\n\r",
    				planet->name, 
    				planet->x , planet->y, planet->z);
			}
                   ch_printf(ch, "\n\rYour Coordinates: %.0f %.0f %.0f\n\r" , 
                             ship->vx , ship->vy, ship->vz);   
                   return;
    	        }
    	        
    	        for ( planet = ship->starsystem->first_planet ; planet ; planet = planet->next_in_system )
                {
                    if ( !str_prefix( arg1 , planet->name) )
                    {
                       pfound = TRUE;
			
                       if ( IS_SET( planet->flags, PLANET_HIDDEN ) && !IS_IMMORTAL( ch ) )
                       {
                          do_land( ch, "" );
			  return;
                       }
                       
                       if ( !planet->area )
                       { 
    	                 send_to_char("&RThat planet doesn't have any landing areas.\n\r",ch);
    	                 return;
    	               } 

					   if ( find_range( ship->vx, ship->vy, ship->vz, planet->x, planet->y, planet->z ) > 200 )
    	               {
    	                  send_to_char("&RThat planet is too far away! You'll have to fly a little closer.\n\r",ch);
    	                  return;
    	               }       
    	               
    	               if ( argument[0] != '\0' )     
                         for ( room = planet->area->first_room ; room ; room = room->next_in_area )
                         {
                           if ( IS_SET( room->room_flags , ROOM_CAN_LAND ) && !str_prefix( argument , room->name) )
                           {
                              rfound = TRUE;
                              break;
                           }
                         }
                       
                       if ( !rfound )
                       {
      	                   send_to_char("&CPlease type the location after the planet name.\n\r",ch);
      	                   ch_printf( ch , "Possible choices for %s:\n\r\n\r", planet->name);
                           for ( room = planet->area->first_room ; room ; room = room->next_in_area )
                              if ( IS_SET( room->room_flags, ROOM_CAN_LAND ) ) 
                                 ch_printf( ch , "%s\n\r", room->name);
                           return;
                       }
                       
                       break;
                    }
                }
    	        
                if ( !pfound )
                {
    	           target = get_ship_here( arg , ship->starsystem );
                    
    	           if ( target != NULL && !IS_SET( target->flags, SHIP_CLOAKED ) )
    	           {
    	              if ( target == ship )
    	              { 
    	                 send_to_char("&RYou can't land your ship inside itself!\n\r",ch);
    	                 return;
    	              } 
    	              if ( !target->first_hangar )
    	              {
    	                send_to_char("&RThat ship has no hangar for you to land in!\n\r",ch);
    	                return;
    	              }
		      if ( ship->towing )
		      {
			send_to_char("&RYou cannot dock there while hauling an asteroid!\n\r",ch);
			return;
		      }
			          if ( find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 200 )
    	              {
    	                send_to_char("&R That ship is too far away! You'll have to fly a little closer.\n\r",ch);
    	                return;
    	              }       
		      if ( ( model[target->model].rooms - 5 ) < model[ship->model].rooms )
		      {
			send_to_char("&RYour ship is too big to land there!\n\r", ch );
			return;
		      }
    	           }
                   else
                   {
    	                send_to_char("&RI don't see that here.\n\r&W",ch);
    	                do_land( ch , "" );
    	                return;
    	           }
    	        }
    	           
                chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
                if ( number_percent( ) < chance )
    		{
    		   set_char_color( AT_GREEN, ch );
    		   send_to_char( "Landing sequence initiated.\n\r", ch);
    		   act( AT_PLAIN, "$n begins the landing sequence.", ch,
		        NULL, "" , TO_ROOM );
		   echo_to_ship( AT_YELLOW , ship , "The ship slowly begins its landing aproach.");
    		   ship->dest = STRALLOC(arg);
    		   ship->shipstate = SHIP_LAND;
    		   ship->currspeed = 0;
		   ship->target = NULL;
                   if ( number_percent() == 23 )
                   {
	                send_to_char( "Your experience makes you feel more coordinated than before.\n\r", ch );
                        ch->perm_dex++;
                        ch->perm_dex = UMIN( ch->perm_dex , 25 );
                   }
                   learn_from_success( ch, gsn_spacecraft );
		   if(ship->towing != NULL) {
			sell_asteroid(ship, ch);
		   }
                   return;
	        }
	        send_to_char("You fail to work the controls properly.\n\r",ch);
    	   	return;	
}

void landship( SHIP_DATA *ship, char *argument )
{    
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];
    int destination=0;
    ROOM_INDEX_DATA *shipland;
    char arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    PLANET_DATA * planet;
    ROOM_INDEX_DATA * room;
    bool shiplanding = FALSE;
    HANGAR_DATA *hangar;

    strcpy( arg , argument );        
    argument = one_argument( argument , arg1 );

    for ( planet = ship->starsystem->first_planet ; planet ; planet = planet->next_in_system )
    {
         if ( !str_prefix( arg1 , planet->name) )
         {        
               if ( !planet->area )
                     continue;
               for ( room = planet->area->first_room ; room ; room = room->next_in_area )
               {
                    if ( IS_SET( room->room_flags , ROOM_CAN_LAND ) && !str_prefix( argument , room->name) )
                    {
                          destination = room->vnum;
                          break;
                    }
               }
               break;        
         }
    }

    echo_to_room( AT_YELLOW , ship->pilotseat, "Landing sequence complete."); 
    echo_to_ship( AT_YELLOW , ship , "You feel a slight thud as the ship sets down on the ground."); 
    sprintf( buf ,"%s disapears from your scanner." , ship->name  );
        
    if ( !destination )
    {
        target = get_ship_here( arg , ship->starsystem );
        if ( target && target != ship && target->first_hangar )
	{
	   hangar = target->first_hangar;
	   shipland = hangar->room;
        }
	else
	  shipland = NULL;
	shiplanding=TRUE;
	if( !shipland )
	{
	  echo_to_room( AT_RED , ship->pilotseat, "No hangars are cleared for your landing. Landing aborted.");
	  echo_to_ship( AT_YELLOW, ship, "The ship pulls back up out of its landing sequence.");
	  if( ship->shipstate != SHIP_DISABLED )
	    ship->shipstate = SHIP_READY;
	  return;
        }
        if ( !ship_to_hangar( ship , shipland ) )
        {
            echo_to_room( AT_YELLOW , ship->pilotseat, "Could not complete aproach. Landing aborted.");
            echo_to_ship( AT_YELLOW , ship , "The ship pulls back up out of its landing sequence.");
            if (ship->shipstate != SHIP_DISABLED)
              ship->shipstate = SHIP_READY;
            return;
        }   
    }

    if (!shiplanding  &&  !ship_to_room( ship , destination ) ) 
    { 
       echo_to_room( AT_YELLOW , ship->pilotseat, "Could not complete approach. Landing aborted."); 
       echo_to_ship( AT_YELLOW , ship , "The ship pulls back up out of its landing sequence."); 
       if (ship->shipstate != SHIP_DISABLED) 
           ship->shipstate = SHIP_READY; 
       return; 
    } 
    echo_to_system( AT_YELLOW, ship, buf , NULL );
    
    ship->location = destination;
    ship->lastdoc = ship->location;
    if (ship->shipstate != SHIP_DISABLED)
       ship->shipstate = SHIP_DOCKED;
    ship_from_starsystem(ship, ship->starsystem);
    
    sprintf( buf, "%s lands on the platform.", ship->name );
    echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf );
    
    ship->energy = ship->energy - 25 - 5*ship->model;

    save_ship(ship);   
}

void do_accelerate( CHAR_DATA *ch, char *argument )
{
    int chance;
    int change;
    SHIP_DATA *ship;
    char buf[MAX_STRING_LENGTH];
    
    	        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
    	        {
    	            send_to_char("&RYou must be at the controls of a ship to do that!\n\r",ch);
    	            return;
    	        }
                
                if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
                
                if ( autofly(ship) )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }
    	        
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RPlatforms can't move!\n\r" , ch );
                   return;
                }   

                if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou can only do that in realspace!\n\r",ch);
                  return;   
                }
                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to accelerate.\n\r",ch);
    	            return;
    	        }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\n\r",ch);
    	            return;
    	        }
    	        if ( ship->energy < abs((atoi(argument)-abs(ship->currspeed))/10) )
    	        {
    	           send_to_char("&RTheres not enough fuel!\n\r",ch);
    	           return;
    	        }
    	        
                chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
                if ( number_percent( ) >= chance )
    		{
	           send_to_char("&RYou fail to work the controls properly.\n\r",ch);
    	   	   return;	
                }
                
    change = atoi(argument);
                      
    act( AT_PLAIN, "$n manipulates the ships controls.", ch,
    NULL, argument , TO_ROOM );
    
    if ( change > ship->currspeed )
    {
       send_to_char( "&GAccelerating\n\r", ch);
       echo_to_cockpit( AT_YELLOW , ship , "The ship begins to accelerate.");
       sprintf( buf, "%s begins to speed up." , ship->name );
       echo_to_system( AT_ORANGE , ship , buf , NULL );
    }
    
    if ( change < ship->currspeed )
    {
       send_to_char( "&GDecelerating\n\r", ch);
       echo_to_cockpit( AT_YELLOW , ship , "The ship begins to slow down.");
       sprintf( buf, "%s begins to slow down." , ship->name );
       echo_to_system( AT_ORANGE , ship , buf , NULL );
    }
    		     
    ship->energy -= abs((change-abs(ship->currspeed))/10);
    
    ship->currspeed = URANGE( 0 , change , ship->realspeed );         
         
    learn_from_success( ch, gsn_spacecraft );
    	
}

/**
 * A gameplay command that lets you change the course of a ship.
 *
 * @author  Unknown
 * @version 1.0         11/18/04
 *
 * @param   ch          The character who used the command.
 * @param   argument    The arguments the character used with the command.
 */
void do_trajectory( CHAR_DATA *ch, char *argument )
{
    char  buf[MAX_STRING_LENGTH];
    char  arg2[MAX_INPUT_LENGTH];
    char  arg3[MAX_INPUT_LENGTH];
    int chance;
    float vx,vy,vz;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    PLANET_DATA *planet;
    ASTEROID_DATA *asteroid;

    /* Check if there were no arguments. */
    if (argument[0] == '\0')
    {
        ch_printf(ch, "Change your course to what?");
        return;
    }
  
    	        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
    	        {
    	            send_to_char("&RYou must be at the helm of a ship to do that!\n\r",ch);
    	            return;
    	        }
                
                if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
                if ( autofly(ship))
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }
    	        
                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to manuever.\n\r",ch);
    	            return;
    	        }
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RPlatforms can't turn!\n\r" , ch );
                   return;
                }   

    	        if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou can only do that in realspace!\n\r",ch);
                  return;   
                }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\n\r",ch);
    	            return;
    	        }
    	        if (ship->shipstate != SHIP_READY)
    	        {
    	            send_to_char("&RPlease wait until the ship has finished its current manouver.\n\r",ch);
    	            return;
    	        }
    	        if ( ship->energy < (ship->currspeed/10) )
    	        {
    	           send_to_char("&RTheres not enough fuel!\n\r",ch);
    	           return;
    	        }
    	        
                chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
                if ( number_percent( ) > chance )
    		{ 
	        send_to_char("&RYou fail to work the controls properly.\n\r",ch);
    	   	return;	
    	        }    
    	
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

	if(arg3[0] == '\0')
        {
                /* Make sure it is a planet in the same system. */
		if (((planet = get_planet(arg2)) != NULL) &&
                    (planet->starsystem != NULL) &&
                    (ship->starsystem != NULL) &&
                    (ship->starsystem == planet->starsystem))
                {                    
			vx=planet->x;
			vy=planet->y;
			vz=planet->z;
		}
                else if((target = get_ship_here(arg2, ship->starsystem)))
                {
			vx=target->vx;
			vy=target->vy;
			vz=target->vz;
		}
                else if((asteroid = get_asteroid_here(arg2, ship->starsystem)))
                {
			vx=asteroid->x;
			vy=asteroid->y;
			vz=asteroid->z;
		}
                else
                {
                    /* Else there is no planet, ship, or asteroid with that name in the system. */
                    ch_printf(ch, "&RThat can not be found in this system.\r\n");
                    return;
                }
	}
        else
        {
		vx = atof( arg2 );
		vy = atof( arg3 );
		vz = atof( argument );

                /**
                 * This checks if someone entered in a non numeric string because when atof tries
                 * to parse a string it returns a 0, and if the original string wasn't a 0 then
                 * we know it was some other string that wasn't a number.  Keep in mind that str_cmp
                 * returns TRUE if the strings are not equal.
                 */
                if (((vx == 0) && (str_cmp(arg2, "0") == TRUE)) ||
                    ((vy == 0) && (str_cmp(arg3, "0") == TRUE)) ||
                    ((vz == 0) && (str_cmp(argument, "0") == TRUE)))
                {
                    ch_printf(ch, "&RThat doesn't appear to be in the system.\r\n");
                    return;
                }
               
	}
            
    if ( vx == ship->vx && vy == ship->vy && vz == ship->vz )
    {
       ch_printf( ch , "The ship is already at %.0f %.0f %.0f !" ,vx,vy,vz);       
    }
                
    ship->hx = vx - ship->vx;
    ship->hy = vy - ship->vy;
    ship->hz = vz - ship->vz;
    
    ship->energy -= (ship->currspeed/10);

    ch_printf( ch ,"&GNew course set, aproaching %.0f %.0f %.0f.\n\r" , vx,vy,vz );
    act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, argument , TO_ROOM );

    echo_to_cockpit( AT_YELLOW ,ship, "The ship begins to turn.\n\r" );                        
    sprintf( buf, "%s turns altering its present course." , ship->name );
    echo_to_system( AT_ORANGE , ship , buf , NULL );
                                                            
    if ( ship->manuever > 100 )
        ship->shipstate = SHIP_BUSY_3;
    else if ( ship->manuever > 50  )
        ship->shipstate = SHIP_BUSY_2;
    else
        ship->shipstate = SHIP_BUSY;     
   
    learn_from_success( ch, gsn_spacecraft );
    	
}


void do_buyship(CHAR_DATA *ch, char *argument )
{
    long         price;
    SHIP_DATA   *ship;
    SHIP_PROTOTYPE   *prototype;
    char         shipname[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
        
   if ( IS_NPC(ch) || !ch->pcdata )
   {
   	send_to_char( "&ROnly players can do that!\n\r" ,ch );
   	return;
   }

   if ( !ch->in_room || 
   ( !IS_SET(ch->in_room->room_flags, ROOM_SHIPYARD)  && !IS_SET(ch->in_room->room_flags, ROOM_GARAGE) ) )
   {
   	send_to_char( "&RYou must be in a shipyard or garage to purchase transportation.\n\r" ,ch );
   	return;
   }
 

   prototype = get_ship_prototype( argument );
   if ( prototype == NULL )
   {
   	send_to_char( "&RThat model does not exist.\n\r" ,ch );
        if ( IS_SET( ch->in_room->room_flags , ROOM_SHIPYARD ) )
           do_prototypes( ch, "ships" );
   	else
   	   do_prototypes( ch, "vehicles" );
   	return;
   }

   if ( IS_SET( ch->in_room->room_flags , ROOM_SHIPYARD ) && prototype->class > SPACE_STATION )
   {
   	send_to_char( "&RThats not a ship prototype.\n\r" ,ch );
        do_prototypes( ch, "ships" );
   	return;
   }
   if ( IS_SET( ch->in_room->room_flags , ROOM_GARAGE ) && prototype->class <= SPACE_STATION )
   {
   	send_to_char( "&RThats not a vehicle prototype.\n\r" ,ch );
        do_prototypes( ch, "vehicles" );
   	return;
   }


   price = get_prototype_value( prototype );
   
    if ( ch->gold < price )
    {
       ch_printf(ch, "&RThat type of ship costs %ld. You don't have enough credits!\n\r" , price );
       return;
    }
    
    ch->gold -= price;
    ch_printf(ch, "&GYou pay %ld credits to purchace the ship.\n\r" , price ); 

    /* If if it a large transaction then it should be logged for possible cheating. */ 
    if (price >= LARGE_TRANSACTION)
    {
        sprintf(buf, "TRANSACTION: %s purchased a ship for %ld credits.", ch->name, price);
        log_string(buf);
    }

    act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.",ch,
       NULL, argument , TO_ROOM );
    
    ship = make_ship( prototype );
    ship_to_room( ship, ch->in_room->vnum );
    ship->location = ch->in_room->vnum;
    ship->lastdoc = ch->in_room->vnum;
    ship->price=price; //phel
    
    sprintf( shipname , "%s's %s %s" , ch->name , prototype->name , ship->filename );

    STRFREE( ship->owner );
    ship->owner = STRALLOC( ch->name );
    STRFREE( ship->name );
    ship->name = STRALLOC( shipname );
    save_ship( ship );
    write_ship_list();
                 
}

void do_clanbuyship(CHAR_DATA *ch, char *argument )
{
    long         price;
    SHIP_DATA   *ship;
    CLAN_DATA   *clan;
    SHIP_PROTOTYPE   *prototype;
    char         shipname[MAX_STRING_LENGTH];
   
       
   if ( IS_NPC(ch) || !ch->pcdata )
   {
   	send_to_char( "&ROnly players can do that!\n\r" ,ch );
   	return;
   }
   if ( !ch->pcdata->clan )
   {
   	send_to_char( "&RYou aren't a member of any organizations!\n\r" ,ch );
   	return;
   }
   
   clan = ch->pcdata->clan;
   
   if (!is_empowered(ch, POWER_BUYSHIP))
   {
   	send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability.\n\r" ,ch );
   	return;
   }
	
   if ( !ch->in_room || 
   ( !IS_SET(ch->in_room->room_flags, ROOM_SHIPYARD)  && !IS_SET(ch->in_room->room_flags, ROOM_GARAGE) ) )
   {
   	send_to_char( "&RYou must be in a shipyard or garage to purchase transportation.\n\r" ,ch );
   	return;
   }
 

   prototype = get_ship_prototype( argument );
   if ( prototype == NULL )
   {
   	send_to_char( "&RThat model does not exist.\n\r" ,ch );
        if ( IS_SET( ch->in_room->room_flags , ROOM_SHIPYARD ) )
           do_prototypes( ch, "ships" );
   	else
   	   do_prototypes( ch, "vehicles" );
   	return;
   }

   if ( IS_SET( ch->in_room->room_flags , ROOM_SHIPYARD ) && prototype->class > SPACE_STATION )
   {
   	send_to_char( "&RThats not a ship prototype.\n\r" ,ch );
        do_prototypes( ch, "ships" );
   	return;
   }
   if ( IS_SET( ch->in_room->room_flags , ROOM_GARAGE ) && prototype->class <= SPACE_STATION )
   {
   	send_to_char( "&RThats not a vehicle prototype.\n\r" ,ch );
        do_prototypes( ch, "vehicles" );
   	return;
   }


   price = get_prototype_value( prototype );
   
    if ( clan->funds < price )
    {
       ch_printf(ch, "&RThat type of ship costs %ld. Your organization can't afford it!\n\r" , price );
       return;
    }
    
    clan->funds -= price;
    ch_printf(ch, "&GYou pay %ld credits from clan funds to purchace the ship.\n\r" , price );   

    act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.",ch,
       NULL, argument , TO_ROOM );
    
    ship = make_ship( prototype );
    ship_to_room( ship, ch->in_room->vnum );
    ship->location = ch->in_room->vnum;
    ship->lastdoc = ch->in_room->vnum;
    ship->price=price; //phel
    
    sprintf( shipname , "%s %s %s" , clan->name , prototype->name , ship->filename );

    STRFREE( ship->owner );
    ship->owner = STRALLOC( clan->name );
    STRFREE( ship->name );
    ship->name = STRALLOC( shipname );
    save_ship( ship );
    
   if ( ship->class <= SPACE_STATION )
             clan->spacecraft++;
   else
             clan->vehicles++;
}

void do_sellship(CHAR_DATA *ch, char *argument )
{
    long         price;
    SHIP_DATA   *ship;
    char buf[MAX_STRING_LENGTH];

   ship = ship_in_room( ch->in_room , argument );
   if ( !ship )
   {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
           return;
   }

   if ( str_cmp( ship->owner , ch->name) )
   {
   	send_to_char( "&RThat isn't your ship!" ,ch );
   	return;
   }

    price = ship->price;

    ch->gold += ( price - price/10 );
    ch_printf(ch, "&GYou receive %ld credits from selling your ship.\n\r" , price - price/10 );   

    /* If if it a large transaction then it should be logged for possible cheating. */ 
    if (price >= LARGE_TRANSACTION)
    {
        sprintf(buf, "TRANSACTION: %s sold a ship for %ld credits.", ch->name, price - (price / 10));
        log_string(buf);
    }

    act( AT_PLAIN, "$n walks over to a terminal and makes a credit transaction.",ch,
       NULL, argument , TO_ROOM );
 
	STRFREE( ship->owner );
	ship->owner = STRALLOC( "" );
	save_ship( ship );

}

void do_info(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    SHIP_DATA *target;
   
    if (  (ship = ship_from_cockpit(ch->in_room))  == NULL )
    {
            if ( argument[0] == '\0' )
            {
               act( AT_PLAIN, "Which ship do you want info on?.", ch, NULL, NULL, TO_CHAR );
               return;
            }
    
            ship = ship_in_room( ch->in_room , argument );
            if ( !ship )
            {
               act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
               return;
            }

            target = ship;
    }
    else if (argument[0] == '\0')
       target = ship;
    else
       target = get_ship_here( argument , ship->starsystem );
    	
    if ( target == NULL )
    {
         send_to_char("&RI don't see that here.\n\rTry the radar, or type info by itself for info on this ship.\n\r",ch);
         return;
    }          
    
    ch_printf( ch, "%s%s : %s\n\r",
		        target->type == PLAYER_SHIP ? "" : "MOB " ,
		        target->class == SPACECRAFT ? model[target->model].name :
		       (target->class == SPACE_STATION ? "Space Station" : 
		       (target->class == AIRCRAFT ? "Aircraft" : 
		       (target->class == BOAT ? "Boat" : 
		       (target->class == SUBMARINE ? "Submarine" : 
		       (target->class == LAND_VEHICLE ? "land vehicle" : "Unknown" ) ) ) ) ), 
    			target->name);
    if( check_ship_pilot( ch, target) || IS_IMMORTAL(ch)){
	    ch_printf( ch, "Owner: %s   Pilot: %s   Copilot: %s\n\r",
	    			target->owner, target->pilot,  target->copilot );
    } //-Zakul-

    ch_printf( ch, "Laser cannons: %d  ",
    			target->lasers);				
    ch_printf( ch, "Maximum Missiles: %d  ",
       			target->maxmissiles);		
    ch_printf( ch, "Max Chaff: %d\n\r",
       			target->maxchaff);		
    ch_printf( ch, "Max Hull: %d  ",
                        target->maxhull);
    ch_printf( ch, "Max Shields: %d   Max Energy(fuel): %d\n\r",
                        target->maxshield,
    		        target->maxenergy);
    ch_printf( ch, "Maximum Speed: %d   Hyperspeed: %d   Manuever: %d\n\r",
                        target->realspeed, target->hyperspeed, target->manuever );                    
        
    act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch,
         NULL, argument , TO_ROOM );
	  
}

void do_autorecharge(CHAR_DATA *ch, char *argument )
{
    int chance;
    SHIP_DATA *ship;
    int recharge;
    int energy_buffer;     
        
        if (  (ship = ship_from_gunseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be at the shield controls of a ship to do that!\n\r",ch);
            return;
        }

    /**
     * If you try to perform an action that will take you below less than 5% energy
     * it will fail to happen to protect you from running out of energy.
     */
    energy_buffer = ship->maxenergy * .05;

        if ( autofly(ship)  )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }

        if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
        {
           ch_printf( ch, "You might want to decloak first.\r\n" );
           return;
        }
    	        
                chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
        if ( number_percent( ) > chance )
        {
           send_to_char("&RYou fail to work the controls properly.\n\r",ch);
           return;	
        }
    
    act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );

        /* Get how much shields that can be recharged. */
    recharge = ship->maxshield - ship->shield;
    /* Either recharge the most that ship can charge per a tick, or recharge enough to have full shields. */
    recharge = UMIN(recharge , 20+ship->model*10);

    if ( !str_cmp(argument,"on" ) )
    {
        /* If the recharge amount will bring it below the energy buffer then say there is not enough energy. */
        if ((ship->energy - recharge) < energy_buffer)
        {
            send_to_char("&RTheres not enough energy!\n\r", ch);
            return;
        }

        ship->autorecharge=TRUE;
        send_to_char( "&GYou power up the shields.\n\r", ch);
        echo_to_cockpit( AT_YELLOW , ship , "Shields ON. Autorecharge ON.");
    }
    else if ( !str_cmp(argument,"off" ) )
    {
        ship->autorecharge=FALSE;
        send_to_char( "&GYou shutdown the shields.\n\r", ch);
        echo_to_cockpit( AT_YELLOW , ship , "Shields OFF. Shield strength set to 0. Autorecharge OFF.");
        ship->shield = 0;
    }
    else if ( !str_cmp(argument,"idle" ) )
    {
        ship->autorecharge=FALSE;
        send_to_char( "&GYou let the shields idle.\n\r", ch);
        echo_to_cockpit( AT_YELLOW , ship , "Autorecharge OFF. Shields IDLEING.");
    }
    else
    {   
        if (ship->autorecharge == TRUE)
        {
           ship->autorecharge=FALSE;
           send_to_char( "&GYou toggle the shields.\n\r", ch);
           echo_to_cockpit( AT_YELLOW , ship , "Autorecharge OFF. Shields IDLEING.");
        }
        else
        {
            /* If the recharge amount will bring it below the energy buffer then say there is not enough energy. */
            if ((ship->energy - recharge) < energy_buffer)
            {
                send_to_char("&RTheres not enough energy!\n\r", ch);
                return;
            }

           ship->autorecharge=TRUE;
           send_to_char( "&GYou toggle the shields.\n\r", ch);
           echo_to_cockpit( AT_YELLOW , ship , "Shields ON. Autorecharge ON");
        }   
    }
    
    if (ship->autorecharge)
    {
       ship->shield += recharge;
       ship->energy -= (recharge * 5);
    }

    learn_from_success( ch, gsn_spacecraft );
	
}

void do_autopilot(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
        
        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be at the pilot controls of a ship to do that!\n\r",ch);
            return;
        }
        
         if ( ! check_ship_pilot(ch,ship) )
       	     {
       	       send_to_char("&RHey! Thats not your ship!\n\r",ch);
       	       return;
       	     }

         if ( ship->target  )
       	     {
       	       send_to_char("&RNot while the ship is enganged with an enemy!\n\r",ch);
       	       return;
       	     }
  
        
    act( AT_PLAIN, "$n flips a switch on the control panell.", ch,
         NULL, argument , TO_ROOM );

        if (ship->autopilot == TRUE)
        {
           ship->autopilot=FALSE;
           send_to_char( "&GYou toggle the autopilot.\n\r", ch);
           echo_to_cockpit( AT_YELLOW , ship , "Autopilot OFF.");
        }
        else
        {
           ship->autopilot=TRUE;
           ship->autorecharge = TRUE;
           send_to_char( "&GYou toggle the autopilot.\n\r", ch);
           echo_to_cockpit( AT_YELLOW , ship , "Autopilot ON.");
        }   
    
}

void do_openhatch(CHAR_DATA *ch, char *argument )
{
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];
   
   if ( !argument || argument[0] == '\0' || !str_cmp(argument,"hatch") )
   {
       ship = ship_from_entrance( ch->in_room );
       if( ship == NULL)
       {
          send_to_char( "&ROpen what?\n\r", ch );
          return;
       }
       else
       {
          if ( !ship->hatchopen)
       	  {
       	     
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RTry one of the docking bays!\n\r" , ch );
                   return;
                }   
       	     if ( ship->location != ship->lastdoc ||
       	        ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED ) )
       	     {
       	       send_to_char("&RPlease wait till the ship lands!\n\r",ch);
       	       return;
       	     }
       	     ship->hatchopen = TRUE;
       	     send_to_char("&GYou open the hatch.\n\r",ch);
       	     act( AT_PLAIN, "$n opens the hatch.", ch, NULL, argument, TO_ROOM );
       	     sprintf( buf , "The hatch on %s opens." , ship->name);  
       	     echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf );
	     return;
       	  }
       	  else
       	  {
       	     send_to_char("&RIt's already open.\n\r",ch);
       	     return;	
       	  }
       }
   }
   
   ship = ship_in_room( ch->in_room , argument );
   if ( !ship )
   {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
           return;
   }

   if ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
        send_to_char( "&RThat ship has already started to launch",ch);
        return;
   }
   
   if ( ! check_ship_pilot(ch,ship) )
       	     {
       	       send_to_char("&RHey! Thats not your ship!\n\r",ch);
       	       return;
       	     }
       	     
   if ( !ship->hatchopen)
   {
   	ship->hatchopen = TRUE;
   	act( AT_PLAIN, "You open the hatch on $T.", ch, NULL, ship->name, TO_CHAR );
   	act( AT_PLAIN, "$n opens the hatch on $T.", ch, NULL, ship->name, TO_ROOM );
   	echo_to_room( AT_YELLOW , ship->entrance , "The hatch opens from the outside." );
	return;
   }

   send_to_char("&GIts already open!\n\r",ch);

}


void do_closehatch(CHAR_DATA *ch, char *argument )
{
   SHIP_DATA *ship;
   char buf[MAX_STRING_LENGTH];
   
   if ( !argument || argument[0] == '\0' || !str_cmp(argument,"hatch") )
   {
       ship = ship_from_entrance( ch->in_room );
       if( ship == NULL)
       {
          send_to_char( "&RClose what?\n\r", ch );
          return;
       }
       else
       {
          
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RTry one of the docking bays!\n\r" , ch );
                   return;
                }   
          if ( ship->hatchopen)
       	  {
       	     ship->hatchopen = FALSE;
       	     send_to_char("&GYou close the hatch.\n\r",ch);
       	     act( AT_PLAIN, "$n closes the hatch.", ch, NULL, argument, TO_ROOM );  
       	     sprintf( buf , "The hatch on %s closes." , ship->name);  
       	     echo_to_room( AT_YELLOW , get_room_index(ship->location) , buf );
 	     return;
       	  }
       	  else
       	  {
       	     send_to_char("&RIt's already closed.\n\r",ch);
       	     return;	
       	  }
       }
   }
   
   ship = ship_in_room( ch->in_room , argument );
   if ( !ship )
   {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
           return;
   }

   if ( ship->shipstate != SHIP_DOCKED && ship->shipstate != SHIP_DISABLED )
   {
        send_to_char( "&RThat ship has already started to launch",ch);
        return;
   }
   else
   {
      if(ship->hatchopen)
      {
   	ship->hatchopen = FALSE;
   	act( AT_PLAIN, "You close the hatch on $T.", ch, NULL, ship->name, TO_CHAR );
   	act( AT_PLAIN, "$n closes the hatch on $T.", ch, NULL, ship->name, TO_ROOM );
        echo_to_room( AT_YELLOW , ship->entrance , "The hatch is closed from outside.");

   	return;
      }
      else
      {
      	send_to_char("&RIts already closed.\n\r",ch);
      	return;
      }
   }


}

void do_status(CHAR_DATA *ch, char *argument )
{
    int chance, slot;
    SHIP_DATA *ship;
    SHIP_DATA *target;

	if( (ship = ship_from_cockpit(ch->in_room))  == NULL )
		if( (ship = ship_from_engine(ch->in_room))  == NULL )
			if( (ship = ship_from_turret(ch->in_room))  == NULL )
			{
				send_to_char("&RYou must be in the cockpit, turret or engineroom of a ship to do that!\n\r",ch);
				return;
			}

    if (argument[0] == '\0')
       target = ship;
    else
       target = get_ship_here( argument , ship->starsystem );
    	
    if ( target == NULL || ( IS_SET( target->flags, SHIP_CLOAKED ) && !IS_IMMORTAL( ch ) && target != ship ) )
    {
         send_to_char("&RI don't see that here.\n\rTry the radar, or type status by itself for your ships status.\n\r",ch);
         return;
    }          
    
    chance = IS_NPC(ch) ? 100
	                 : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou cant figure out what the readout means.\n\r",ch);
        return;	
    }
        
    act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch,
         NULL, argument , TO_ROOM );
    
    ch_printf( ch, "&W%s:\n\r",target->name);
    ch_printf( ch, "&OCurrent Coordinates:&Y %.0f %.0f %.0f\n\r",
                        target->vx, target->vy, target->vz );
    ch_printf( ch, "&OCurrent Heading:&Y %.0f %.0f %.0f\n\r",
                        target->hx, target->hy, target->hz );
    ch_printf( ch, "&OCurrent Speed:&Y %d&O/%d\n\r",
                        target->currspeed , target->realspeed );
    ch_printf( ch, "&OHull:&Y %d&O/%d  Ship Condition:&Y %s\n\r",
                        target->hull,
    		        target->maxhull,
    			target->shipstate == SHIP_DISABLED ? "Disabled" : "Running");
    ch_printf( ch, "&OShields:&Y %d&O/%d   Energy(fuel):&Y %d&O/%d\n\r",
                        target->shield,
    		        target->maxshield,
    		        target->energy,
    		        target->maxenergy);
    ch_printf( ch, "&OLaser Condition:&Y %s  &OCurrent Target:&Y %s\n\r",
    		        target->laserstate == LASER_DAMAGED ? "Damaged" : "Good" , target->target ? target->target->name : "none");
    ch_printf( ch, "\n\r&OMissiles:&Y %d&O/%d  Condition:&Y %s\n\r",
       			target->missiles,
    			target->maxmissiles,
    			target->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good" );		
    ch_printf( ch, "&OCloaking: &Y%s\r\n", IS_SET( ship->flags, SHIP_CLOAKED ) ? "On" : "Off" );

    if(ship->towing != NULL) {
	ch_printf(ch, "&OTowing: %s\n\r", target->towing->name);
    }

    for ( slot = 0; slot < MAX_MODS; slot++ )
    {       
       switch( ship->module[slot] )
       {
          default:
             continue;        
          case MODULE_LOCKDOWN:
             ch_printf( ch, "&OModule Slot %d: &Yan automatic lockdown module\r\n", slot + 1 );
             break;
          case MODULE_SPEED:
             ch_printf( ch, "&OModule Slot %d: &Ya speed module\r\n", slot + 1 );
             break;
          case MODULE_CLOAK:
             ch_printf( ch, "&OModule Slot %d: &Ya cloaking module\r\n", slot + 1 );
             break;
          case MODULE_FUELTANK:
             ch_printf( ch, "&OModule Slot %d: &Ya fueltank module\r\n", slot + 1 );
             break;
          case MODULE_TRACTORBEAM:
             ch_printf( ch, "&OModule Slot %d: &Ya tractorbeam  module\r\n", slot + 1);
             break;
          case MODULE_MANUEVER:
             ch_printf( ch, "&OModule Slot %d: &Ya manuever module\r\n", slot + 1 );
             break;
       }
    }
    	send_to_char("&w", ch );
	learn_from_success( ch, gsn_spacecraft );
}

void do_hyperspace(CHAR_DATA *ch, char *argument )
{
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *eShip;
    char buf[MAX_STRING_LENGTH];
   
        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be in control of a ship to do that!\n\r",ch);
            return;
        }
        
        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }

	if(ship->towing) {
		send_to_char("&RYou can't hyper while towing an asteroid!\n\r", ch);
		return;
	}

        if ( autofly(ship)  )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }
    	        
        
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RPlatforms can't move!\n\r" , ch );
                   return;
                }       
                if (ship->hyperspeed == 0)
                {
                  send_to_char("&RThis ship is not equipped with a hyperdrive!\n\r",ch);
                  return;   
                }
                if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou are already travelling lightspeed!\n\r",ch);
                  return;   
                }

				if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
				{
					ch_printf( ch, "&RYou can't do that while cloaked.\r\n" );
					return;
				}

                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to manuever.\n\r",ch);
    	            return;
    	        }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\n\r",ch);
    	            return;
    	        }
    	        if (ship->shipstate != SHIP_READY)
    	        {
    	            send_to_char("&RPlease wait until the ship has finished its current manouver.\n\r",ch);
    	            return;
    	        } 
                if (!ship->currjump)
    	        {
    	            send_to_char("&RYou need to calculate your jump first!\n\r",ch);
    	            return;
    	        } 

        if ( ship->energy < (200+ship->hyperdistance) )
        {
              send_to_char("&RTheres not enough fuel!\n\r",ch);
              return;
        }

        if ( ship->currspeed <= 0 )
        {
              send_to_char("&RYou need to speed up a little first!\n\r",ch);
              return;
        }
    	
    	for ( eShip = ship->starsystem->first_ship; eShip; eShip = eShip->next_in_starsystem )
    	{
    	   if ( eShip == ship )
    	      continue;
    	      
    	   if ( abs( eShip->vx - ship->vx ) < 500 
    	   &&  abs( eShip->vy - ship->vy ) < 500 
    	   &&  abs( eShip->vz - ship->vz ) < 500 )
           {
              ch_printf(ch, "&RYou are too close to %s to make the jump to lightspeed.\n\r", eShip->name );
              return;
           }    	   
    	}
    	        
        chance = IS_NPC(ch) ? 100
             : (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
        if ( number_percent( ) > chance )
        {
            send_to_char("&RYou can't figure out which lever to use.\n\r",ch);
    	   return;	
        }
    sprintf( buf ,"%s disapears from your scanner." , ship->name );
    echo_to_system( AT_YELLOW, ship, buf , NULL );

    ship_from_starsystem( ship , ship->starsystem );
    ship->shipstate = SHIP_HYPERSPACE;
        
    send_to_char( "&GYou push forward the hyperspeed lever.\n\r", ch);
    act( AT_PLAIN, "$n pushes a lever forward on the control panel.", ch,
         NULL, argument , TO_ROOM );
    echo_to_ship( AT_YELLOW , ship , "The ship lurches slightly as it makes the jump to lightspeed." );     
    echo_to_cockpit( AT_YELLOW , ship , "The stars become streaks of light as you enter hyperspace.");	  
    
    ship->energy -= (100+ship->hyperdistance);
    
    ship->vx = ship->jx;
    ship->vy = ship->jy;
    ship->vz = ship->jz;
    
    learn_from_success( ch, gsn_spacecraft );
    	
}

    
void do_target(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];
    TURRET_DATA *turret = NULL;
        
    strcpy( arg, argument );    
    
    switch( ch->substate )
    { 
    	default:
    	    if (  (ship = ship_from_turret(ch->in_room))  == NULL )
    	    {
    	        send_to_char("&RYou must be in the gunners seat or turret of a ship to do that!\n\r",ch);
    	        return;
    	    }
            
            if ( ship->class > SPACE_STATION )
    	    {
    	        send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	        return;
    	    }
    	    
            if (ship->shipstate == SHIP_HYPERSPACE)
            {
                send_to_char("&RYou can only do that in realspace!\n\r",ch);
                return;   
            }              
    	    if (! ship->starsystem )
    	    {
    	        send_to_char("&RYou can't do that until you've finished launching!\n\r",ch);
    	        return;
    	    }

            if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
            {
               ch_printf( ch, "&RYou can't do that while cloaked.\r\n" );
               return;
            }
            
            if ( autofly(ship) )
    	    {
    	        send_to_char("&RYou'll have to turn off the ships autopilot first....\n\r",ch);
    	        return;
    	    }
    	        
            if (arg[0] == '\0')
    	    {
    	        send_to_char("&RYou need to specify a target!\n\r",ch);
    	        return;
    	    }
            
            if ( !str_cmp( arg, "none") )
    	    {
    	        send_to_char("&GTarget set to none.\n\r",ch);
    	        ship->target = NULL;
    	        return;
    	    }
            
            target = get_ship_here( arg, ship->starsystem );
			if (  target == NULL || IS_SET( target->flags, SHIP_CLOAKED ) )
            {
                send_to_char("&RThat ship isn't here!\n\r",ch);
                return;
            }			
            
            if (  target == ship )
            {
                send_to_char("&RYou can't target your own ship!\n\r",ch);
                return;
            }
            
            if ( !str_cmp(target->owner, ship->owner) && str_cmp( target->owner , "" ) )
            {
                send_to_char("&RThat ship has the same owner... try targetting an enemy ship instead!\n\r",ch);
                return;
            }
            
			if ( find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, ship->vz ) > 5000 )
            {
                send_to_char("&RThat ship is too far away to target.\n\r",ch);
                return;
            }
            
            chance = IS_NPC(ch) ? ch->top_level
	                : (int)  (ch->pcdata->learned[gsn_weaponsystems]) ;
            if ( number_percent( ) < chance )
    		{
    		   send_to_char( "&GTracking target.\n\r", ch);
    		   act( AT_PLAIN, "$n makes some adjustments on the targeting computer.", ch,
		        NULL, argument , TO_ROOM );
    		   add_timer ( ch , TIMER_DO_FUN , 1 , do_target , 1 );
    		   ch->dest_buf = str_dup(arg);
    		   return;
	        }
	        send_to_char("&RYou fail to work the controls properly.\n\r",ch);
	        learn_from_failure( ch, gsn_weaponsystems );
    	   	return;	
    	
    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		strcpy(arg, ch->dest_buf);
    		DISPOSE( ch->dest_buf);
    		break;
    		
    	case SUB_TIMER_DO_ABORT:
    		DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
    	        send_to_char("&RYour concentration is broken. You fail to lock onto your target.\n\r", ch);
    		return;
    }
    
    ch->substate = SUB_NONE;
    
    if ( (ship = ship_from_turret(ch->in_room)) == NULL )
    {  
       return;
    }
    
    target = get_ship_here( arg, ship->starsystem );
    if (  target == NULL || target == ship)
    {
           send_to_char("&RThe ship has left the starsytem. Targeting aborted.\n\r",ch);
           return;
    }
    
    if ( ch->in_room == ship->gunseat )
        ship->target = target;
    else
    {
        for ( turret = ship->first_turret ; turret ; turret = turret->next )    
          if ( turret->room == ch->in_room )
             break;
             
        if ( !turret )
           return;
           
        turret->target = target;
    }
        
    send_to_char( "&GTarget Locked.\n\r", ch);
    sprintf( buf , "You are being targetted by %s." , ship->name);  
    echo_to_cockpit( AT_BLOOD , target , buf );
       	      
    learn_from_success( ch, gsn_weaponsystems );
    	
    if ( autofly(target) && !target->target)
    {
       sprintf( buf , "You are being targetted by %s." , target->name);
       echo_to_cockpit( AT_BLOOD , ship , buf );
       target->target = ship;
    }
}

void do_fire(CHAR_DATA *ch, char *argument )
{
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];
    TURRET_DATA *turret;
    
        if (  (ship = ship_from_turret(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be in the gunners chair or turret of a ship to do that!\n\r",ch);
            return;
        }
        
        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
        if (ship->shipstate == SHIP_HYPERSPACE)
        {
             send_to_char("&RYou can only do that in realspace!\n\r",ch);
             return;   
        }
    	if (ship->starsystem == NULL)
    	{
    	     send_to_char("&RYou can't do that until after you've finished launching!\n\r",ch);
    	     return;
    	}
    	if ( ship->energy <5 )
        {
             send_to_char("&RTheres not enough energy left to fire!\n\r",ch);
             return;
        }   
  
                if ( autofly(ship) )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first.\n\r",ch);
    	            return;
    	        }
    	              
        
        chance = IS_NPC(ch) ? ch->top_level
                 : (int) ( ch->perm_dex*2 + ch->pcdata->learned[gsn_weaponsystems]/2
                           + ch->pcdata->learned[gsn_spacecombat]/2 );        
        
    	if ( !str_prefix( argument , "lasers") )
    	{
    	      if ( ch->in_room == ship->gunseat )
              {
                  if (ship->laserstate == LASER_DAMAGED)
    	          {
    	                  send_to_char("&RThe ships main laser is damaged.\n\r",ch);
    	      	          return;
    	     	  }
             	  if (ship->laserstate >= ship->lasers )
    	     	  {
    	     		send_to_char("&RThe lasers are still recharging.\n\r",ch);
    	     		return;
    	     	  }
    	     	  if (ship->target == NULL )
    	     	  {
    	     		send_to_char("&RYou need to choose a target first.\n\r",ch);
    	     		return;
    	     	  }    	    
    	     	  target = ship->target;
              }    
              else
              {
                  for ( turret = ship->first_turret ; turret ; turret = turret->next )    
                    if ( turret->room == ch->in_room )
                      break;
              
                  if ( !turret )
    	          {
    	                  send_to_char("&RThis turret is out of order.\n\r",ch);
    	      	          return;
    	     	  } 
                                       
                  if (turret->laserstate == LASER_DAMAGED)
    	          {
    	                  send_to_char("&RThe turret is damaged.\n\r",ch);
    	      	          return;
    	     	  } 
             	  if (turret->laserstate > 1 )
    	     	  {
    	     		send_to_char("&RThe lasers are still recharging.\n\r",ch);
    	     		return;
    	     	  }
    	     	  if (turret->target == NULL )
    	     	  {
    	     		send_to_char("&RYou need to choose a target first.\n\r",ch);
    	     		return;
    	     	  }    	    
    	     	  target = turret->target;
              }
    	
    	     if (target->starsystem != ship->starsystem)
    	     {
    	     	send_to_char("&RYour target seems to have left.\n\r",ch);
    	        ship->target = NULL; 
    	     	return;
    	     } 
			 if ( find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 1000 )
             {
                send_to_char("&RThat ship is out of laser range.\n\r",ch);
    	     	return;
             } 
             if ( ship->class < SPACE_STATION && !is_facing( ship, target ) )
             {
                send_to_char("&RThe main laser can only fire forward. You'll need to turn your ship!\n\r",ch);
    	     	return;
             }      
             if ( ch->in_room == ship->gunseat )
                 ship->laserstate++;
             else
                 turret->laserstate++;
             chance += target->model*5;
             chance -= target->manuever/10;
             chance -= target->currspeed/20;
             chance -= ( abs(target->vx - ship->vx)/70 );
             chance -= ( abs(target->vy - ship->vy)/70 );
             chance -= ( abs(target->vz - ship->vz)/70 );
             chance = URANGE( 10 , chance , 90 );
             act( AT_PLAIN, "$n presses the fire button.", ch,
                  NULL, argument , TO_ROOM );
             if ( number_percent( ) > chance )
             {  
                sprintf( buf , "Lasers fire from %s at you but miss." , ship->name);  
                echo_to_cockpit( AT_ORANGE , target , buf );
                sprintf( buf , "The ships lasers fire at %s but miss." , target->name);  
                echo_to_cockpit( AT_ORANGE , ship , buf );
    	        sprintf( buf, "Laserfire from %s barely misses %s." , ship->name , target->name );
                echo_to_system( AT_ORANGE , ship , buf , target );
    	        return;
             }
             sprintf( buf, "Laserfire from %s hits %s." , ship->name, target->name );
             echo_to_system( AT_ORANGE , ship , buf , target );
             sprintf( buf , "You are hit by lasers from %s!" , ship->name);
             echo_to_cockpit( AT_BLOOD , target , buf );
             sprintf( buf , "Your ships lasers hit %s!." , target->name);
             echo_to_cockpit( AT_YELLOW , ship , buf );
             learn_from_success( ch, gsn_spacecombat );
             echo_to_ship( AT_RED , target , "A small explosion vibrates through the ship." );
             if ( turret )
                damage_ship_ch( target , 15 , 25 , ch );
             else
                damage_ship_ch( target , 5 , 10 , ch );

             if ( target->starsystem && autofly(target) && target->target != ship )
             {
                target->target = ship;
                sprintf( buf , "You are being targetted by %s." , target->name);
                echo_to_cockpit( AT_BLOOD , ship , buf );
             }
             
             return;
    	}
        
        if ( !str_prefix( argument , "missile") )
    	{
    	     if (ship->missilestate == MISSILE_DAMAGED)
    	     {
    	        send_to_char("&RThe ships missile launchers are dammaged.\n\r",ch);
    	      	return;
    	     } 
             if (ship->missiles <= 0)
    	     {
    	     	send_to_char("&RYou have no missiles to fire!\n\r",ch);
    	        return;
    	     }
    	     if (ship->missilestate != MISSILE_READY )
    	     {
    	     	send_to_char("&RThe missiles are still reloading.\n\r",ch);
    	     	return;
    	     }
    	     if (ship->target == NULL )
    	     {
    	     	send_to_char("&RYou need to choose a target first.\n\r",ch);
    	     	return;
    	     }    	    
    	     target = ship->target;
             if (ship->target->starsystem != ship->starsystem)
    	     {
    	     	send_to_char("&RYour target seems to have left.\n\r",ch);
    	        ship->target = NULL; 
    	     	return;
    	     } 			 

			 if ( find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 1000 )
             {
                send_to_char("&RThat ship is out of missile range.\n\r",ch);
    	     	return;
             } 
             if ( ship->class < SPACE_STATION && !is_facing( ship, target ) )
             {
                send_to_char("&RMissiles can only fire in a forward. You'll need to turn your ship!\n\r",ch);
    	     	return;
             }      
             chance -= target->manuever/5;
             chance -= target->currspeed/20;
             chance += target->model*target->model*2;
             chance -= ( abs(target->vx - ship->vx)/100 );
             chance -= ( abs(target->vy - ship->vy)/100 );
             chance -= ( abs(target->vz - ship->vz)/100 );
             chance += ( 30 );
             chance = URANGE( 20 , chance , 80 );
             act( AT_PLAIN, "$n presses the fire button.", ch,
                  NULL, argument , TO_ROOM );
             if ( number_percent( ) > chance )
             {
                send_to_char( "&RYou fail to lock onto your target!", ch );
                ship->missilestate = MISSILE_RELOAD_2;
    	        return;	
             } 
             new_missile( ship , target , ch  );
             ship->missiles-- ;
             act( AT_PLAIN, "$n presses the fire button.", ch,
                  NULL, argument , TO_ROOM );
             echo_to_cockpit( AT_YELLOW , ship , "Missiles launched.");
             sprintf( buf , "Incoming missile from %s." , ship->name);  
             echo_to_cockpit( AT_BLOOD , target , buf );
             sprintf( buf, "%s fires a missile towards %s." , ship->name, target->name );
             echo_to_system( AT_ORANGE , ship , buf , target );
             learn_from_success( ch, gsn_weaponsystems );
             if ( ship->class == SPACE_STATION )
                   ship->missilestate = MISSILE_RELOAD;
             else
                   ship->missilestate = MISSILE_FIRED;
             
             if ( autofly(target) && target->target != ship )
             {
                target->target = ship;
                sprintf( buf , "You are being targetted by %s." , target->name);  
                echo_to_cockpit( AT_BLOOD , ship , buf );
             }
             
             return;
    	}
        
        send_to_char( "&RYou can't fire that!\n\r" , ch);
    	
}


void do_calculate(CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];  
    int chance , count;
    SHIP_DATA *ship;
    PLANET_DATA *planet;
    SPACE_DATA *starsystem;
    
    argument = one_argument( argument , arg1);
    argument = one_argument( argument , arg2);
    argument = one_argument( argument , arg3);

    
    	        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
    	        {
    	            send_to_char("&RYou must be in control of a ship to do that!\n\r",ch);
    	            return;
    	        }
                
                if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
                if ( autofly(ship)  )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first....\n\r",ch);
    	            return;
    	        }
    	        
                if  ( ship->class == SPACE_STATION )
                {
                   send_to_char( "&RAnd what exactly are you going to calculate...?\n\r" , ch );
                   return;
                }   
    	        if (ship->hyperspeed == 0)
                {
                  send_to_char("&RThis ship is not equipped with a hyperdrive!\n\r",ch);
                  return;   
                }
                if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\n\r",ch);
    	            return;
    	        }
    	        if (ship->starsystem == NULL)
    	        {
    	            send_to_char("&RYou can only do that in realspace.\n\r",ch);
    	            return;
    	        }
    	        if (argument[0] == '\0')
    	        {
    	            send_to_char("&WFormat: Calculate <starsystem> <entry x> <entry y> <entry z>\n\r&wPossible destinations:\n\r",ch);
    	            for ( starsystem = first_starsystem; starsystem; starsystem = starsystem->next )
                    {
                       set_char_color( AT_NOTE, ch );
                       ch_printf(ch,"%s ----- ",starsystem->name);
                       count++;
                    }
                    if ( !count )
                    {
                        send_to_char( "No Starsystems found.\n\r", ch );
                    }
                    return;
    	        }
             	chance = IS_NPC(ch) ? 100
             		: (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
                if ( number_percent( ) > chance )
    		{
	           send_to_char("&RYou cant seem to figure the charts out today.\n\r",ch);
    	   	   return;	
    	   	}

    
    ship->currjump = starsystem_from_name( arg1 );
    ship->jx = atoi(arg2);
    ship->jy = atoi(arg3);
    ship->jz = atoi(argument);
    
    if ( ship->currjump == NULL )
    {
        send_to_char( "&RYou can't seem to find that starsytem on your charts.\n\r", ch);
        return;
    }
    else
    { 
        SPACE_DATA * starsystem;
      
        starsystem = ship->currjump;

		if ( starsystem->star1
			&& strcmp( starsystem->star1, "" )
			&& find_range( ship->jx, ship->jy, ship->jz, starsystem->s1x, starsystem->s1y, starsystem->s1z ) < 300 )
                {
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object.");
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set.");
                    ship->currjump = NULL;
                    return;
                }              
          else if ( starsystem->star2
				  && strcmp( starsystem->star2, "" )
				  && find_range( ship->jx, ship->jy, ship->jz, starsystem->s2x, starsystem->s2y, starsystem->s2z ) < 300 )
                {
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object.");
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set.");
                    ship->currjump = NULL;
                    return;
                }
          for ( planet = starsystem->first_planet ; planet ; planet = planet->next_in_system )                 
			  if ( find_range( ship->jx, ship->jy, ship->jz, planet->x, planet->y, planet->z ) < 300 )
                {
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates too close to stellar object.");
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set.");
                    ship->currjump = NULL;
                    return;
                }
                            
          ship->jx += number_range ( -250 , 250 );
          ship->jy += number_range ( -250 , 250 );
          ship->jz += number_range ( -250 , 250 );
    }
    
    if ( ship->starsystem == ship->currjump )
        ship->hyperdistance = number_range(0, 200);
    else
        ship->hyperdistance = number_range(500 , 1000);
    
    send_to_char( "&GHyperspace course set. Ready for the jump to lightspeed.\n\r", ch);
    act( AT_PLAIN, "$n does some calculations using the ships computer.", ch,
		        NULL, argument , TO_ROOM );
	                
    learn_from_success( ch, gsn_spacecraft );
    	
    	
    WAIT_STATE( ch , 2*PULSE_VIOLENCE );	
}

void do_recharge(CHAR_DATA *ch, char *argument )
{
    int recharge;
    int chance;
    SHIP_DATA *ship;
    int energy_buffer;      
   
        if (  (ship = ship_from_gunseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be at the shield controls of a ship to do that!\n\r",ch);
            return;
        }

    /**
     * If you try to perform an action that will take you below less than 5% energy
     * it will fail to happen to protect you from running out of energy.
     */
    energy_buffer = ship->maxenergy * .05;  
        
                if ( autofly(ship)  )
    	        {
    	            send_to_char("&R...\n\r",ch);
    	            return;
    	        }
    	        
                if (ship->shipstate == SHIP_DISABLED)
    	        {
    	            send_to_char("&RThe ships drive is disabled. Unable to manuever.\n\r",ch);
    	            return;
    	        }

    /* Get how much shields that can be recharged. */
    recharge = ship->maxshield - ship->shield;
    /* Either recharge the most that ship can charge per a tick, or recharge enough to have full shields. */
    recharge = UMIN(recharge , 20+ship->model*10);

    /* If the recharge amount will bring it below the energy buffer then say there is not enough energy. */
    if ((ship->energy - recharge) < energy_buffer)
    {
        send_to_char("&RTheres not enough energy!\n\r", ch);
        return;
    }
    	        
        chance = IS_NPC(ch) ? 100
             		: (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
        if ( number_percent( ) > chance )
        {
            send_to_char("&RYou fail to work the controls properly.\n\r",ch);
    	   return;	
        }
           

    send_to_char( "&GRecharging shields..\n\r", ch);
    act( AT_PLAIN, "$n pulls back a lever on the control panel.", ch,
         NULL, argument , TO_ROOM );
    
    learn_from_success( ch, gsn_spacecraft );
    	         
    ship->shield += recharge;
    ship->energy -= ( recharge*5 );        
}


void do_repairship(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int chance, change;
    SHIP_DATA *ship;
    
    strcpy( arg, argument );    
    
    switch( ch->substate )
    { 
    	default:
    	        if (  (ship = ship_from_engine(ch->in_room))  == NULL )
    	        {
    	            send_to_char("&RYou must be in the engine room of a ship to do that!\n\r",ch);
    	            return;
    	        }
                
                if ( str_cmp( argument , "hull" ) && str_cmp( argument , "drive" ) && 
                     str_cmp( argument , "launcher" ) && str_cmp( argument , "laser" ) )
                {
                   send_to_char("&RYou need to spceify something to repair:\n\r",ch);
                   send_to_char("&rTry: hull, drive, launcher, laser\n\r",ch);
                   return;
                }
                            
                chance = IS_NPC(ch) ? ch->top_level
	                 : (int) (ch->pcdata->learned[gsn_shipmaintenance]);
                if ( number_percent( ) < chance )
    		{
    		   send_to_char( "&GYou begin your repairs\n\r", ch);
    		   act( AT_PLAIN, "$n begins repairing the ships $T.", ch,
		        NULL, argument , TO_ROOM );
    		   if ( !str_cmp(arg,"hull") )
    		     add_timer ( ch , TIMER_DO_FUN , 15 , do_repairship , 1 );
    		   else
    		     add_timer ( ch , TIMER_DO_FUN , 5 , do_repairship , 1 );
    		   ch->dest_buf = str_dup(arg);
    		   return;
	        }
	        send_to_char("&RYou fail to locate the source of the problem.\n\r",ch);
    	   	return;	
    	
    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		strcpy(arg, ch->dest_buf);
    		DISPOSE( ch->dest_buf);
    		break;
    		
    	case SUB_TIMER_DO_ABORT:
    		DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
    	        send_to_char("&RYou are distracted and fail to finish your repairs.\n\r", ch);
    		return;
    }
    
    ch->substate = SUB_NONE;
    
    if ( (ship = ship_from_engine(ch->in_room)) == NULL )
    {  
       return;
    }
    
    if ( !str_cmp(arg,"hull") )
    {
        change = URANGE( 0 , 
                         number_range( (int) ( ch->pcdata->learned[gsn_shipmaintenance] / 2 ) , (int) (ch->pcdata->learned[gsn_shipmaintenance]) ),
                         ( ship->maxhull - ship->hull ) );
        ship->hull += change;
        ch_printf( ch, "&GRepair complete.. Hull strength inreased by %d points.\n\r", change );
    }
    
    if ( !str_cmp(arg,"drive") )
    {  
       if (ship->location == ship->lastdoc)
          ship->shipstate = SHIP_DOCKED;
       else
          ship->shipstate = SHIP_READY;
       send_to_char("&GShips drive repaired.\n\r", ch);		
    }
    
    if ( !str_cmp(arg,"launcher") )
    {  
       ship->missilestate = MISSILE_READY;
       send_to_char("&GMissile launcher repaired.\n\r", ch);
    }
    
    if ( !str_cmp(arg,"laser") )
    {  
       ship->laserstate = LASER_READY;
       send_to_char("&GMain laser repaired.\n\r", ch);
    }
    
    act( AT_PLAIN, "$n finishes the repairs.", ch,
         NULL, argument , TO_ROOM );

    learn_from_success( ch, gsn_shipmaintenance );
    	
}


/**
 * A command that adds another player as a pilot of a character's ship.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_addpilot(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    char arg1[MAX_INPUT_LENGTH];
    
    if (  (ship = ship_from_cockpit(ch->in_room))  == NULL )
    {
            send_to_char("&RYou must be in the cockpit of a ship to do that!\n\r",ch);
            return;
    }

    if  ( ship->class == SPACE_STATION )
    {
        send_to_char( "&RYou can't do that here.\n\r" , ch );
        return;
    }   

    if (!check_ship_owner(ch, ship))
    {   
        send_to_char( "&RThat isn't your ship!" ,ch );
        return;
    }

    one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
        send_to_char( "&RAdd which pilot?\n\r" ,ch );
        return;
    }

    if (!str_cmp(ch->name, arg1)
    {
        ch_printf(ch, "You already are a pilot of this ship!\r\n");
        return;
    }

    if ( str_cmp( ship->pilot , "" ) )
    {        
        if (!str_cmp(ship->pilot, arg1))
        {
            ch_printf(ch, "They already are a pilot!\r\n");
            return;
        }

        if ( str_cmp( ship->copilot , "" ) )
        {
            send_to_char( "&RYou are ready have a pilot and copilot..\n\r" ,ch );
            send_to_char( "&RTry rempilot first.\n\r" ,ch );
            return;
        }        
        
        STRFREE( ship->copilot );
        ship->copilot = STRALLOC( argument );
        send_to_char( "Copilot Added.\n\r", ch );
        save_ship( ship );
        return;
    }

    STRFREE( ship->pilot );
    ship->pilot = STRALLOC( argument );
    send_to_char( "Pilot Added.\n\r", ch );
    save_ship( ship );
}

/**
 * A command that removes a player as a pilot or copilot of a ship.
 *
 * @author      unknown
 * @author      Joel McBeth
 * @version     1.0     07/28/05
 *
 * @param       ch          The character that called the command.
 * @param       argument    The arguments passed with the command.
 */
void do_rempilot(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    char arg1[MAX_INPUT_LENGTH];

    if (  (ship = ship_from_cockpit(ch->in_room))  == NULL )
    {
            send_to_char("&RYou must be in the cockpit of a ship to do that!\n\r",ch);
            return;
    }
   
    if  ( ship->class == SPACE_STATION )
    {
        send_to_char( "&RYou can't do that here.\n\r" , ch );
        return;
    }   
   
    if ( !check_ship_owner(ch) )
    {   
        send_to_char( "&RThat isn't your ship!" ,ch );
        return;   
    }

    if (argument[0] == '\0')
    {
        send_to_char( "&RRemove which pilot?\n\r" ,ch );
        return;
    }

    if ( !str_cmp( ship->pilot , arg1 ) )
    {
        STRFREE( ship->pilot );
        ship->pilot = QUICKLINK( ship->copilot );
        STRFREE(ship->copilot);
        send_to_char( "Pilot Removed.\n\r", ch );
        save_ship( ship );
        return;
    }       

    if ( !str_cmp( ship->copilot , arg1 ) )
    {      
        STRFREE( ship->copilot );
        ship->copilot = STRALLOC( "" );
        send_to_char( "Copilot Removed.\n\r", ch );
        save_ship( ship );
        return;
    }    

    send_to_char( "&RThat person isn't listed as one of the ships pilots.\n\r" ,ch );
}

void do_radar( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *target;
    int chance;
    SHIP_DATA *ship;
    MISSILE_DATA *missile;  
    PLANET_DATA *planet;  
    ASTEROID_DATA *asteroid;
   
        if (   (ship = ship_from_cockpit(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be in the cockpit or turret of a ship to do that!\n\r",ch);
            return;
        }
        
        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
        if (ship->shipstate == SHIP_DOCKED)
        {
            send_to_char("&RWait until after you launch!\n\r",ch);
            return;
        }
        
        if (ship->shipstate == SHIP_HYPERSPACE)
        {
            send_to_char("&RYou can only do that in realspace!\n\r",ch);
            return;
        }
        
    	if (ship->starsystem == NULL)
    	{
    	       send_to_char("&RYou can't do that unless the ship is flying in realspace!\n\r",ch);
    	       return;
    	}        
    	        
        chance = IS_NPC(ch) ? 100
             		: (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
        if ( number_percent( ) > chance )
        {
           send_to_char("&RYou fail to work the controls properly.\n\r",ch);
    	   return;	
        }
        
    
    act( AT_PLAIN, "$n checks the radar.", ch,
         NULL, argument , TO_ROOM );
     
    	           set_char_color(  AT_WHITE, ch );
    	           ch_printf(ch, "%s\n\r\n\r" , ship->starsystem->name );
    	           if ( ship->starsystem->star1 && str_cmp(ship->starsystem->star1,"") ) 
    	                 ch_printf(ch, "&Y%s   %d %d %d\n\r" , 
    	                        ship->starsystem->star1,
    	                        ship->starsystem->s1x,
    	                        ship->starsystem->s1y,
    	                        ship->starsystem->s1z );
    	           if ( ship->starsystem->star2 && str_cmp(ship->starsystem->star2,"")  ) 
    	                 ch_printf(ch, "&Y%s   %d %d %d\n\r" , 
    	                        ship->starsystem->star2,
    	                        ship->starsystem->s2x,
    	                        ship->starsystem->s2y,
    	                        ship->starsystem->s2z );
		   ch_printf(ch,"\n\r");

    	           for ( planet = ship->starsystem->first_planet ; planet ; planet = planet->next_in_system ) {
                         if(!IS_SET(planet->flags, PLANET_HIDDEN) || IS_IMMORTAL(ch)) {
                                ch_printf(ch, "&G%s   %d %d %d [%d]\n\r",
    	                              planet->name,
    	                              planet->x,
    	                              planet->y,
    	                              planet->z,
                                         find_range(
                                                planet->x, planet->y, planet->z,
                                                 ship->vx,  ship->vy,  ship->vz
				         )
                                );
                         } //phel
		   }
		   ch_printf(ch,"\n\r");
//roids
    	           for ( asteroid = ship->starsystem->first_roid ; asteroid ; asteroid = asteroid->next ) {
			ch_printf(ch, "&c%s   %d %d %d [%d]\n\r",
				asteroid->name,
				asteroid->x,
				asteroid->y,
				asteroid->z,
					find_range(
						asteroid->x, asteroid->y, asteroid->z,
						ship->vx, ship->vy, ship->vz
					)
			);
		   } //phel
		   ch_printf(ch,"\n\r");

    	           for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
                   {       
                        if ( target != ship && ( !IS_SET( target->flags, SHIP_CLOAKED ) || IS_IMMORTAL( ch ) ) )
                           ch_printf(ch, "&C%s%s    %.0f %.0f %.0f [%d]\n\r", 
						    IS_SET( target->flags, SHIP_CLOAKED ) ? "(cloaked) " : "",
                           	target->name,
                           	target->vx,
                           	target->vy,
                           	target->vz,
                                find_range(
                                    target->vx, target->vy, target->vz,
                                      ship->vx,   ship->vy,   ship->vz)
                           );
                   }
                   ch_printf(ch,"\n\r");
    	           for ( missile = ship->starsystem->first_missile; missile; missile = missile->next_in_starsystem )
                   {        
                           ch_printf(ch, "&RA Missile    %d %d %d\n\r", 
                           	missile->mx,
                           	missile->my,
                                missile->mz );
                   }
                     
                   ch_printf(ch, "\n\r&WYour Coordinates: %.0f %.0f %.0f\n\r" , 
                             ship->vx , ship->vy, ship->vz);   
        
    	        
        learn_from_success( ch, gsn_spacecraft );
    	  
}

void do_autotrack( CHAR_DATA *ch, char *argument )
{
   SHIP_DATA *ship;
   int chance;
 
   if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
   {
        send_to_char("&RYou must be at the controls of a ship to do that!\n\r",ch);
        return;
   }
   
        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
 
        if ( ship->class == SPACE_STATION )
    	        {
    	            send_to_char("&RSpace Stations don't have autotracking systems!\n\r",ch);
    	            return;
    	        }
    	        
                if ( autofly(ship)  )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first....\n\r",ch);
    	            return;
    	        }
    	        
        chance = IS_NPC(ch) ? 100
             		: (int)  (ch->pcdata->learned[gsn_spacecraft]) ;
        if ( number_percent( ) > chance )
        {
           send_to_char("&RYour notsure which switch to flip.\n\r",ch);
    	   return;	
        }
   
   act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );
   if (ship->autotrack)
   {
     ship->autotrack = FALSE;
     echo_to_cockpit( AT_YELLOW , ship, "Autotracking off.");
   }
   else
   {
      ship->autotrack = TRUE;
      echo_to_cockpit( AT_YELLOW , ship, "Autotracking on.");
   }
   
   learn_from_success( ch, gsn_spacecraft );
    	        
}

void do_closebay( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	SHIP_DATA *ship;
	int chance;

	if( (ship=ship_from_cockpit(ch->in_room)) == NULL )
	{
		send_to_char("&RYou must be in the cockpit of a ship to do that!\n\r", ch);
		return;
	}
	if( ship->class > SPACE_STATION )
	{
		send_to_char("&RThis isn't a spacecraft!\n\r",ch);
		return;
	}
	if( !ship->first_hangar )
	{
		send_to_char("&RThis ship has no hangars!\n\r",ch);
		return;
	}
	if( ship->class == SPACE_STATION )
	{
		send_to_char("&RPlatforms don't have bay doors!\n\r",ch);
		return;
	}

	chance = IS_NPC(ch) ? ch->top_level
	: (int) (ch->pcdata->learned[gsn_spacecraft]);
	if( number_percent() > chance )
	{
		send_to_char("&RYou're not sure which switch to flip.\n\r", ch);
		learn_from_failure( ch, gsn_spacecraft );
	}

	act( AT_PLAIN, "$n flips a switch on the control panel.", ch, NULL, argument, TO_ROOM );

	if( ship->first_hangar->bayopen == TRUE)
	{
		ship->first_hangar->bayopen = FALSE;
		echo_to_cockpit( AT_YELLOW, ship, "Closing bay doors.");
		sprintf( buf, "%s's bay doors slowly close.", ship->name);
		echo_to_system( AT_BLUE, ship, buf, NULL );
	}
	else
	{
		echo_to_cockpit( AT_YELLOW, ship, "&RBay doors are already closed!");
		return;
	}

	learn_from_success( ch, gsn_spacecraft );
}
void do_openbay( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	SHIP_DATA *ship;
	int chance;

	if (  (ship = ship_from_cockpit(ch->in_room))  == NULL ) 
	{ 
		send_to_char("&RYou must be in the cockpit of a ship to do that!\n\r",ch); 
		return; 
	} 
 
	if ( ship->class > SPACE_STATION ) 
	{ 
		send_to_char("&RThis isn't a spacecraft!\n\r",ch); 
		return; 
	} 
 
	if ( !ship->first_hangar )
	{
		send_to_char("&RYou don't even have hangars!\n\r",ch);
		return;
	}
 
	if ( ship->class == SPACE_STATION ) 
	{ 
		send_to_char("&RPlatforms don't have bay doors!\n\r",ch); 
		return; 
	} 
 
	if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL ) 
	{ 
		send_to_char("&RYou aren't in the pilots chair!\n\r",ch); 
		return; 
	} 
 
	chance = IS_NPC(ch) ? ch->top_level 
	: (int)  (ch->pcdata->learned[gsn_spacecraft]) ; 
	if ( number_percent( ) > chance ) 
	{ 
		send_to_char("&RYou're not sure which switch to flip.\n\r",ch); 
		learn_from_failure( ch, gsn_spacecraft ); 
		return; 
	} 
 
	act( AT_PLAIN, "$n flips a switch on the control panel.", ch, 
	NULL, argument , TO_ROOM ); 
 
	if (ship->first_hangar->bayopen == FALSE ) 
	{ 
		ship->first_hangar->bayopen = TRUE; 
		echo_to_cockpit( AT_YELLOW , ship, "Opening bay doors.");
		sprintf( buf ,"%s's bay doors slowly open.", ship->name );
		echo_to_system ( AT_BLUE , ship, buf, NULL );
    	}
  	else 
	{  
	echo_to_cockpit( AT_YELLOW , ship, "&RBay doors are already open!"); 
		return;
	} 
 
	learn_from_success( ch, gsn_spacecraft );
}


void do_tractorbeam( CHAR_DATA *ch, char *argument )
{

    char arg[MAX_INPUT_LENGTH];
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
	char buf[MAX_STRING_LENGTH];
    
    strcpy( arg, argument );

	if ( (ship = ship_from_pilotseat(ch->in_room)) == NULL )
	{
		send_to_char("&RYou must be in the pilot's seat of a ship to do that!\n\r",ch);
		return;
	}

    	    
	if ( ship->class > SPACE_STATION )
	{
		send_to_char("&RThis isn't a spacecraft!\n\r",ch);
		return;
	}


	if ( !check_ship_pilot( ch , ship ) )
	{
		send_to_char("This isn't your ship!\n\r" , ch );
		return;
	}

	if ( ship->tractorbeam == 0 )
	{
		send_to_char("You might want to install a tractorbeam!\n\r" , ch );
		return;
	}
    
	if ( ship->first_hangar == 0 )
	{
		send_to_char("No hangar available.\n\r",ch);
		return;
	}
	     
		if( ship->class == SPACE_STATION )
	{
		send_to_char("&RA Space Station? really!\n\r",ch);
		return;
	}

	if ( !ship->first_hangar->bayopen )
	{
		send_to_char("Your hangar is closed.\n\r",ch);
		return;
	}
 	        
    	            	        
	if (ship->shipstate == SHIP_DISABLED)
	{
		send_to_char("&RThe ships drive is disabled. No power available.\n\r",ch);
		return;
	}

	if (ship->shipstate == SHIP_DOCKED)
	{
		send_to_char("&RYour ship is docked!\n\r",ch);    
		return;	        
	}
    	                			
	if (ship->shipstate == SHIP_HYPERSPACE)		   
	{
					
		send_to_char("&RYou can only do that in realspace!\n\r",ch);
		return;   			
	}

	if (ship->shipstate != SHIP_READY)
	{
		send_to_char("&RPlease wait until the ship has finished its current manouver.\n\r",ch);
		return;	    
	}



    	        
	if ( argument[0] == '\0' )	        
	{  	
		send_to_char("&RCapture what?\n\r",ch);	
		return;				
	}
   	            
	target = get_ship_here( argument , ship->starsystem );

	if ( target == NULL )            
	{   	                
		send_to_char("&RI don't see that here.\n\r",ch);   	 
		return;             
	} 
   	           
	if ( target == ship )   	
	{   	               
		send_to_char("&RYou can't yourself!\n\r",ch);   	 
		return;              
	}   	           
   	         
	if ( target->shipstate == SHIP_LAND )
	{
		send_to_char("&RThat ship is already in a landing sequence.\n\r", ch);
		return;
	}

	if ( find_range( ship->vx, ship->vy, ship->vz, target->vx, target->vy, target->vz ) > 200 )
	{
		send_to_char("&R That ship is too far away! You'll have to fly a little closer.\n\r",ch);
		return;
	}   
    
	     /*if ( ( model[target->model].rooms - 5 ) < model[ship->model].rooms )
		      {
			send_to_char("&RYour ship is too big to land there!\n\r", ch );
			return;
		      }*/
    	        
  

	if ( ship->energy < (25 + 25*target->class) )
	{
		send_to_char("&RTheres not enough fuel!\n\r",ch);
		return;
	}



				           
	chance = IS_NPC(ch) ? ch->top_level
	: (int)  (ch->pcdata->learned[gsn_weaponsystems]);

	/* This is just a first guess chance modifier, feel free to change if needed */

	chance = chance * ( ship->tractorbeam / (target->currspeed+1 ) );
 
	if ( number_percent( ) < chance )
	{    		   
		set_char_color( AT_GREEN, ch );    
		send_to_char( "Capture sequence initiated.\n\r", ch);    		   
		act( AT_PLAIN, "$n begins the capture sequence.", ch,		        
			NULL, argument , TO_ROOM );
		echo_to_ship( AT_YELLOW , ship , "ALERT: Ship is being captured, all hands to docking bay." );
    	echo_to_ship( AT_YELLOW , target , "The ship shudders as a tractorbeam locks on." );
		sprintf( buf , "You are being captured by %s." , ship->name);  
		echo_to_cockpit( AT_BLOOD , target , buf );

		/*if ( autofly(target) && !target->target0)
			target->target0 = ship;*/

		target->dest = STRALLOC(ship->name);    		   
		target->shipstate = SHIP_LAND;    		   
		target->currspeed = 0;	           
	                     
		learn_from_success( ch, gsn_weaponsystems );	
		return;
	                  
	}	       
	send_to_char("You fail to work the controls properly.\n\r",ch);
   	echo_to_ship( AT_YELLOW , target , "The ship shudders and then stops as a tractorbeam attemps to lock on." );
	sprintf( buf , "The %s attempted to capture your ship!" , ship->name);  
	echo_to_cockpit( AT_BLOOD , target , buf );
	//if ( autofly(target) && !target->target0)
		//target->ship = ship;

                  
	learn_from_failure( ch, gsn_weaponsystems );                
	  	
   	return;	
}





void do_fly( CHAR_DATA *ch, char *argument )
{}

void do_drive( CHAR_DATA *ch, char *argument )
{
    int dir;
    SHIP_DATA *ship;
    
        if (  (ship = ship_from_pilotseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be in the drivers seat of a land vehicle to do that!\n\r",ch);
            return;
        }
        
        if ( ship->class != LAND_VEHICLE )
    	{
    	      send_to_char("&RThis isn't a land vehicle!\n\r",ch);
    	      return;
    	}
    	        
        
        if (ship->shipstate == SHIP_DISABLED)
    	{
    	     send_to_char("&RThe drive is disabled.\n\r",ch);
    	     return;
    	}
    	        
        if ( ship->energy <1 )
        {
              send_to_char("&RTheres not enough fuel!\n\r",ch);
              return;
        }
        
        if ( ( dir = get_door( argument ) ) == -1 )
        {
             send_to_char( "Usage: drive <direction>\n\r", ch );     
             return;
        }
        
        drive_ship( ch, ship, get_exit(get_room_index(ship->location), dir), 0 );

}

ch_ret drive_ship( CHAR_DATA *ch, SHIP_DATA *ship, EXIT_DATA  *pexit , int fall )
{
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    ROOM_INDEX_DATA *from_room;
    ROOM_INDEX_DATA *original;
    char buf[MAX_STRING_LENGTH];
    char *txt;
    char *dtxt;
    ch_ret retcode;
    sh_int door, distance;
    bool drunk = FALSE;
    CHAR_DATA * rch;
    CHAR_DATA * next_rch;
    

    if ( !IS_NPC( ch ) )
      if ( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE )
	&& ( ch->position != POS_DRAG ) )
	drunk = TRUE;

    if ( drunk && !fall )
    {
      door = number_door();
      pexit = get_exit( get_room_index(ship->location), door );
    }

#ifdef DEBUG
    if ( pexit )
    {
	sprintf( buf, "drive_ship: %s to door %d", ch->name, pexit->vdir );
	log_string( buf );
    }
#endif

    retcode = rNONE;
    txt = NULL;

    in_room = get_room_index(ship->location);
    from_room = in_room;
    if ( !pexit || (to_room = pexit->to_room) == NULL )
    {
	if ( drunk )
	  send_to_char( "You drive into a wall in your drunken state.\n\r", ch );
	 else
	  send_to_char( "Alas, you cannot go that way.\n\r", ch );
	return rNONE;
    }

    door = pexit->vdir;
    distance = pexit->distance;

    if ( IS_SET( pexit->exit_info, EX_WINDOW )
    &&  !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
	send_to_char( "Alas, you cannot go that way.\n\r", ch );
	return rNONE;
    }

    if (  IS_SET(pexit->exit_info, EX_PORTAL) 
       && IS_NPC(ch) )
    {
        act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
	return rNONE;
    }

    if ( IS_SET(pexit->exit_info, EX_NOMOB)
	&& IS_NPC(ch) )
    {
	act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
	return rNONE;
    }

    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
    {
	if ( !IS_SET( pexit->exit_info, EX_SECRET )
	&&   !IS_SET( pexit->exit_info, EX_DIG ) )
	{
	  if ( drunk )
	  {
	    act( AT_PLAIN, "$n drives into the $d in $s drunken state.", ch,
		NULL, pexit->keyword, TO_ROOM );
	    act( AT_PLAIN, "You drive into the $d in your drunken state.", ch,
		NULL, pexit->keyword, TO_CHAR ); 
	  }
	 else
	  act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	}
       else
	{
	  if ( drunk )
	    send_to_char( "You hit a wall in your drunken state.\n\r", ch );
	   else
	    send_to_char( "Alas, you cannot go that way.\n\r", ch );
	}

	return rNONE;
    }

    if ( room_is_private( ch, to_room ) )
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return rNONE;
    }

    if ( !fall )
    {
        if ( IS_SET( to_room->room_flags, ROOM_INDOORS ) 
        || IS_SET( to_room->room_flags, ROOM_SPACECRAFT )  
        || to_room->sector_type == SECT_INSIDE ) 
	{
		send_to_char( "You can't drive indoors!\n\r", ch );
		return rNONE;
	}
        
	if ( in_room->sector_type == SECT_AIR
	||   to_room->sector_type == SECT_AIR
	||   IS_SET( pexit->exit_info, EX_FLY ) )
	{
            if ( ship->class > AIRCRAFT )
	    {
		send_to_char( "You'd need to fly to go there.\n\r", ch );
		return rNONE;
	    }
	}

	if ( in_room->sector_type == SECT_WATER_NOSWIM
	||   to_room->sector_type == SECT_WATER_NOSWIM 
	||   to_room->sector_type == SECT_WATER_SWIM 
	||   to_room->sector_type == SECT_UNDERWATER
	||   to_room->sector_type == SECT_OCEANFLOOR )
	{

	    if ( ship->class != BOAT && ship->class != SUBMARINE )
	    {
		send_to_char( "You'd need a boat to go there.\n\r", ch );
		return rNONE;
	    }
	    	    
	}

	if ( to_room->sector_type == SECT_UNDERWATER
	||   to_room->sector_type == SECT_OCEANFLOOR )
	{

	    if ( ship->class != SUBMARINE )
	    {
		send_to_char( "You'd need a submarine to go there.\n\r", ch );
		return rNONE;
	    }
	    	    
	}

	if ( IS_SET( pexit->exit_info, EX_CLIMB ) )
	{

	    if ( ship->class > AIRCRAFT )
	    {
		send_to_char( "You need to fly or climb to get up there.\n\r", ch );
		return rNONE;
	    }
	}

    }

    if ( to_room->tunnel > 0 )
    {
	CHAR_DATA *ctmp;
	int count = 0;
	
	for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
	  if ( ++count >= to_room->tunnel )
	  {
		  send_to_char( "There is no room for you in there.\n\r", ch );
		return rNONE;
	  }
    }

      if ( fall )
        txt = "falls";
      else
      if ( !txt )
      {
	  if (  ship->class < BOAT )
	      txt = "fly";
	  else
	  if ( ship->class <= SUBMARINE  )
	  {
	      txt = "float";
	  }
	  else
	  if ( ship->class > SUBMARINE  )
	  {
	      txt = "drive";
	  }
      }
      sprintf( buf, "$n %ss the vehicle $T.", txt );
      act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_ROOM );
      sprintf( buf, "You %s the vehicle $T.", txt );
      act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_CHAR );
      sprintf( buf, "%s %ss %s.", ship->name, txt, dir_name[door] );
      echo_to_room( AT_ACTION , get_room_index(ship->location) , buf );

      extract_ship( ship );
      ship_to_room(ship, to_room->vnum );
      
      ship->location = to_room->vnum;
      ship->lastdoc = ship->location;
    
      if ( fall )
        txt = "falls";
      else
	  if (  ship->class < BOAT )
	      txt = "flys in";
	  else
	  if ( ship->class < LAND_VEHICLE  )
	  {
	      txt = "floats in";
	  }
	  else
	  if ( ship->class == LAND_VEHICLE  )
	  {
	      txt = "drives in";
	  }

      switch( door )
      {
      default: dtxt = "somewhere";	break;
      case 0:  dtxt = "the south";	break;
      case 1:  dtxt = "the west";	break;
      case 2:  dtxt = "the north";	break;
      case 3:  dtxt = "the east";	break;
      case 4:  dtxt = "below";		break;
      case 5:  dtxt = "above";		break;
      case 6:  dtxt = "the south-west";	break;
      case 7:  dtxt = "the south-east";	break;
      case 8:  dtxt = "the north-west";	break;
      case 9:  dtxt = "the north-east";	break;
      }

    sprintf( buf, "%s %s from %s.", ship->name, txt, dtxt );
    echo_to_room( AT_ACTION , get_room_index(ship->location) , buf );
    
    for ( rch = ch->in_room->last_person ; rch ; rch = next_rch )
    { 
        next_rch = rch->prev_in_room;
        original = rch->in_room;
        char_from_room( rch );
        char_to_room( rch, to_room );
        do_look( rch, "auto" );
        char_from_room( rch );
        char_to_room( rch, original );
    }
    
/*
    if (  CHECK FOR FALLING HERE
    &&   fall > 0 )
    {
	if (!IS_AFFECTED( ch, AFF_FLOATING )
	|| ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) )
	{
	  set_char_color( AT_HURT, ch );
	  send_to_char( "OUCH! You hit the ground!\n\r", ch );
	  WAIT_STATE( ch, 20 );
	  retcode = damage( ch, ch, 50 * fall, TYPE_UNDEFINED );
	}
	else
	{
	  set_char_color( AT_MAGIC, ch );
	  send_to_char( "You lightly float down to the ground.\n\r", ch );
	}
    }

*/    
    return retcode;

}

void do_bomb( CHAR_DATA *ch, char *argument )
{	    
   SHIP_DATA *ship, * mobship;
   PLANET_DATA * planet;
   char buf[MAX_STRING_LENGTH];
   ROOM_INDEX_DATA * room;
   CHAR_DATA * vch;
    
   if ( ( ship = ship_from_turret( ch->in_room ) ) == NULL )
   {
      ch_printf( ch, "&RYou must be in the gunners chair or turret of a ship to do that!\n\r" );
      return;
   }
        
   if ( ship->class > SPACE_STATION )
   {
      ch_printf( ch, "&RThis isn't a spacecraft!\n\r" );
      return;
   }
    	        
    if ( ship->shipstate == SHIP_HYPERSPACE )
    {
        ch_printf( ch, "&RYou can only do that in realspace!\n\r" );
        return;   
    }

   if ( ship->starsystem == NULL )
   {
      ch_printf( ch, "&RYou can't do that until after you've finished launching!\n\r" );
      return;
   }

   if ( ship->energy < 5 )
   {
      ch_printf( ch, "&RTheres not enough energy left to fire!\n\r" );
      return;
   }

   if ( autofly( ship ) )
   {
      ch_printf( ch, "&RYou'll have to turn off the ships autopilot first.\n\r" );
      return;
   }

   if ( argument[0] == '\0' )
   {
      ch_printf( ch, "Usage: bomb <planet>\r\n" );
      return;
   }

   if ( ( planet = get_planet( argument ) ) == NULL )
   {
      ch_printf( ch, "&RThat planet doesn't exist.\r\n" );
      return;
   }

   if ( IS_SET( planet->flags, PLANET_NOBOMB ) )
   {
      ch_printf( ch, "&RThat planet can't be bombed.\r\n" );
      return;
   }

   if ( ship->starsystem != planet->starsystem )
   {
      ch_printf( ch, "&RThat planet is not in this system.\r\n" );
      return;
   }

   if ( find_range( ship->vx, ship->vy, ship->vz, planet->x, planet->y, planet->z ) >= 20 )
   {
      ch_printf( ch, "&RYou must be in orbit to bomb.\r\n" );
      return;
   }

   if ( !planet->area )
   {
      ch_printf( ch, "&RThere isn't anything to bomb on that planet.\r\n" );
      return;
   }
        
   if ( ch->in_room != ship->gunseat )
   {
      ch_printf( ch, "&RYou can only bomb a planet with the main lasers." );
      return;
   }

   if ( ship->laserstate == LASER_DAMAGED )
   {
      ch_printf( ch, "&RThe ships main laser is damaged.\n\r" );
      return;
   }

   if ( ship->laserstate >= ship->lasers )
   {
      ch_printf( ch, "&RThe lasers are still recharging.\n\r" );
      return;
   }      	   

   act( AT_PLAIN, "$n presses the fire button.", ch, NULL, NULL, TO_ROOM );   

   int shots, cnum = 0, rnum = number_range( 1, planet->size - ship->lasers );
   for ( room = planet->area->first_room; room; room = room->next_in_area )
   {
      if ( ++cnum == rnum )
      {
         for ( shots = 0; ship->laserstate < ship->lasers; shots++ )
         {						
            sprintf( buf, "Laserfire from %s hit %s." , ship->name, planet->name );
            echo_to_system( AT_ORANGE, ship, buf, NULL );

            sprintf( buf, "Your ships lasers hit %s!.", planet->name );
            echo_to_cockpit( AT_YELLOW, ship, buf );

            if ( IS_SET( room->room_flags, ROOM_INDOORS ) )
               echo_to_room( AT_DANGER, room, "There is a bright flash of light as a laser shot blast through the roof and engulfs the room in flames." );
            else
               echo_to_room( AT_DANGER, room, "There is a bright flash of light as a laser shot engulfs the room in flames." );

            for ( vch = room->first_person; vch; vch = vch->next_in_room )
               damage( ch, vch, number_range( 25, 50 ), RIS_ENERGY );

            // TODO: Have it go through and damage ships

            ship->laserstate++;

            room = room->next_in_area;

            if ( !room )
            {
               bug( "do_bomb: went past the end of the list." );
               break;
            }
         }         

         break;
      }
   }

   learn_from_success( ch, gsn_spacecombat );

   for ( mobship = ship->starsystem->first_ship; mobship; mobship = mobship->next_in_starsystem )
      if ( planet->governed_by != NULL && !str_cmp( planet->governed_by->name, mobship->owner ) && mobship->target == NULL )
      {  
         mobship->target = ship;
         
         sprintf( buf , "You are being targetted by %s.", mobship->name );
         echo_to_cockpit( AT_BLOOD, ship, buf );            
      }         
}

void do_chaff( CHAR_DATA *ch, char *argument )
{
    int chance;
    SHIP_DATA *ship;
    
   
        if (  (ship = ship_from_gunseat(ch->in_room))  == NULL )
        {
            send_to_char("&RYou must be at the weapon controls of a ship to do that!\n\r",ch);
            return;
        }
        
        if ( ship->class > SPACE_STATION )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n\r",ch);
    	            return;
    	        }
    	        
        
                if ( autofly(ship) )
    	        {
    	            send_to_char("&RYou'll have to turn the autopilot off first...\n\r",ch);
    	            return;
    	        }
    	        
                if (ship->shipstate == SHIP_HYPERSPACE)
                {
                  send_to_char("&RYou can only do that in realspace!\n\r",ch);
                  return;   
                }
    	        if (ship->shipstate == SHIP_DOCKED)
    	        {
    	            send_to_char("&RYou can't do that until after you've launched!\n\r",ch);
    	            return;
    	        }
                if (ship->chaff <= 0 )
    	        {
    	            send_to_char("&RYou don't have any chaff to release!\n\r",ch);
    	            return;
    	        }
                chance = IS_NPC(ch) ? ch->top_level
                 : (int)  (ch->pcdata->learned[gsn_weaponsystems]) ;
        if ( number_percent( ) > chance )
        {
            send_to_char("&RYou can't figure out which switch it is.\n\r",ch);
    	   return;	
        }
    
    ship->chaff--;
    
    ship->chaff_released++;
        
    send_to_char( "You flip the chaff release switch.\n\r", ch);
    act( AT_PLAIN, "$n flips a switch on the control pannel", ch,
         NULL, argument , TO_ROOM );
    echo_to_cockpit( AT_YELLOW , ship , "A burst of chaff is released from the ship.");
	  
    learn_from_success( ch, gsn_weaponsystems );
}

bool autofly( SHIP_DATA *ship )
{
 
     if (!ship)
        return FALSE;
     
     if ( ship->type == MOB_SHIP )
        return TRUE;
     
     if ( ship->autopilot )
        return TRUE;
     
     return FALSE;   
        
}

void do_rentship( CHAR_DATA *ch, char *argument )
{}

void do_goship( CHAR_DATA *ch, char *argument ) {
	ROOM_INDEX_DATA *fromroom;
	ROOM_INDEX_DATA *toroom;
	SHIP_DATA *ship;

	if ( !argument || argument[0] == '\0') {
		send_to_char( "Board what?\n\r", ch );
		return;
	}
   
	ship = get_ship( argument );
	if ( !ship ) {
		send_to_char( "No such ship.\n\r", ch );
		return;
	}
        if ( ( toroom = ship->entrance )  != NULL ) {
		char_from_room( ch );
		char_to_room( ch , toroom );
		act( AT_IMMORT, "$T", ch, NULL, ch->pcdata->bamfout ,  (int)fromroom );
		act( AT_IMMORT, "$T", ch, NULL, ch->pcdata->bamfin ,  (int)toroom );
		do_look( ch , "auto" );
	}                                                                  
} //phel

void do_slayship( CHAR_DATA *ch, char *argument ) {
	SHIP_DATA *ship;

	if ( !argument || argument[0] == '\0') {
		send_to_char( "Slay what?\n\r", ch );
		return;
	}

	ship = get_ship( argument );
	if ( !ship ) {
		send_to_char( "No such ship.\n\r", ch );
		return;
	}
	destroy_ship(ship, NULL);
} //phel

void do_renameship(CHAR_DATA *ch, char *argument) {
	SHIP_DATA   *ship;
	char oldname[MAX_STRING_LENGTH];
	char type[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];

	argument=one_argument(argument, oldname);

	if(oldname[0] == '\0') {
		send_to_char("What ship?\n\r", ch);
		return;
	}

	if(argument[0] == '\0') {
		send_to_char("Using default name.\n\r", ch);
	}

	ship = ship_in_room(ch->in_room, oldname);
	if ( !ship ) {
		act( AT_PLAIN, "I see no $T here.", ch, NULL, oldname, TO_CHAR );
		return;
	}

	if ( str_cmp( ship->owner , ch->name )) {
   	send_to_char( "&RThat isn't your ship!\n\r" ,ch );
   	return; }
  
   
     /*-Zakul-*/

	act( AT_PLAIN, "$n walks over to $T&w and paints a new name on it.", ch, NULL, oldname, TO_ROOM );
	act( AT_PLAIN, "You walk over to $T&w and paint a new name on it.", ch, NULL, oldname, TO_CHAR );

	sprintf(type, "%s%s",
                        ship->type == PLAYER_SHIP ? "" : "MOB " ,
                        ship->class == SPACECRAFT ? model[ship->model].name :
                       (ship->class == SPACE_STATION ? "Space Station" :
                       (ship->class == AIRCRAFT ? "Aircraft" :
                       (ship->class == BOAT ? "Boat" :
                       (ship->class == SUBMARINE ? "Submarine" :
                       (ship->class == LAND_VEHICLE ? "land vehicle" : "Unknown" ) ) ) ) )
	);

	if(argument[0]=='\0') {
		sprintf(buf, "&C %s's %s %s&w", ch->name, type, ship->filename);
	} else {
                sprintf(buf, "%s&c %s&w", argument, ship->filename);
	}

	STRFREE(ship->name);
	ship->name = STRALLOC(buf);
	save_ship(ship);

} //phel

void do_giveship(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	SHIP_DATA   *ship;
        CLAN_DATA *clan;
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];

	argument=one_argument(argument, arg1);
	argument=one_argument(argument, arg2);

	if(arg1[0] == '\0') {
		send_to_char("What ship?\n\r", ch);
		return;
	}

	if(arg2[0] == '\0') {
		send_to_char("Give to who?\n\r", ch);
		return;
	}

        if ( ( ship = ship_in_room( ch->in_room, arg1 ) ) == NULL )
        {
           act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
           return;
        }

        if ( ( clan = get_clan( arg2 ) ) != NULL )
           ship->owner = STRALLOC( clan->name );
        else if ( ( victim = get_char_world( ch, arg2 ) ) != NULL )
           ship->owner = STRALLOC( victim->name );
        else
        {
           ch_printf( ch, "Give the ship to what!?\r\n" );
           return;
        }

        ch_printf( ch, "Ownership changed.\r\n" );
	save_ship(ship);

} //phel

void do_bounty( CHAR_DATA *ch, char *argument)
{
}

void do_bounties(CHAR_DATA *ch, char *argument)
{
}

// This is a command that will cloak a ship if it has a cloak module
// installed.
void do_cloak( CHAR_DATA * ch, char * argument )
{
	int slot, energy;
	bool hasCloakMod = FALSE;
	SHIP_DATA * ship, * vch;
	char buf[ MAX_STRING_LENGTH ];
	char arg[ MAX_INPUT_LENGTH ];

	strcpy( arg, argument );  

	switch ( ch->substate )
	{
		default:
			if ( IS_NPC( ch ) )
			{
				ch_printf( ch, "You can't do that.\r\n" );
				return;
			}			                        

			if ( ( ship = ship_from_pilotseat( ch->in_room ) ) == NULL )
			{
				ch_printf( ch, "&RYou must be at the controls of a ship to do that!\r\n" );
				return;
			}			

			if ( ship->shipstate == SHIP_DOCKED )
			{
				ch_printf( ch, "&RYou must launch first.\r\n" );
				return;
			}

			if ( autofly( ship ) )
			{
				ch_printf( ch, "&RYou have to turn autopilot off first.\r\n" );
				return;
			}
			
			if ( arg[0] == '\0' )
			{
				ch_printf( ch, "Usage: cloak <on/off>\r\n" );
				return;
			}

			for ( slot = 0; slot < MAX_MODS; slot++ )	
			{
				if ( ship->module[slot] == MODULE_CLOAK )
				{
					hasCloakMod = TRUE;
					break;
				}
			}

			if ( !hasCloakMod && !IS_IMMORTAL( ch ) )
			{
				ch_printf( ch, "&RYou don't have a cloak module installed.\r\n" );
				return;
			}			

			if ( number_percent() > ch->pcdata->learned[ gsn_weaponsystems ] )
			{
				ch_printf( ch, "&RYou fail to work the controls properly.\n\r" );

				learn_from_failure( ch, gsn_spacecraft );
				return;
			}

			if ( !str_cmp( arg, "on" ) )
			{
				if ( !IS_SET( ship->flags, SHIP_CLOAKED ) )
				{
					energy = 100 + ( ship->maxhull / 25 );

					if ( ( ship->energy - energy ) <= 1 )
					{
						ch_printf( ch, "&RYou do not have enough energy. " );
						return;
					}

					ship->energy -= energy;

					ch_printf( ch, "You press the button to begin cloaking.\r\n" );	
					act( AT_PLAIN, "$n presses some buttons on the control panel.", ch, NULL, NULL, TO_ROOM );

					echo_to_cockpit( AT_YELLOW , ship , "The ship begins powering up its cloaking module." );

					sprintf( buf, "%s begins to fade from view.", ship->name );
					echo_to_system( AT_YELLOW , ship , buf , NULL );
					
					add_timer ( ch, TIMER_DO_FUN, 5, do_cloak, 1 );
					ch->dest_buf = str_dup( arg );
				}
				else ch_printf( ch, "&RYour ship is already cloaked.\r\n" );				

				return;
			}			

			if ( !str_cmp( arg, "off" ) )
			{				
				if ( IS_SET( ship->flags, SHIP_CLOAKED ) )
				{
					ch_printf( ch, "You press the button to decloak.\r\n" );
					act( AT_PLAIN, "$n presses some buttons on the control pane.", ch, NULL, NULL, TO_ROOM );

					sprintf( buf, "The ship fades into view." );
					echo_to_cockpit( AT_YELLOW , ship , buf );

					sprintf( buf, "%s fades into view.", ship->name );
					echo_to_system( AT_YELLOW , ship , buf , NULL );

					REMOVE_BIT( ship->flags, SHIP_CLOAKED );
				}
				else ch_printf( ch, "&RYour ship cloak is already off.\r\n" );

				return;
			}

			do_cloak( ch, "" );
			return;				

		case 1:
    			if ( !ch->dest_buf )
    		   		return;

    			strcpy( arg, ch->dest_buf );
    			DISPOSE( ch->dest_buf );
    			break;
    		
    		case SUB_TIMER_DO_ABORT:
    			DISPOSE( ch->dest_buf );

    			ch->substate = SUB_NONE;

			ch_printf( ch, "&RYour concentration is broken, you fail to finish cloaking.\r\n" );
    			return;
	}

	ch->substate = SUB_NONE;

	if ( ( ship = ship_from_room( ch->in_room ) ) == NULL )
	{
		bug( "do_cloak: couldn't find the ship after the timer expired." );
		return;
	}

	SET_BIT( ship->flags, SHIP_CLOAKED );

	echo_to_cockpit( AT_YELLOW, ship, "The ship disappears from view." );

	sprintf( buf, "%s disappears from view.", ship->name );
	echo_to_system( AT_YELLOW, ship, buf, NULL );

	for ( vch = first_ship; vch; vch = vch->next )		
		if ( vch->target == ship )
			vch->target = NULL;

	ship->target = NULL;
	
	ship->energy += ship->shield;
	ship->shield = 0;
}

// This is a command that will load a ship onto a hangar.
// Does not take into consideration multiple hangars.
// USAGE: loadship <ship to load> <ship to load onto>
void do_loadship( CHAR_DATA * ch, char * args )
{
   SHIP_DATA *ship, *to;
   char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   // Check if there are no arguments.
   if ( args[0] == '\0' )
   {
      ch_printf( ch, "Load what onto what?\r\n" );
      return;
   }

   // Get the ship to load from the arguments
   args = one_argument( args, arg1 );
   // Get the ship to load onto from the arguments
   one_argument( args, arg2 );

   // Check if either argument is missing.
   if ( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      do_loadship( ch, "" );
      return;
   }

   // Check if the ship to load is in the room.
   if ( ( ship = ship_in_room( ch->in_room, arg1 ) ) == NULL )
   {
      ch_printf( ch, "Try loading a ship that is in the room.\r\n" );
      return;
   }

   // Check if the ship to load onto is in the room.
   if ( ( to = ship_in_room( ch->in_room, arg2 ) ) == NULL )
   {
      ch_printf( ch, "Try loading it onto a ship that is here.\r\n" );
      return;
   }

   // Check if the character is a pilot of the ship.
   if ( !check_ship_pilot( ch, ship ) )
   {
      ch_printf( ch, "That is not your ship.\r\n" );
      return;
   }

   // Check if the character is a pilot of the ship to that you are loading onto.
   if ( !check_ship_pilot( ch, to ) )
   {
      ch_printf( ch, "Try loading it onto a ship you pilot.\r\n" );
      return;
   }

   // Check if the ship you are loading onto has a hangar.
   if ( !to->first_hangar )
   {
      ch_printf( ch, "That ship doesn't have a hangar." );
      return;
   }

   // Check if the ship to be loaded is atleast 5 rooms smaller than the ship
   // that it will be loaded on.
   // This means that you can't loadship customs because it doesn't have
   // a set number of rooms.
   // USAGE: unloadship <ship to unload>
   // TODO: Have it determine size based off of hull and maybe rooms and include
   //       customs.
   if ( ( model[to->model].rooms - 5 ) < model[ship->model].rooms )
   {
      ch_printf( ch, "Your ship it to big.\r\n" );
      return;
   }

   // TODO: take into consideration ships with multiple hangars.

   // Removes the ship from its current room.
   extract_ship( ship );

   // Moves the ship to load into the first hangar on the ship to load onto.
   ship_to_hangar( ship, to->first_hangar->room );
      
   sprintf( buf, "Workers load %s onto %s.", ship->name, to->name );
   echo_to_room( AT_PLAIN, ch->in_room, buf );
   
   sprintf( buf, "%s is loaded into the hangar.", ship->name );
   echo_to_room( AT_PLAIN, to->first_hangar->room, buf );   
}

// This is a command that will unload a ship from a hangar.
void do_unloadship( CHAR_DATA * ch, char * args )
{
   SHIP_DATA * ship, * from;
   char arg1[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];   

   // Check if there are no arguments.
   if ( args[0] == '\0' )
   {
      ch_printf( ch, "Unload what ship?\r\n" );
      return;
   }

   // Check if the character is even on a ship and get the ship.
   if ( ( from = ship_from_room( ch->in_room ) ) == NULL )
   {
      ch_printf( ch, "You are not even on a ship.\r\n" );
      return;
   }

   // Get the name of the ship to unload from the arguments.
   args = one_argument( args, arg1 );
   
   // Check if the ship is in the room and get the ship.
   if ( ( ship = ship_in_room( ch->in_room, arg1 ) ) == NULL )
   {
      ch_printf( ch, "That ship isn't here." );
      return;
   }

   // Check if the character is a pilot of the ship.
   if ( !check_ship_pilot( ch, ship ) )
   {
      ch_printf( ch, "That is not your ship.\r\n" );
      return;
   }

   // Check if the last docked location is the current location to see if
   // it is docked. Also check the shipstate to see if its docked, there
   // might be a case when the shipstate is SHIP_DISABLED and it is docked
   // but i'm not sure.
   if ( from->lastdoc != from->location || from->shipstate != SHIP_DOCKED )
   {
      ch_printf( ch, "You don't seem to be docked right now.\r\n");
      return;
   }

   // Removes the ship from its current room.
   extract_ship( ship );

   // Move the ship to the docked location, should be a landing platform or shipyard.
   ship_to_room( ship, from->location );   
   
   sprintf( buf, "%s is unloaded from the hangar.", ship->name );
   echo_to_room( AT_PLAIN, ch->in_room, buf );
      
   sprintf( buf, "Workers unload %s from %s.", ship->name, from->name );
   echo_to_room( AT_PLAIN, from->in_room, buf );
}
