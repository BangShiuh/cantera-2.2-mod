/**
 *  @file ReactionPath.h
 *
 *  Classes for reaction path analysis.
 *
 * $Author$
 * $Revision$
 * $Date$
 */

// Copyright 2001  California Institute of Technology


#ifndef CT_RXNPATH_H
#define CT_RXNPATH_H

// STL includes
#include <vector>
#include <map>
#include <string>
#include <iostream>
using namespace std;

// Cantera includes 
#include "ct_defs.h"
#include "DenseMatrix.h"
#include "Group.h"
#include "Kinetics.h"

namespace Cantera {

    enum flow_t   { NetFlow, OneWayFlow };

    Group parseGroupString(string str, vector<string>& esyms);

    // forward references
    class Path; 

    /**
     *  Nodes in reaction path graphs.
     */
    class SpeciesNode {
    public:

        typedef vector<Path*> path_list;

        /// Default constructor
        SpeciesNode() : number(-1), name(""), value(0.0), 
                        visible(false) {}

        /// Destructor
        virtual ~SpeciesNode() {}

        // public attributes
        int         number;           ///<  Species number
        string      name;             ///<  Label on graph
        doublereal  value;            ///<  May be used to set node appearance
        bool        visible;          ///<  Visible on graph;


        // public methods

        /** 
         *  @name References.
         * Return a reference to a path object connecting this node
         *  to another node.
         */
        //@{
        Path*        path(int n)       { return m_paths[n]; }
        const Path*  path(int n) const { return m_paths[n]; }
        //@}


        /// Total number of paths to or from this node 
        int nPaths() const { return m_paths.size(); }

        /// add a path to or from this node
        void addPath(Path* path) { m_paths.push_back(path); }

    protected:

        path_list m_paths;
    };


        
    class Path {

    public:
 
        typedef map<int, doublereal> rxn_path_map;

        /**
         *  Constructor. Construct a one-way path from 
         *  \c begin to \c end.
         */
        Path(SpeciesNode* begin, SpeciesNode* end);

        /// Destructor
        virtual ~Path() {}

        void addReaction(int rxnNumber, doublereal value, string label = "");

        /// Upstream node.
        const SpeciesNode* begin() const { return m_a; }
        SpeciesNode* begin() { return m_a; }

        /// Downstream node.
        const SpeciesNode* end() const { return m_b; }
        SpeciesNode* end() { return m_b; }

        /**
         *  If \c n is one of the nodes this path connects, then
         *  the other node is returned. Otherwise zero is returned.
         */
        SpeciesNode* otherNode(SpeciesNode* n) { 
            return (n == m_a ? m_b : (n == m_b ? m_a : 0));
        }

        /// The total flow in this path
        doublereal flow() { return m_total; }
        void setFlow(doublereal v) { m_total = v; }

        ///  Number of reactions contributing to this path
        int nReactions() { return m_rxn.size(); }

        ///  Map from reaction number to flow from that reaction in this path.
        const rxn_path_map& reactionMap() { return m_rxn; }

        void writeLabel(ostream& s, doublereal threshold = 0.005);
        
    protected:

        map<string, doublereal> m_label;
        SpeciesNode *m_a, *m_b;
        rxn_path_map m_rxn;
        doublereal m_total;
    };


    /**
     *  Reaction path diagrams (graphs).
     */
    class ReactionPathDiagram {

    public:

        ReactionPathDiagram();
        
        virtual ~ReactionPathDiagram();

        /// The largest one-way flow value in any path
        doublereal maxFlow() { return m_flxmax; }

        /// The net flow from node \c k1 to node \c k2
        doublereal netFlow(int k1, int k2) { 
            return flow(k1, k2) - flow(k2, k1);
        }

        /// The one-way flow from node \c k1 to node \c k2
        doublereal flow(int k1, int k2) {
            return (m_paths[k1][k2] ? m_paths[k1][k2]->flow() : 0.0);
        }

        /// True if a node for species k exists
        bool hasNode(int k) {
            return (m_nodes[k] != 0);
        }

        void writeData(ostream& s);
        void exportToDot(ostream& s);
        void add(ReactionPathDiagram& d);
        SpeciesNode* node(int k) { return m_nodes[k]; }
        Path* path(int k1, int k2) { return m_paths[k1][k2]; }
        Path* path(int n) { return m_pathlist[n]; }
        int nPaths() { return m_pathlist.size(); }
        int nNodes() { return m_nodes.size(); }

        void addNode(int k, string nm, doublereal x = 0.0);

        void displayOnly(int k=-1) { m_local = k; }

        void linkNodes(int k1, int k2, int rxn, doublereal value,
            string legend = "");

        void include(string name) { m_include.push_back(name); }
        void exclude(string name) { m_exclude.push_back(name); }
        void include(vector<string>& names) { 
            int n = names.size();
            for (int i = 0; i < n; i++) m_include.push_back(names[i]);
        }
        void exclude(vector<string>& names) { 
            int n = names.size();
            for (int i = 0; i < n; i++) m_exclude.push_back(names[i]);
        }
        vector<string>& included() { return m_include; }
        vector<string>& excluded() { return m_exclude; }
        vector_int species();
        vector_int reactions();
        void findMajorPaths(doublereal threshold, int lda, doublereal* a);
        void setFont(string font) {
            m_font = font;
        }
        // public attributes

        string title;
        string bold_color;
        string normal_color;
        string dashed_color;
        string element;
        string m_font;
        doublereal threshold, 
            bold_min, dashed_max, label_min;
        doublereal x_size, y_size;
        string name, dot_options;
        flow_t flow_type;
        double scale;
        double arrow_width; 
        bool show_details;
        double arrow_hue;

    protected:

        doublereal                    m_flxmax;
        map<int, map<int, Path*> >    m_paths;
        map<int, SpeciesNode*>        m_nodes;
        vector<Path*>                 m_pathlist;
        vector<string>                m_include;
        vector<string>                m_exclude;
        vector_int                   m_speciesNumber;
        map<int, int>                 m_rxns;
        int                           m_local;
    };



    class ReactionPathBuilder {

    public:
        ReactionPathBuilder() {}
        virtual ~ReactionPathBuilder() {}
    
        int init(ostream& logfile, Kinetics& s);

        int build(Kinetics& s, string element, ostream& output, 
            ReactionPathDiagram& r, bool quiet=false);

        int findGroups(ostream& logfile, Kinetics& s);
 
        void writeGroup(ostream& out, const Group& g);

    protected:

        int m_nr;
        int m_ns;
        int m_nel;
        vector_fp m_ropf;
        vector_fp m_ropr;
        array_fp m_x;
        vector<vector_int> m_reac;
        vector<vector_int> m_prod;
        DenseMatrix m_elatoms;
        vector<vector<int> > m_groups;
        vector<Group> m_sgroup;
        vector<string> m_elementSymbols;
        //        map<int, int> m_warn;
        map<int, map<int, map<int, Group> > >  m_transfer;
        vector<bool> m_determinate;
    };

}

#endif
