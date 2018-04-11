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
    Tooth( int type ) : m_toothType(type), m_dim(0,0)   {}
    ~Tooth()    {}

    // Add boundary vertices for cell i (RENDER_HUMPPA)
    void add_cell_shape( uint32_t i, mesh::vertex& vert )
    {
        if ( m_cellShapes.size() > i )
            m_cellShapes.at(i).push_back( vert );
        else {
            std::vector<mesh::vertex> s = { vert };
            m_cellShapes.push_back(s);
        }
    }

    // Returns cell shapes (RENDER_HUMPPA)
    std::vector<mesh::vertex_array>& get_cell_shapes()  { return m_cellShapes; }

    // Set cell data (e.g., morphogen concentrations, RENDER_HUMPPA)
    void add_cell_data( std::vector<float>& data )      { m_cellData.push_back(data); }
    std::vector<std::vector<float>>& get_cell_data()    { return m_cellData; }

    // Set 2D domain dimensions (RENDER_PIXEL)
    void set_domain_dim( int m, int n )                 { m_dim = std::make_pair(m,n); }
    std::pair<int,int> get_domain_dim()                 { return m_dim; }

    // Returns Tooth object type (RENDER_MESH, RENDER_PIXEL, RENDER_HUMPPA)
    int get_tooth_type()                                { return m_toothType; }

    // Sets object mesh (RENDER_MESH)
    void add_mesh( Mesh& m )                            { m_mesh = m; }
    Mesh& get_mesh()                                    { return m_mesh; }



private:
    std::vector<std::vector<float>> m_cellData;     // morphogen concentrations for RENDER_HUMPPA
    std::vector<mesh::vertex_array> m_cellShapes;   // cell boundaries for RENDER_HUMPPA
    int m_toothType;                                // render mode
    std::pair<int,int> m_dim;                       // domain dimensions for RENDER_PIXEL
    Mesh m_mesh;                                    // mesh object for RENDER_MESH
};
