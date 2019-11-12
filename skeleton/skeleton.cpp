#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/boost/graph/graph_traits_Polyhedron_3.h>
#include <CGAL/boost/graph/properties_Polyhedron_3.h>
#include <CGAL/extract_mean_curvature_flow_skeleton.h>
#include <CGAL/boost/graph/split_graph_into_polylines.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/mesh_segmentation.h>
#include <boost/foreach.hpp>

#include <fstream>

typedef CGAL::Simple_cartesian<double>                               Kernel;
typedef Kernel::Point_3                                              Point;
typedef CGAL::Polyhedron_3<Kernel, CGAL::Polyhedron_items_with_id_3> Polyhedron;
typedef boost::graph_traits<Polyhedron>::vertex_descriptor           vertex_descriptor;
typedef boost::graph_traits<Polyhedron>::halfedge_descriptor         halfedge_descriptor;
typedef boost::graph_traits<Polyhedron>::face_descriptor             face_descriptor;
typedef CGAL::Mean_curvature_flow_skeletonization<Polyhedron>        Skeletonization;
typedef Skeletonization::Skeleton                                    Skeleton;
typedef Skeleton::vertex_descriptor                                  Skeleton_vertex;
typedef Skeleton::edge_descriptor                                    Skeleton_edge;

// Property map associating a facet with an integer as id to an
// element in a vector stored internally
template<class ValueType>
struct Facet_with_id_pmap
    : public boost::put_get_helper<ValueType&,
             Facet_with_id_pmap<ValueType> >
{
    typedef face_descriptor key_type;
    typedef ValueType value_type;
    typedef value_type& reference;
    typedef boost::lvalue_property_map_tag category;
    Facet_with_id_pmap(
      std::vector<ValueType>& internal_vector
    ) : internal_vector(internal_vector) { }
    reference operator[](key_type key) const
    { return internal_vector[key->id()]; }
private:
    std::vector<ValueType>& internal_vector;
};

//only needed for the display of the skeleton as maximal polylines
struct Display_polylines{
  const Skeleton& skeleton;
  std::ofstream& out;
  int polyline_size;
  std::stringstream sstr;
  Display_polylines(const Skeleton& skeleton, std::ofstream& out)
    : skeleton(skeleton), out(out)
  {}
  void start_new_polyline(){
    polyline_size=0;
    sstr.str("");
    sstr.clear();
  }
  void add_node(Skeleton_vertex v){
    ++polyline_size;
    sstr << " " << skeleton[v].point;
  }
  void end_polyline()
  {
    out << polyline_size << sstr.str() << "\n";
  }
};

// This example extracts a medially centered skeleton from a given mesh.
int main(int argc, char* argv[])
{
  // argv[1] Input mesh in .off format
  // argv[2] Output skeleton in .cgal format
  // argv[3] Output point correspondances in .cgal format
  std::ifstream input((argc>1)?argv[1]:"data/elephant.off");
  Polyhedron tmesh;
  input >> tmesh;
  Skeleton skeleton;
  CGAL::extract_mean_curvature_flow_skeleton(tmesh, skeleton);
  std::cout << "Number of vertices of the skeleton: " << boost::num_vertices(skeleton) << "\n";
  std::cout << "Number of edges of the skeleton: " << boost::num_edges(skeleton) << "\n";
  
  // Output all the edges of the skeleton.
  std::ofstream output((argc>2)?argv[2]:"skel.cgal");
  Display_polylines display(skeleton,output);
  CGAL::split_graph_into_polylines(skeleton, display);
  output.close();

  // init the polyhedron simplex indices
  CGAL::set_halfedgeds_items_id(tmesh);

  //for each input vertex compute its distance to the skeleton
  std::vector<double> distances(num_vertices(tmesh));
  BOOST_FOREACH(Skeleton_vertex v, CGAL::make_range(vertices(skeleton)) )
  {
    const Point& skel_pt = skeleton[v].point;
    BOOST_FOREACH(vertex_descriptor mesh_v, skeleton[v].vertices)
    {
      const Point& mesh_pt = mesh_v->point();
      distances[mesh_v->id()] = std::sqrt(CGAL::squared_distance(skel_pt, mesh_pt));
    }
  }

  // create a property-map for sdf values
  std::vector<double> sdf_values( num_faces(tmesh) );
  Facet_with_id_pmap<double> sdf_property_map(sdf_values);

  // compute sdf values with skeleton
  BOOST_FOREACH(face_descriptor f, faces(tmesh))
  {
    double dist = 0;
    BOOST_FOREACH(halfedge_descriptor hd, halfedges_around_face(halfedge(f, tmesh), tmesh))
      dist += distances[target(hd, tmesh)->id()];
    sdf_property_map[f] = dist / 3.;
  }

  // post-process the sdf values
  CGAL::sdf_values_postprocessing(tmesh, sdf_property_map);

  // write distance map
  output.open((argc>3)?argv[3]:"dist.cgal");
  BOOST_FOREACH(face_descriptor f, faces(tmesh))
    output << sdf_property_map[f] << "\n";
  return 0;
}
