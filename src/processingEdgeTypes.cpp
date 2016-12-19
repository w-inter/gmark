/*
 * processingEdgeTypes.cpp
 *
 *  Created on: Nov 18, 2016
 *      Author: wilcovanleeuwen
 */

#include "processingEdgeTypes.h"
#include "configparser.h"
#include "analyseCorrelation.h"

namespace std {

processingEdgeTypes::processingEdgeTypes(config::config conf_, vector<int> nb_nodes_per_graph_, string conf_file_) {
	this->conf = conf_;
	this->nb_nodes_per_graph = nb_nodes_per_graph_;
	this->conf_file = conf_file_;
}

processingEdgeTypes::~processingEdgeTypes() {
	// TODO Auto-generated destructor stub
}

void processingEdgeTypes::sequentialProcessing() {
	// Delete the output graphs
	auto runningTime = 0;
	for (int i=0; i<conf.nb_graphs; i++) {
		string file = "outputGraph" + to_string(i) + ".txt";
		const char * fileChar = file.c_str();
		remove(fileChar);
	}

//	// Keep track of the edge-types that are already processed due to the correlation
//	vector<int> allreadyProcessed;
//
//	vector<incrementalDeterministicGraphGenerator::edge2> generatedEdges;
//	for (config::correlationDef correlation: conf.correlations) {
//		for (int edgeTypeIdBasis: correlation.basis_for_correlation) {
//			vector<incrementalDeterministicGraphGenerator::edge2> edges = processEdgeType(edgeTypeIdBasis);
//		}
//	}

	// Generate the remaining edge-types




	for (int i=0; i<conf.schema.edges.size(); i++) {
//		cout << "Processing edge-type " << i << endl;
		processEdgeType(i);
	}

	cout << runningTime << ", ";
}

void processingEdgeTypes::processEdgeType(int edgeTypeId) {
	incrementalDeterministicGraphGenerator graphGenerator = incrementalDeterministicGraphGenerator();

	// For all different graph sizes
	for (int j=0; j<conf.nb_graphs; j++) {
		// Update config for the right number of nodes
		config::config conf2;
		if (nb_nodes_per_graph.size() > j) {
			conf2.nb_nodes = nb_nodes_per_graph[j];
		} else {
			conf2.nb_nodes = 0;
		}
		// Parse the confoguration
		configparser::parse_config(conf_file, conf2, j);
		conf2.complete_config();
		conf2.nb_graphs = conf.nb_graphs;

		// Define the output file
		ofstream outputFile;
		outputFile.open("outputGraph" + to_string(j) + ".txt", ios::app);

		graphGenerator.processEdgeTypeSingleGraph(conf2, conf2.schema.edges.at(edgeTypeId), outputFile, j);

		if (conf2.schema.edges[edgeTypeId].correlated_with.size() > 0) {
			analyseCorrelation correlation(conf2);
			correlation.analyze();
		}
	}
}

} /* namespace std */
