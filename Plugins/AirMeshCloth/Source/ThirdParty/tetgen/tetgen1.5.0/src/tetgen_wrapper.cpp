#include "tetgen.h"

#include "tetgen_wrapper.h"

#define SET_NULL_TO_IN_TETGENIO(tetgenio_member, inout_member) \
if (in.inout_member != nullptr) \
{ \
	in2.tetgenio_member = nullptr; \
}

#define SET_NULL_TO_OUT_TETGENIO(tetgenio_member, inout_member) \
out.inout_member = out2.tetgenio_member; \
out2.tetgenio_member = nullptr;

#define SET_NULL_TO_TETGENIO(tetgenio_member, inout_member) \
SET_NULL_TO_OUT_TETGENIO(tetgenio_member, inout_member)

tetgen_wrapper::int32 tetgen_wrapper::tetrahedralize(const char * switches, const input_output& in, input_output& out)
{
	tetgenio in2;
	tetgenio out2;

	// build input parameters for tetgen

	in2.firstnumber = 0;
	in2.mesh_dim = in.mesh_dimension;
	in2.useindex = 0;

	in2.pointlist = in.point_list;
	in2.pointattributelist = in.point_attribute_list;
	in2.pointmtrlist = in.point_mtr_list;
	in2.pointmarkerlist = in.point_marker_list;
	in2.numberofpoints = in.num_points;
	in2.numberofpointattributes = in.num_point_attributes;
	in2.numberofpointmtrs = in.num_point_mtrs;

	in2.tetrahedronlist = in.tetrahedron_list;
	in2.tetrahedronattributelist = in.tetrahedron_attribute_list;
	in2.tetrahedronvolumelist = in.tetrahedron_volume_list;
	in2.neighborlist = in.neighbor_list;
	in2.numberoftetrahedra = in.num_tetrahedra;
	in2.numberofcorners = in.num_corners;
	in2.numberoftetrahedronattributes = in.num_tetrahedron_attributes;

	in2.facetlist = reinterpret_cast<tetgenio::facet*>(in.facet_list);
	in2.facetmarkerlist = in.facet_marker_list;
	in2.numberoffacets = in.num_facets;

	in2.holelist = in.hole_list;
	in2.numberofholes = in.num_holes;

	int32 return_value = 0;
	
	try {
		// const_cast to adapt to tetgen interface
		::tetrahedralize(const_cast<char*>(switches), &in2, &out2);

#ifdef TETTEST
		::tetrahedralize(const_cast<char*>(switches), &in2, nullptr);
		in2.save_poly("tetgen-tmpfile");
#endif
	}
	catch (int32 excep_code)
	{
		// tetgen sends error code as an exception
		return_value = excep_code;
	}

	SET_NULL_TO_IN_TETGENIO(pointlist, point_list);
	SET_NULL_TO_IN_TETGENIO(pointattributelist, point_attribute_list);
	SET_NULL_TO_IN_TETGENIO(pointmtrlist, point_mtr_list);
	SET_NULL_TO_IN_TETGENIO(pointmarkerlist, point_marker_list);
	SET_NULL_TO_IN_TETGENIO(tetrahedronlist, tetrahedron_list);
	SET_NULL_TO_IN_TETGENIO(tetrahedronattributelist, tetrahedron_attribute_list);
	SET_NULL_TO_IN_TETGENIO(tetrahedronvolumelist, tetrahedron_volume_list);
	SET_NULL_TO_IN_TETGENIO(neighborlist, neighbor_list);
	SET_NULL_TO_IN_TETGENIO(facetmarkerlist, facet_marker_list);
	SET_NULL_TO_IN_TETGENIO(holelist, hole_list);
	SET_NULL_TO_IN_TETGENIO(facetlist, facet_list);

	if (return_value == 0)
	{
		// assign null to avoid deallocation at tetgenio::~tetgenio()

		SET_NULL_TO_TETGENIO(pointlist, point_list);
		SET_NULL_TO_TETGENIO(pointattributelist, point_attribute_list);
		SET_NULL_TO_TETGENIO(pointmtrlist, point_mtr_list);
		SET_NULL_TO_TETGENIO(pointmarkerlist, point_marker_list);
		SET_NULL_TO_TETGENIO(tetrahedronlist, tetrahedron_list);
		SET_NULL_TO_TETGENIO(tetrahedronattributelist, tetrahedron_attribute_list);
		SET_NULL_TO_TETGENIO(tetrahedronvolumelist, tetrahedron_volume_list);
		SET_NULL_TO_TETGENIO(neighborlist, neighbor_list);
		SET_NULL_TO_TETGENIO(facetmarkerlist, facet_marker_list);
		SET_NULL_TO_TETGENIO(holelist, hole_list);

		out.num_points = out2.numberofpoints;
		out.num_point_attributes = out2.numberofpointattributes;
		out.num_point_mtrs = out2.numberofpointmtrs;
		out.num_tetrahedra = out2.numberoftetrahedra;
		out.num_corners = out2.numberofcorners;
		out.num_tetrahedron_attributes = out2.numberoftetrahedronattributes;
		out.num_facets = out2.numberoffacets;
		out.num_holes = out2.numberofholes;

		out.facet_list = reinterpret_cast<facet*>(out2.facetlist);
		out2.facetlist = nullptr;

		out.automatic_deallocation = true;
	}

	return return_value;
}

