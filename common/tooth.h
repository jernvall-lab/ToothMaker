#pragma once

/**
 * @class Tooth
 * @brief Model output container.
 *
 * Contains everything related to a single stored model state; meshes, cell data,
 * data matrices etc.
 *
 * Each Tooth object owns the following fields:
 * - Mesh for storing 3D geometry.
 * - Cell data vector for storing concentrations.
 * - Cell shape vector for storing cell boundary vertices.
 *
 * All the above fields are filled independently, hence it is important to make
 * sure that e.g. the mesh vertex order corresponds to the cell data order.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "morphomaker.h"
#include "parameters.h"
#include "mesh.h"


class Tooth
{
public:

    // Set a tooth for render type (RENDER_MESH, RENDER_PIXEL, RENDER_HUMPPA).
    Tooth( int type ) : _tooth_type(type), _dim(0,0)   {}
    ~Tooth()    {}

    // Add boundary vertices for cell i (RENDER_HUMPPA)
    void add_cell_shape( uint32_t i, vertex& vert )
    {
        if ( _cell_shapes.size() > i )
            _cell_shapes.at(i).push_back( vert );
        else {
            std::vector<vertex> s = { vert };
            _cell_shapes.push_back(s);
        }
    }

    // Returns cell shapes (RENDER_HUMPPA)
    std::vector<vertex_array>& get_cell_shapes()        { return _cell_shapes; }

    // Set cell data (e.g., morphogen concentrations, RENDER_HUMPPA)
    void add_cell_data( std::vector<float>& data )      { _cell_data.push_back(data); }
    std::vector<std::vector<float>>& get_cell_data()    { return _cell_data; }

    // Set 2D domain dimensions (RENDER_PIXEL)
    void set_domain_dim( int m, int n )                 { _dim = std::make_pair(m,n); }
    std::pair<int,int> get_domain_dim()                 { return _dim; }

    // Returns Tooth object type (RENDER_MESH, RENDER_PIXEL, RENDER_HUMPPA)
    int get_tooth_type()                                { return _tooth_type; }

    // Sets object mesh (RENDER_MESH)
    void add_mesh( Mesh& m )                            { _mesh = m; }
    Mesh& get_mesh()                                    { return _mesh; }



private:
    std::vector<std::vector<float>> _cell_data;     // morphogen concentrations for RENDER_HUMPPA
    std::vector<vertex_array> _cell_shapes;         // cell boundaries for RENDER_HUMPPA
    int _tooth_type;                                // render mode
    std::pair<int,int> _dim;                        // domain dimensions for RENDER_PIXEL
    Mesh _mesh;                                     // mesh object for RENDER_MESH
};
