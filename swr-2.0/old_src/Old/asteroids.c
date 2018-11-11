/*Released under the BSD license.   */
/*Look it up yourself, you lazy bum.*/
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mud.h"

void do_release(CHAR_DATA *ch, char *argument) {
	ASTEROID_DATA *asteroid;
	SHIP_DATA *ship;	
        char buf[MAX_STRING_LENGTH];

	if((ship = ship_from_pilotseat(ch->in_room))  == NULL) {
		send_to_char("&RYou must be at the helm of a ship to do that!\n\r", ch);
		return;
	}

	if (ship->starsystem == NULL) {
		send_to_char("&RYou can only do that in realspace.\n\r",ch);
		return;
	}

	if (ship->shipstate == SHIP_DOCKED) {
		send_to_char("&RYou can't do that until after you've launched!\n\r", ch);
		return;
	}

	if(ship->towing == NULL) {
		send_to_char("&RYou're not towing anything!\n\r", ch);
		return;
	}

	asteroid = ship->towing;
	asteroid->towedby = NULL;
	ship->towing = NULL;
	sprintf(buf ,"%s releases %s." , ship->name, asteroid->name);
	echo_to_system(AT_YELLOW, ship, buf , NULL);
	send_to_char("&RReleased.\n\r", ch);
	return;
}

void do_haul(CHAR_DATA *ch, char *argument) {
	ASTEROID_DATA *asteroid;
	SHIP_DATA *ship;	
	int range;
	char buf[MAX_STRING_LENGTH];

	if((ship = ship_from_pilotseat(ch->in_room))  == NULL) {
		send_to_char("&RYou must be at the helm of a ship to do that!\n\r", ch);
		return;
	}

	if (ship->shipstate == SHIP_HYPERSPACE) {
		send_to_char("&RYou can only do that in realspace!\n\r", ch);
		return;
	}

	if (ship->starsystem == NULL) {
		send_to_char("&RYou can only do that in realspace.\n\r",ch);
		return;
	}

	if (ship->shipstate == SHIP_DOCKED) {
		send_to_char("&RYou can't do that until after you've launched!\n\r", ch);
		return;
	}

	if(ship->towing != NULL) {
		send_to_char("&RYou're already towing something!\n\r", ch);
		return;
	}

	if(argument[0] == '\0') {
		send_to_char("&RHaul what?\n\r", ch);
		return;
	}

	if((asteroid = get_asteroid_here(argument, ship->starsystem)) != NULL) {
		range = find_range(asteroid->x, asteroid->y, asteroid->z, ship->vx, ship->vy, ship->vz);
		if(range <= 300 && range >= 0) {
			if(asteroid->towedby) {
				send_to_char("&RSomeone's already towing that.\n\r", ch);
				return;
			}
			ch_printf(ch, "A bright blue flash of light emits from your ship as %s begins following you\n\r", asteroid->name);
			asteroid->x = ship->vx;
			asteroid->y = ship->vy;
			asteroid->z = ship->vz;
			ship->towing = asteroid;
			asteroid->towedby = ship;
			sprintf(buf ,"%s emits a bright flash of light as %s begins moveing along with %s." , ship->name, asteroid->name, ship->name);
			echo_to_system(AT_YELLOW, ship, buf , NULL);
		} else {
			send_to_char("That asteroid is too far away.\n\r", ch);
		}
	} else {
		send_to_char("That asteroid isn't here.\n\r", ch);
	}
}

void make_asteroid(SPACE_DATA *system) {
	ASTEROID_DATA *asteroid;
        ASTEROID_DATA* ast;
	int range, max_coord;
	char name[MAX_STRING_LENGTH];
	max_coord=10000;
        int num = 0;

	if(system==NULL) {
		bug("Null system during asteroid generation.\n\r");
		return;
        }

        /**
         * Get the number of asteroids, probably should keep up with
         * this in a variable in <code>SYSTEM_DATA</code instead.
         */
        for (ast = system->first_roid; ast; ast = ast->next)
        {
            num++;
        }

        /**
         * If there are too many asteroids in the system don't make one.
         * This probably should be handled in reset.c because this is just
         * supposed to make an asteroid, and the gameplay design should be
         * handled elsewhere.
         */
        if (num > MAX_ASTEROID)
        {
            return;
        }

	CREATE( asteroid, ASTEROID_DATA, 1 );

	asteroid->system = system;
	asteroid->next = NULL;
	asteroid->prev = NULL;
	asteroid->towedby = NULL;

	do {
		asteroid->x = number_range(max_coord*-1,max_coord);
	} while(asteroid->x > 10000);

	do {
		asteroid->y = number_range(max_coord*-1,max_coord);
	} while(asteroid->y > 10000);

	do {
		asteroid->z = number_range(max_coord*-1,max_coord);
	} while(asteroid->z > 10000);

	range = find_range(asteroid->x,asteroid->y,asteroid->z,0,0,0);

	asteroid->value = range * 10;
	sprintf(name, "Asteroid %i %c", asteroid->value, '\0');
	asteroid->name=STRALLOC(name);

	if(!system->first_roid) {
		system->first_roid = asteroid;
	}

        if(system->last_roid) {
		system->last_roid->next = asteroid;
		asteroid->prev = system->last_roid;
	}

	system->last_roid = asteroid;	
}

void sell_asteroid(SHIP_DATA *ship, CHAR_DATA *ch) {
	ASTEROID_DATA *asteroid;

	asteroid = ship->towing;

	if(ship->starsystem->first_roid == asteroid) {
		ship->starsystem->first_roid = asteroid->next;
	} else {
		asteroid->prev->next = asteroid->next;
	}

	if(ship->starsystem->last_roid == asteroid) {
		ship->starsystem->last_roid = asteroid->prev;
	} else {
		asteroid->next->prev = asteroid->prev;
	}

	ch->gold += asteroid->value;
	ch_printf(ch, "You made %d credits selling that asteroid.\n\r", ship->towing->value);   
	ship->towing = NULL;
	asteroid->towedby = NULL;
	DISPOSE(asteroid);	
}
