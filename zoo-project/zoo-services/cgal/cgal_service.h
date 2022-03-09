/**
 * Author : Gérald FENOY
 *
 * Copyright 2009-2013 GeoLabs SARL. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZOO_CGAL_SERVICE_H
#define ZOO_CGAL_SERVICE_H

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <vector>

#include "cpl_minixml.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "service.h"
#include "service_internal.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel            Kernel;
typedef Kernel::Point_2                                                Pointz;

int parseInput(maps*,maps*, std::vector<Pointz>*,char*);

#endif

