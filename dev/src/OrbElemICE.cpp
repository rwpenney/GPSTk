#pragma ident "$Id$"

//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//
//  Copyright 2004, The University of Texas at Austin
//
//============================================================================

//============================================================================
//
//This software developed by Applied Research Laboratories at the University of
//Texas at Austin, under contract to an agency or agencies within the U.S.
//Department of Defense. The U.S. Government retains all rights to use,
//duplicate, distribute, disclose, or release this software.
//
//Pursuant to DoD Directive 523024
//
// DISTRIBUTION STATEMENT A: This software has been approved for public
//                           release, distribution is unlimited.
//
//=============================================================================

/**
 * @file OrbElemICE.cpp
 * OrbElemCNAV2 data encapsulated in engineering terms
 */
#include <iomanip>
#include <cmath>

#include "StringUtils.hpp"
#include "GNSSconstants.hpp"
#include "GPSWeekSecond.hpp"
#include "GPS_URA.hpp"
#include "SVNumXRef.hpp"
#include "TimeString.hpp"
#include "OrbElemICE.hpp"


namespace gpstk
{
   using namespace std;

   OrbElemICE::OrbElemICE()
      :OrbElem(),
       URAed(0), URAned0(0), URAned1(0),
       IntegrityStatusFlag(false),
       ctTop(CommonTime::BEGINNING_OF_TIME),
       transmitTime(CommonTime::BEGINNING_OF_TIME)
   {
     ctTop.setTimeSystem(TimeSystem::GPS);
     transmitTime.setTimeSystem(TimeSystem::GPS);    
   }

      // See IS-GPS-200 30.3.3.1.1.4
      //     IS-GPS-705 20.3.3.1.1.4
      //     IS-GPS-800 3.5.3.5
   double OrbElemICE::getAdjNomURAed(const double elevation) const
      throw( InvalidRequest )
   {
      if (!dataLoaded())
      {
         InvalidRequest exc("Required data not stored.");
         GPSTK_THROW(exc);
      }
    
      double nomURA = ura2CNAVNominalaccuracy( URAed );
      double elvR = (elevation+90.0)*PI/180.0;
      double acc = nomURA*std::sin(elvR);

      return acc; 
   }

      // See IS-GPS-800 3.5.3.8
   double OrbElemICE::getIAURAned(const CommonTime& t) const
      throw( InvalidRequest )
   {
      if (!dataLoaded())
      {
         InvalidRequest exc("Required data not stored.");
         GPSTK_THROW(exc);
      }

      double ned0 = ura2CNAVaccuracy( URAned0 );
      double ned1 = 1.0/std::pow(2.0, (14.0 + URAned1));
      double ned2 = 1.0/std::pow(2.0, (28.0 + URAned2));
      double deltaT = t - ctTop;
      if(deltaT <= 93600)
         return ( ned0 + ned1 * deltaT);
      else
         return ned0 + ned1 * deltaT + ned2 * std::pow((deltaT - 93600), 2.0);          
   } 

      // See IS-GPS-200 30.3.3.1.1.4
      //     IS-GPS-705 20.3.3.1.1.4
      //     IS-GPS-800 3.5.3.5
    double OrbElemICE::getAdjIAURAed(const double elevation) const
      throw( InvalidRequest )
   {
      if (!dataLoaded())
      {
         InvalidRequest exc("Required data not stored.");
         GPSTK_THROW(exc);
      }
    
      double max = ura2CNAVaccuracy( URAed );
      double elvR = (elevation+90.0)*PI/180.0;
      double acc = max*std::sin(elvR);

      return acc; 
   } 

      // See IS-GPS-200 30.3.3.1.1, Paragraph 5-6 
   double OrbElemICE::getCompositeIAURA(const CommonTime& t, const double elevation) const
      throw( InvalidRequest)
   {
      double ed = getAdjIAURAed(elevation);
      double ned = getIAURAned(t);
      double value = std::sqrt(ed * ed + ned * ned);
      return value;
   }

   void OrbElemICE::dumpHeader(ostream& s) const
      throw( InvalidRequest )
   {
      ios::fmtflags oldFlags = s.flags();
       
      SVNumXRef svNumXRef; 
      int NAVSTARNum = 0; 

      s << endl;
      s << "PRN : " << setw(2) << satID.id << " / "
        << "SVN : " << setw(2);
      try
      {
	NAVSTARNum = svNumXRef.getNAVSTAR(satID.id, ctToe );
        s << NAVSTARNum << "  ";
      }
      catch(SVNumXRef::NoNAVSTARNumberFound)
      { 
	s << "XX";
      }  
       
      s << endl
        << endl
        << "           ACCURACY PARAMETERS"
        << endl
        << endl    
        << "ED accuracy index              :      " << setfill(' ')
        << setw(4) << dec << URAed << endl
        << "NED accuracy index             :      " << setfill(' ')
        << setw(4) << dec << URAned0 << endl
        << "NED accuracy change index      :      " << setfill(' ')
        << setw(4) << dec << URAned1 << endl
        << "NED accuracy change rate index :      " << setfill(' ')
        << setw(4) << dec << URAned2;
     
    
      s.setf(ios::fixed, ios::floatfield);
      s.setf(ios::right, ios::adjustfield);
      s.setf(ios::uppercase);
      s.precision(0);
      s.fill(' ');
 
      s << endl
        << endl;
      s << "              Week(10bt)  SOW      DOW     UTD   SOD"
        << "     MM/DD/YYYY   HH:MM:SS\n"; 
      s << "Predict    :  ";
      timeDisplay(s, ctTop);
      s << endl;
      s.flags(oldFlags);                
   } // end of dumpHeader()   

   void OrbElemICE::dumpTerse(ostream& s) const
      throw(InvalidRequest )
   {} // end of dumpTerse() 

   ostream& operator<<(ostream& s, const OrbElemICE& eph)
   {
      try
      {
         eph.dump(s);
      }
      catch(gpstk::Exception& ex)
      {
         GPSTK_RETHROW(ex);
      }
      return s;

   } // end of operator<<

} // end namespace
