//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package slaw.mobility;

import inet.mobility.base.MobilityBase;

//
// This mobility module sets initial position according to the SLAW model
//
// @author A. G. Medrano-Chávez
//
simple SlawStationaryMobility extends MobilityBase {
    parameters:
        string map = "map";
        string walkerModelType @enum("SlawMatlab", "SlawTransNetw");
        double clusterRatio = default(1.0);
        @class(SlawStationaryMobility);
}

