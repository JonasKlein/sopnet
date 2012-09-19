#ifndef HYPOTHESES_H
#define HYPOTHESES_H

#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include "graph.h"
#include "log.h"
#include "traxels.h"

namespace Tracking {
  ////
  //// HypothesesGraph
  ////

  // Properties of a HypothesesGraph

  // node_timestep
  struct node_timestep {};
  template <typename Graph>
    struct property_map<node_timestep, Graph> {
    typedef lemon::IterableValueMap< Graph, typename Graph::Node, int > type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<node_timestep,Graph>::name = "node_timestep";

  // node_traxel
  struct node_traxel {};
  class Traxel;
  template <typename Graph>
    struct property_map<node_traxel, Graph> {
    typedef lemon::IterableValueMap< Graph, typename Graph::Node, Traxel > type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<node_traxel,Graph>::name = "node_traxel";

  // node_active
  struct node_active {};
  template <typename Graph>
    struct property_map<node_active, Graph> {
    typedef lemon::IterableBoolMap< Graph, typename Graph::Node> type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<node_active,Graph>::name = "node_active";

  // arc_from_timestep
  struct arc_from_timestep {};
  template <typename Graph>
    struct property_map<arc_from_timestep, Graph> {
    typedef typename Graph::template ArcMap<int> type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<arc_from_timestep,Graph>::name = "arc_from_timestep";

  // arc_to_timestep
  struct arc_to_timestep {};
  template <typename Graph>
    struct property_map<arc_to_timestep, Graph> {
    typedef typename Graph::template ArcMap<int> type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<arc_to_timestep,Graph>::name = "arc_to_timestep";

  // arc_active
  struct arc_active {};
  template <typename Graph>
    struct property_map<arc_active, Graph> {
    typedef lemon::IterableBoolMap< Graph, typename Graph::Arc> type;
    static const std::string name;
  };
  template <typename Graph>
    const std::string property_map<arc_active,Graph>::name = "arc_active";



  class HypothesesGraph : public PropertyGraph<lemon::ListDigraph> {
  public:
    typedef property_map<node_timestep, HypothesesGraph::base_graph>::type node_timestep_map;

    HypothesesGraph() {
      // Properties attached to every HypothesesGraph
      add(node_timestep());
      add(arc_from_timestep());
      add(arc_to_timestep());
    };

    // use this instead of calling the parent graph directly
    HypothesesGraph::Node add_node(node_timestep_map::Value timestep);

    const std::set<HypothesesGraph::node_timestep_map::Value>& timesteps() const;
    node_timestep_map::Value earliest_timestep() const;
    node_timestep_map::Value latest_timestep() const;
    
    private:
    std::set<node_timestep_map::Value> timesteps_;      
  };

}
#endif /* HYPOTHESES_H */
