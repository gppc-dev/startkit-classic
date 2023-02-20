/*
 * $Id: timer.cpp,v 1.6 2006/11/30 20:33:25 nathanst Exp $
 *
 * timer.cpp
 * HOG file
 * 
 * Written by Renee Jansen on 08/28/06
 *
 * This file is part of HOG.
 *
 * HOG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * HOG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with HOG; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//#include "UnitSimulation.h"
#include "Timer.h"
#include <stdint.h>
#include <cstring>

Timer::Timer()
{
	elapsedTime = duration::zero();
}

void Timer::StartTimer()
{
	startTime = clock::now();
}

#ifdef linux

float Timer::getCPUSpeed()
{
	FILE *f;

	static float answer = -1;
	
	if (answer != -1)
		return answer;
	
	f = std::fopen("/proc/cpuinfo", "r");
	if (f)
	{
		while (!feof(f))
		{
			char entry[1024];
			char temp[1024];
			std::fgets(entry, 1024, f);
			if (std::strstr(entry, "cpu MHz"))
			{
				//                              cpu MHz         : 997.399
				float answer;
				std::sscanf(entry, "%[^0-9:] : %f", temp, &answer);
				//printf("Read CPU speed: %1.2f\n", answer);
				std::fclose(f);
				return answer;
			}
		}
		std::fclose(f);
	}
	return 0;
}

#else

float Timer::getCPUSpeed()
{
	return 0;
}

#endif

Timer::duration Timer::EndTimer()
{
	clock::time_point stopTime = clock::now();
	
	elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime);
	return elapsedTime;
}
