# This test requires the Python packages matplotlib and networkx to be
# installed.
#
# The Python interface of finiteflow is not able to draw the dataflow
# graphs by itself.  However, information about nodes and edges is
# available and can be used by external packages to draw the graphs or
# perform any other analysis on them.
#
# The following example uses the Python packages networkx and
# matplotlib to draw a simple finiteflow graph.

from fflow import *
import matplotlib.pyplot as plt
import networkx as nx

# Define a FiniteFlow graph

(graph,innode) = NewGraphWithInput(3)

node1 = AlgSlice(graph,innode,0,3)
node2 = AlgSlice(graph,innode,0,1)
node3 = AlgChain(graph,[node1,node2])

node_name = {
    innode: "Input",
    node1: "Node 1",
    node2: "Node 2",
    node3: "Node 3"
}

SetOutputNode(graph,node3)

# Get the edges
edges = GraphEdges(graph)

# Build graph with networkx
g = nx.DiGraph()
g.add_edges_from(edges)

# Draw with networkx and matplotlib
nx.draw(g,
        arrows=True,
        with_labels=True,
        labels=node_name, # remove to use integer ids
        node_color='skyblue',
        node_size=2500)
plt.show()
