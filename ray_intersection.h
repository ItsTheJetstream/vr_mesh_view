#pragma once
#include <cgv\math\fvec.h>
#include <cgv/media/axis_aligned_box.h>

#include "halfedgemesh.h"
#include "mesh_utils.h"
#include "aabb_tree.h"

typedef typename cgv::math::fvec<float, 3> vec3;
typedef typename cgv::media::axis_aligned_box<float, 3> box3;

namespace ray_intersection {
	struct ray {
		vec3 origin;
		vec3 direction;
	};
	bool isInsideTriangle(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& p)
	{
		vec3 edge0 = v1 - v0;
		vec3 edge1 = v2 - v1;
		vec3 edge2 = v0 - v2;
		vec3 C0 = p - v0;
		vec3 C1 = p - v1;
		vec3 C2 = p - v2;
		vec3 N = cross(edge0, edge1);
		if (dot(N,cross(edge0,C0)) > 0 &&
			dot(N, cross(edge1, C1)) > 0 &&
			dot(N, cross(edge2, C2)) > 0) return true; // P is inside the triangle 
		return false;
	}
	//t value gives the distance multiplier 
	bool rayTriangleIntersect(const ray& r, const vec3& v0, const vec3& v1, const vec3& v2, float& t)
	{
		vec3 d = r.direction;
		vec3 o = r.origin;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;
		vec3 p = cross(d, e2);
		float a = dot(e1, p);

		if (a == 0)
			return false;

		float f = 1.0f / a;

		vec3 s = o - v0;
		float u = f * dot(s, p);
		if (u < 0.0f || u > 1.0f)
			return false;

		vec3 q = cross(s, e1);
		float v = f * dot(d, q);

		if (v < 0.0f || u + v > 1.0f)
			return false;

		t = f * dot(e2, q);

		return (t >= 0);
	}
	
	bool rayFaceIntersect(ray& r, HE_Mesh* mesh, HE_Face* face, float& t)
	{
		vec3 v0, v1, v2;
		mesh_utils::getVerticesOfFace(mesh, face, v0, v1, v2);
		if (rayTriangleIntersect(r, v0, v1, v2, t)) {
			return true;
		}
			
		return false;
	}
	
	bool rayMeshIntersect(ray& r, HE_Mesh* mesh, float& t)
	{
		vec3 v0, v1, v2;
		bool intersect = false;
		float temp_t = 1000000;

		for (auto face : *mesh->GetFaces()) {
			if (rayFaceIntersect(r, mesh, face, t))
			{
				intersect = true;
				if (t < temp_t)
					temp_t = t;
			}
		}
		
		if (!intersect)
			return false;
		else {
			t = temp_t;
			return true;
		}
	}
	HE_Face* getIntersectedFace(ray& r, HE_Mesh* mesh)
	{
		vec3 v0, v1, v2;
		bool intersect = false;
		float temp_t = 1000000;
		float t = 0;

		HE_Face* temp_face = new HE_Face();

		for (auto face : *mesh->GetFaces()) {
			if (rayFaceIntersect(r, mesh, face, t))
			{
				intersect = true;
				if (t < temp_t) {
					temp_t = t;
					temp_face = face;
				}		
			}
		}
		if (intersect)
			return temp_face;
		else {
			std::cout << "No intersection, returning face object anyway" << std::endl;
				return temp_face;
		}
	}
	
	bool rayBoxIntersect(const ray& r, const box3& b)
	{
		vec3 max_pnt = b.get_max_pnt();
		vec3 min_pnt = b.get_min_pnt();
		vec3 dir = r.direction;
		vec3 orig = r.origin;

		float tmin = (min_pnt.x() - orig.x()) / dir.x();
		float tmax = (max_pnt.x() - orig.x()) / dir.x();

		if (tmin > tmax) {float temp = tmin;tmin = tmax;tmax = temp;}

		float tymin = (min_pnt.y() - orig.y()) / dir.y();
		float tymax = (max_pnt.y() - orig.y()) / dir.y();

		if (tymin > tymax) {float temp = tymin;tymin = tymax;tymax = temp;}

		if ((tmin > tymax) || (tymin > tmax))
			return false;

		if (tymin > tmin)
			tmin = tymin;

		if (tymax < tmax)
			tmax = tymax;

		float tzmin = (min_pnt.z() - orig.z()) / dir.z();
		float tzmax = (max_pnt.z() - orig.z()) / dir.z();

		if (tzmin > tzmax) {float temp = tzmin; tzmin = tzmax; tzmax = temp;}

		if ((tmin > tzmax) || (tzmin > tmax))
			return false;

		if (tzmin > tmin)
			tmin = tzmin;

		if (tzmax < tmax)
			tmax = tzmax;

		return true;
	}

	
	void rayNodeIntersect(const ray& r, AabbTree<triangle>::AabbNode* node, float& t)
	{
		box3 box = node->get_box();
		float temp = t;
		if (rayBoxIntersect(r, box)) {
			if (node->is_leaf()) {
				float temp2 = temp;
				std::vector<vec3> triangle = node->get_triangle();
				if (rayTriangleIntersect(r, triangle.at(0), triangle.at(1), triangle.at(2), temp2))
					if (temp2 != 0)
						if (temp == 0 || temp2 < temp){
							std::cout << "Changed from: " << temp << " to " << temp2 << std::endl;
							temp = temp2;
						}	
			}
			else {
				rayNodeIntersect(r, node->left_child(), temp);
				rayNodeIntersect(r, node->right_child(), temp);
			}
		}
		t = temp;
	}
	
	bool rayTreeIntersect(const ray& r, AabbTree<triangle>& tree, float& t)
	{
		AabbTree<triangle>::AabbNode* rootNode = tree.Root();
		std::cout << "Before ray tree intersect: " << t << std::endl;
		rayNodeIntersect(r, rootNode, t);
		std::cout << "After ray tree intersect: " << t << std::endl;
		if (t > 0) return true;
		else return false;
	}
	
	vec3 getIntersectionPoint(const ray& r, const float& t)
	{
		return r.origin + t * r.direction;
	}
}