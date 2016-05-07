// 

#pragma once

#include <cstdint>

namespace tetgen_wrapper
{

using int32 = std::int32_t;

// same layout with tetgenio::polygon
struct polygon
{
	int32* vertex_list;
	int32 num_vertices;
};

// same layout with tetgenio::facet
struct facet
{
	polygon* polygon_list;
	int32 num_polygons;
	double* hole_list;
	int32 num_holes;
};

struct input_output
{
	int32 mesh_dimension = 3;

	// points
		
	double* point_list = nullptr;
	double* point_attribute_list = nullptr;
	double* point_mtr_list = nullptr;
	int32* point_marker_list = nullptr;
		
	int32 num_points = 0;
	int32 num_point_attributes = 0;
	int32 num_point_mtrs = 0;

	// tetrahedra

	int32* tetrahedron_list = nullptr;
	double* tetrahedron_attribute_list = nullptr;
	double* tetrahedron_volume_list = nullptr;
	int32* neighbor_list = nullptr;

	int32 num_tetrahedra = 0;
	int32 num_corners = 0;
	int32 num_tetrahedron_attributes = 0;

	// facets

	facet* facet_list = nullptr;
	int32* facet_marker_list = nullptr;
	int32 num_facets = 0;

	// holes

	double* hole_list = nullptr;
	int32 num_holes = 0;

	// require deallocation at destructor or not
	bool automatic_deallocation = true;

	~input_output();
};

/*
* Wrapper for original tetgen's tetrahedralize()
*
* @param switches Command line switches of tetgen (examples: "p", "q", "Y", "pq", ...)
* @param input Input parameters
* @param output Results
* @return Result code, 0 indicates success, otherwise failure
*/
int32 tetrahedralize(const char* switches, const input_output& in, input_output& out);

}
