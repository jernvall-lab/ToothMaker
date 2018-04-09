#pragma once

/**
 *  @class Mesh
 *  @brief Representation of 2-manifold mesh.
 *
 *  Supports triangular, quad and mixed triangular-quad meshes. Vertices can be
 *  assigned two sets of color values (colors, alt_colors) and any number of
 *  properties ('property' is an array of values for the mesh vertices, for
 *  example morphogen concentrations).
 */

#include <vector>
#include <algorithm>
#include <functional>
#include <stdint.h>

struct vertex_color {
    float r, g, b, a;
};


struct vertex {
    float x, y, z;

    vertex operator=(const vertex& w) {
        x = w.x;
        y = w.y;
        z = w.z;
        return *this;
    }

    vertex operator-(const vertex& w) const {
        vertex v;
        v.x = x - w.x;
        v.y = y - w.y;
        v.z = z - w.z;
        return v;
   }

    vertex operator+(const vertex& w) const {
        vertex v;
        v.x = x + w.x;
        v.y = y + w.y;
        v.z = z + w.z;
        return v;
    }
};


typedef std::vector<uint32_t>       polygon;
typedef std::vector<polygon>        polygon_array;
typedef std::vector<vertex>         vertex_array;
typedef std::vector<vertex_color>   color_array;
typedef std::vector<double>         property;
typedef std::vector<property>       property_array;



class Mesh
{
public:
    // Construct Mesh; allocates storage for nv vertices and np polygons.
    Mesh( int nv=0, int np=0 )
    {
        vertices.reserve(nv);
        polygons.reserve(np);
        properties.reserve(nv);
        colors.reserve(nv);
    }

    ~Mesh()
    {
        vertices.clear();
        polygons.clear();
        tris.clear();
        quads.clear();
        colors.clear();
        alt_colors.clear();
    }

    // Add a 3D vertex to the mesh.
    void add_vertex( double x, double y, double z )
    {
        vertices.push_back( {float(x), float(y), float(z)} );
        colors.push_back( {0.0, 0.0, 0.0, 1.0} );
    }

    // Assign all vertices at once.
    void set_vectices( vertex_array& vert)          { vertices.swap(vert); }

    // Get all vertices.
    const vertex_array& get_vertices()              { return vertices; }


    // Add a polygon (either triangle or quad) to the mesh.
    void add_polygon( polygon& p )
    {
        polygons.push_back( p );
        if ( p.size() == 3 ) {
            tris.push_back( p.at(0) );
            tris.push_back( p.at(1) );
            tris.push_back( p.at(2) );
        }
        if ( p.size() == 4 ) {
            quads.push_back( p.at(0) );
            quads.push_back( p.at(1) );
            quads.push_back( p.at(2) );
            quads.push_back( p.at(3) );
        }
    }

    // Removes items from polygon array.
    // NOTE: Does not remove the corresponding elements in tris/quads!
    void remove_polygons( std::vector<uint32_t>& ind )
    {
        std::sort( ind.begin(), ind.end(), std::greater<uint32_t>() );
        for (auto i : ind)
            polygons.erase( polygons.begin()+i );
    }

    // Returns polygons: May contain mixed triangle/quad data.
    const polygon_array& get_polygons()                 { return polygons; }

    // Get triangle, quad indices.
    const std::vector<uint32_t>& get_triangle_indices() { return tris; }
    const std::vector<uint32_t>& get_quad_indices()     { return quads; }

    // Set color for vertex i.
    void set_vertex_color( uint32_t i, vertex_color& c )
    {
        if ( colors.size() > i )
            colors.at(i) = c;
        else
            colors.push_back( c );
    }

    // Add secondary/alternative set of vertex colors.
    void set_alt_colors( const color_array& alt )   { alt_colors = alt; }

    // Returns either primary (i=0) or secondary colors (i!=0).
    const color_array& get_vertex_colors( int i=0 )
    {
        if (i)
            return alt_colors;
        return colors;
    }

    // Set a property for mesh vertices.
    void set_property( property& prop )             { properties.push_back(prop); }
    const property_array& get_properties()          { return properties; }


private:
    vertex_array    vertices;
    polygon_array   polygons;
    color_array     colors;
    color_array     alt_colors;
    property_array  properties;

    std::vector<uint32_t> tris;
    std::vector<uint32_t> quads;
};
