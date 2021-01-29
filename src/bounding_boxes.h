#pragma once

#include "cgal.h"
#include "grid.h"
#include <vector>
#include <boost/variant.hpp>

/*!
 * A set of bounding boxes that can be in double or precise coordinates
 * (i.e. BoundingBox coming from PolySet data vs. CGAL_Iso_cuboid_3 coming from
 * CGAL_Nef_polyhedron data).
 *
 * Best precision occurs when not mixing the two.
 */
class BoundingBoxes
{
public:
	// Union of the two types of bounding boxes.
	typedef boost::variant<CGAL_Iso_cuboid_3, BoundingBox> BoundingBoxoid;

	static CGAL_Iso_cuboid_3 bboxToIsoCuboid(const BoundingBox &bbox)
	{
		auto &min = bbox.min();
		auto &max = bbox.max();

		return CGAL_Iso_cuboid_3(NT3(min.x()), NT3(min.y()), NT3(min.z()), NT3(max.x()), NT3(max.y()),
														 NT3(max.z()));
	}

	static Eigen::Vector3d point3ToVector3d(const CGAL_Point_3 &pt)
	{
		return Eigen::Vector3d(to_double(pt.x()), to_double(pt.y()), to_double(pt.z()));
	}
	static BoundingBox isoCuboidToBbox(const CGAL_Iso_cuboid_3 &cuboid)
	{
		return BoundingBox(point3ToVector3d(cuboid.min()), point3ToVector3d(cuboid.max()));
	}

	void add(const BoundingBoxoid &x)
	{
		if (auto *bbox = boost::get<BoundingBox>(&x)) {
			bboxes.push_back(*bbox);
		}
		else if (auto *cuboid = boost::get<CGAL_Iso_cuboid_3>(&x)) {
			cuboids.push_back(*cuboid);
		}
		else {
			assert(!"Unknown bbox type");
		}
	}

	bool intersects(const BoundingBoxoid &x)
	{
		if (auto *bbox = boost::get<BoundingBox>(&x)) {
			auto extended = *bbox;
#ifdef DEFENSIVE_FAST_UNIONS
			// static Eigen::Vector3d margin(GRID_FINE, GRID_FINE, GRID_FINE);
			static Eigen::Vector3d margin(GRID_COARSE, GRID_COARSE, GRID_COARSE);
			extended.extend(bbox->min() - margin);
			extended.extend(bbox->max() + margin);
#endif
			return intersects_bboxes(extended) || (!extended.isEmpty() && cuboids.size() &&
																							intersects_cuboids(bboxToIsoCuboid(extended)));
		}
		else if (auto *cuboid = boost::get<CGAL_Iso_cuboid_3>(&x)) {
			auto extended = *cuboid;
#ifdef DEFENSIVE_FAST_UNIONS
			// static Eigen::Vector3d margin(GRID_FINE, GRID_FINE, GRID_FINE);
			static Eigen::Vector3d margin(GRID_COARSE, GRID_COARSE, GRID_COARSE);
			auto bbox = isoCuboidToBbox(extended);
			bbox.extend(bbox.min() - margin);
			bbox.extend(bbox.max() + margin);
			extended = bboxToIsoCuboid(bbox);
#endif
			return intersects_cuboids(extended) ||
						 (bboxes.size() && intersects_bboxes(isoCuboidToBbox(extended)));
		}
		else {
			assert(!"Unknown bbox type");
			return false;
		}
	}

private:
	bool intersects_bboxes(const BoundingBox &b)
	{
		for (auto &bbox : bboxes) {
			if (bbox.intersects(b)) {
				return true;
			}
		}
		return false;
	}

	bool intersects_cuboids(const CGAL_Iso_cuboid_3 &c)
	{
		for (auto &cuboid : cuboids) {
			if (CGAL::intersection(c, cuboid) != boost::none) {
				return true;
			}
		}
		return false;
	}

	std::vector<BoundingBox> bboxes;
	std::vector<CGAL_Iso_cuboid_3> cuboids;
};
