///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2010
//
//    This file is part of rootpwa
//
//    rootpwa is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    rootpwa is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with rootpwa. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------
// File and Version Information:
// $Rev::                             $: revision of last commit
// $Author::                          $: author of last commit
// $Date::                            $: date of last commit
//
// Description:
//      basic test program for particle and related classes
//
//
// Author List:
//      Boris Grube          TUM            (original author)
//
//
//-------------------------------------------------------------------------


#include "TVector3.h"

#include "utilities.h"
#include "particleDataTable.h"
#include "particle.h"


using namespace std;
using namespace rpwa;


int
main(int argc, char** argv)
{

  if (1) {
    // switch on debug output
    particleProperties::setDebug(true);
    particleDataTable::setDebug(true);
    particle::setDebug(true);
  }

  // test loading of particle data table
  particleDataTable& pdt = particleDataTable::instance();
  pdt.readFile();
  if (0)
    printInfo << "particle data table:" << endl
	      << pdt;

  // test filling of particle properties
  if (0) {
    particleProperties partProp;
    const string       partName = "pi";
    partProp.fillFromDataTable(partName);
    printInfo << "particle properties for '" << partName << "':" << endl
	      << partProp << endl;
    pdt.addEntry(partProp);
  }

  // test construction of particles
  if (0) {
    TVector3 mom;
    mom = TVector3(1, 2, 3);
    const particle p1("pi+", mom,  0);
    mom = TVector3(2, 3, 4);
    const particle p2("pi-", mom, -1);
    particle p3 = p2;
    p3.setName("X+");
    p3.setSpinProj(+1);
    printInfo << "created particles: " << endl
	      << p1 << endl
	      << p2 << endl
	      << p3 << endl;
  }

  // checking charge name handling
  if (1) {
    for (int i = -2; i < 3; ++i) {
      stringstream c;
      c << "pi";
      if (abs(i) > 1)
	c << abs(i);
      c << sign(i);
      //const particle p(c.str(), i);
      int q;
      const string n = particle::chargeFromName(c.str(), q);
      cout << c.str() << ": charge = " << q << ", name = " << n << endl;
      const particle p(c.str());
      cout << "name = " << p.name() << endl
	   << endl;
    }
  }

}
