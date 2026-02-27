/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

static unsigned int securitycookie = 0;

void
set_securitycookie(unsigned int newsecuritycookie)
{
	securitycookie = newsecuritycookie;
}

unsigned int
get_securitycookie()
{
	return securitycookie;
}

/**
 *	Tests if the test security cookie equals the stored
 *	security cookie. Resets the stored security cookie,
 *	if the test fails.
 *	
 *	@returns: 1 if both security cookies match
 *			  0 if either the stored security cookie is 0
 *				or if both security cookies do not match
 */
unsigned char
check_securitycookie(unsigned int testsecuritycookie)
{
	if (securitycookie == 0) {
		return 0;
	} else if (securitycookie == testsecuritycookie) {
		return 1;
	} else {
		securitycookie = 0;
		return 0;
	}
}
