/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utilities.h"

std::vector<std::string> splitString(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;

    if (delim.empty())
        result.push_back(str);
    else
    {
      std::string::const_iterator substart = str.begin(), subend;

      while (true)
      {
        subend = search(substart, str.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);
        result.push_back(temp);
        if (subend == str.end())
          break;

        substart = subend + delim.size();
      }
    }
    return result;
}

std::string dayOfWeekToString(const int& days_of_week, const std::vector<std::string>& days)
{
  std::string result;
  result = days.size() > 0 ? days[0] : "";

  if (days_of_week == 0x7F && days.size() > 8)
    result.append(days[8]);
  else
  {
    for(int i = 0;i < 7;i++)
    {
      if (((days_of_week & (1 << i)) != 0) && days.size() > (i+1))
        result.append(days[i+1]);
    }

    if (result.size() > 0)
      result.resize(result.size()-1);
  }
  return result;
}