tetgen_wrapper::input_output::~input_output()
{
	if (automatic_deallocation)
	{
		delete[] point_list;
		delete[] point_attribute_list;
		delete[] point_mtr_list;
		delete[] point_marker_list;
		delete[] tetrahedron_list;
		delete[] tetrahedron_attribute_list;
		delete[] tetrahedron_volume_list;
		delete[] neighbor_list;
		delete[] hole_list;
		if (facet_list != nullptr)
		{
			for (int32 i = 0; i < num_facets; i++)
			{
				auto& f = facet_list[i];
				for (int32 j = 0; j < f.num_polygons; j++)
				{
					auto& p = f.polygon_list[j];
					delete[] p.vertex_list;
				}
				delete[] f.polygon_list;
				delete[] f.hole_list;
			}
			delete[] facet_list;
		}
	}
}

#ifdef TETTEST

#include <vector>

// testcode
int main()
{
	namespace tw = tetgen_wrapper;

	// generate vertices
	std::vector<double> points
	{
#if 0
		0.0, 0.0, 0.0,
		1.0, 0.1, 0.0,
		2.0, 0.2, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.1, 0.0,
		2.0, 1.2, 0.0,
		0.0, 2.0, 0.0,
		1.0, 2.1, 0.0,
		2.0, 2.2, 0.0,

		0.0, 0.0 / 3.0, 1.0,
		1.0, 0.0 / 3.0, 1.0,
		2.0, 0.0 / 3.0, 1.0,
		0.0, 2.0 / 3.0, 1.0,
		1.0, 2.0 / 3.0, 1.0,
		2.0, 2.0 / 3.0, 1.0,
		0.0, 4.0 / 3.0, 1.0,
		1.0, 4.0 / 3.0, 1.0,
		2.0, 4.0 / 3.0, 1.0,

		-2.0, -0.3,  0.5,
		 1.0, -0.3, -1.5,
		-2.0,  2.3,  0.5,
		 1.0,  2.3, -1.5,
		 1.0, -0.3,  2.5,
		 3.0, -0.3,  0.5,
		 1.0,  2.3,  2.5,
		 3.0,  2.3,  0.5,

		0.0, 2.0, 1.0,
		1.0, 2.0, 1.0,
		2.0, 2.0, 1.0,
#else
		-419.99978637695313, 330.00021362304688, 316.00158691406250, // 0
		-420.00000000000000, 280.00021362304688, 316.00158691406250, // 1
		-420.00021362304688, 230.00021362304688, 316.00158691406250, // 2
		-369.99978637695313, 330.00000000000000, 316.00158691406250, // 3
		-370.00000000000000, 280.00000000000000, 316.00158691406250, // 4
		-370.00021362304688, 230.00000000000000, 316.00158691406250, // 5
		-319.99978637695313, 329.99978637695313, 316.00158691406250, // 6
		-320.00000000000000, 279.99978637695313, 316.00158691406250, // 7
		-320.00021362304688, 229.99980163574219, 316.00158691406250, // 8
		-409.99978637695313, 330.00015258789063, 316.00158691406250, // 9
		-410.00000000000000, 280.00018310546875, 316.00158691406250, // 10
		-410.00021362304688, 230.00016784667969, 316.00158691406250, // 11
		-359.99978637695313, 329.99993896484375, 316.00158691406250, // 12
		-360.00000000000000, 279.99996948242188, 316.00158691406250, // 13
		-360.00021362304688, 229.99996948242188, 316.00158691406250, // 14
		-309.99978637695313, 329.99975585937500, 316.00158691406250, // 15
		-310.00000000000000, 279.99975585937500, 316.00158691406250,
		-310.00021362304688, 229.99975585937500, 316.00158691406250,
		-399.99978637695313, 330.00012207031250, 316.00158691406250,
		-400.00000000000000, 280.00012207031250, 316.00158691406250,
		-400.00021362304688, 230.00012207031250, 316.00158691406250,
		-349.99978637695313, 329.99990844726563, 316.00158691406250,
		-350.00000000000000, 279.99990844726563, 316.00158691406250,
		-350.00021362304688, 229.99992370605469, 316.00158691406250,
		-299.99978637695313, 329.99969482421875, 316.00158691406250,
		-300.00000000000000, 279.99969482421875, 316.00158691406250,
		-300.00021362304688, 229.99971008300781, 316.00158691406250,
		-421.20022583007813, 228.79971313476563, 314.80157470703125,
		-298.79977416992188, 228.79971313476563, 314.80157470703125,
		-421.20022583007813, 331.20022583007813, 314.80157470703125,
		-298.79977416992188, 331.20022583007813, 314.80157470703125,
		-421.20022583007813, 228.79971313476563, 317.20159912109375,
		-298.79977416992188, 228.79971313476563, 317.20159912109375,
		-421.20022583007813, 331.20022583007813, 317.20159912109375,
		-298.79977416992188, 331.20022583007813, 317.20159912109375,
#endif
	};
	//assert(points.size() == 29 * 3 && "# points must be 29 (test case input)");
	assert(points.size() == 35 * 3 && "# points must be 35 (test case input)");

	// generate facets
	struct orig_facet
	{
		tw::int32 num_vertices;
		tw::int32 vertex_list[4];
	};

	std::vector<orig_facet> orig_facets
	{
#if 0
		{ 3, 0, 1, 4 },
		{ 3, 0, 4, 3 },
		{ 3, 1, 2, 5 },
		{ 3, 1, 5, 4 },
		{ 3, 3, 4, 7 },
		{ 3, 3, 7, 6 },
		{ 3, 4, 5, 8 },
		{ 3, 4, 8, 7 },
		{ 3, 9, 10, 13 },
		{ 3, 9, 13, 12 },
		{ 3, 10, 11, 14 },
		{ 3, 10, 14, 13 },
		{ 3, 12, 13, 16 },
		{ 3, 12, 16, 15 },
		{ 3, 13, 14, 17 },
		{ 3, 13, 17, 16 },
		{ 3, 15, 16, 27 },
		{ 3, 15, 27, 26 },
		{ 3, 16, 17, 28 },
		{ 3, 16, 28, 27 },
		{ 4, 18, 19, 21, 20 },
		{ 4, 19, 23, 25, 21 },
		{ 4, 23, 22, 24, 25 },
		{ 4, 22, 18, 20, 24 },
		{ 4, 22, 23, 19, 18 },
		{ 4, 20, 21, 25, 24 },
#else
		{ 3, 0, 1, 3 },
		{ 3, 1, 4, 3 },
		{ 3, 1, 2, 4 },
		{ 3, 2, 5, 4 },
		{ 3, 3, 4, 6 },
		{ 3, 4, 7, 6 },
		{ 3, 4, 5, 7 },
		{ 3, 5, 8, 7 },
		{ 3, 9, 10, 12 },
		{ 3, 10, 13, 12 },
		{ 3, 10, 11, 13 },
		{ 3, 11, 14, 13 },
		{ 3, 12, 13, 15 },
		{ 3, 13, 16, 15 },
		{ 3, 13, 14, 16 },
		{ 3, 14, 17, 16 },
		{ 3, 18, 19, 21 },
		{ 3, 19, 22, 21 },
		{ 3, 19, 20, 22 },
		{ 3, 20, 23, 22 },
		{ 3, 21, 22, 24 },
		{ 3, 22, 25, 24 },
		{ 3, 22, 23, 25 },
		{ 3, 23, 26, 25 },
		{ 4, 27, 28, 30, 29 },
		{ 4, 28, 32, 34, 30 },
		{ 4, 32, 31, 33, 34 },
		{ 4, 31, 27, 29, 33 },
		{ 4, 31, 32, 28, 27 },
		{ 4, 29, 30, 34, 33 },

#endif
	};
	//assert(orig_facets.size() == 26 && "# facets must be 25 (test case input)");
	assert(orig_facets.size() == 30 && "# facets must be 30 (test case input)");

	std::vector<tw::polygon> polygons;
	polygons.reserve(orig_facets.size());
	for (auto& f : orig_facets)
	{
		polygons.push_back(tw::polygon{ f.vertex_list, f.num_vertices });
	}

	std::vector<tw::facet> facets;
	facets.reserve(orig_facets.size());
	for (auto& p : polygons)
	{
		facets.emplace_back(tw::facet{ &p, 1, nullptr, 0 });
	}

	// build input

	tw::input_output in;

	in.automatic_deallocation = false;

	in.point_list = points.data();
	in.num_points = (tw::int32)points.size() / 3;

	in.facet_list = facets.data();
	in.num_facets = (tw::int32)facets.size();
	
	for (tw::int32 i = 0; i < 10; i++)
	{
		tw::input_output out;
		tw::tetrahedralize("", in, out);

		assert(out.num_points == (tw::int32)points.size() / 3 && "# points are not preserved");
		//assert(out.num_tetrahedra == 110 && "# tetrahedra doesn't match (110 expected)");
		assert(out.num_tetrahedra == 116 && "# tetrahedra doesn't match (116 expected)");
	}

	{
		tw::input_output out;
		tw::tetrahedralize("", in, out);
	}

	return EXIT_SUCCESS;
}

#endif
