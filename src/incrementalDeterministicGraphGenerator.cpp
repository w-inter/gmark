/*
 * incrementalDeterministicGraphGenerator.cpp
 *
 *  Created on: Sep 16, 2016
 *      Author: wilcovanleeuwen
 */

#include <vector>
#include "incrementalDeterministicGraphGenerator.h"
#include "graphNode.h"

namespace std {

incrementalDeterministicGraphGenerator::incrementalDeterministicGraphGenerator(config::config configuration) {
	this->conf = configuration;
}

incrementalDeterministicGraphGenerator::~incrementalDeterministicGraphGenerator() {
	// TODO Auto-generated destructor stub
}

// ####### Generate edges #######
//graphNode *incrementalDeterministicGraphGenerator::findNodeIdFromCumulProbs(vector<float> & cumulProbs, bool findSourceNode) {
//	uniform_real_distribution<double> distribution(0.0,1.0);
//	double randomValue = distribution(randomGenerator);
//	int i = cumDistrUtils.findPositionInCdf(cumulProbs, randomValue);
//	if (findSourceNode) {
//		return &nodes.first.at(i);
//	} else {
//		return &nodes.second.at(i);
//	}
//}

int incrementalDeterministicGraphGenerator::findSourceNode(config::edge & edgeType, vector<graphNode*> &nodesWithOpenICs) {
	double randomValue = uniformDistr(randomGenerator);
	int subjectNodeId = cumDistrUtils.calculateCDF(nodesWithOpenICs, tempNode, randomValue);

	return subjectNodeId;
}

int incrementalDeterministicGraphGenerator::findTargetNode(config::edge & edgeType, int sourceNodeLocalId, vector<graphNode*> &nodesWithOpenICs) {
	double randomValue = uniformDistr(randomGenerator);
	int objectNodeId = cumDistrUtils.calculateCDF(nodesWithOpenICs, nodes.first[sourceNodeLocalId], randomValue);

	return objectNodeId;
}


void incrementalDeterministicGraphGenerator::addEdge(graphNode &sourceNode, graphNode &targetNode, int predicate, ofstream*  outputFile) {
	sourceNode.decrementOpenInterfaceConnections();
	targetNode.decrementOpenInterfaceConnections();
//	nodes.first.at(sourceNode->iterationId).decrementOpenInterfaceConnections();
//	nodes.second.at(targetNode->iterationId).decrementOpenInterfaceConnections();

//	sourceNode->setConnection(targetNode->iterationId, 1);
//	nodes.first.at(sourceNode->iterationId).setConnection(targetNode->iterationId, 1);
	*outputFile << sourceNode.type << "-" << sourceNode.iterationId << " " << predicate << " " << targetNode.type << "-" << targetNode.iterationId << endl;
}
// ####### Generate edges #######


// ####### Update interface-connections #######
void incrementalDeterministicGraphGenerator::updateInterfaceConnectionsForZipfianDistributions(vector<graphNode> *nodesVec, distribution distr) {
//	cout << "New Zipfian case" << endl;
	int nmNodes = nodesVec->size();

	vector<float> zipfianCdf = cumDistrUtils.zipfCdf(distr, nmNodes);
	int newInterfaceConnections = 0;
	int difference = 0;
	for (int i=0; i<nmNodes; i++) {
		graphNode *node = &nodesVec->at(i);

//		cout << "Postion of node" << i << ": " << node->getPosition(outDistr) << endl;
//		cout << "Old NmOfInterfaceConnections: " << node->getNumberOfInterfaceConnections(outDistr) << endl;
//		cout << "Old NmOfOpenInterfaceConnections: " << node->getNumberOfOpenInterfaceConnections() << endl;
//		cout << "node.getPosition(edgeTypeId, outDistr): " << node.getPosition(edgeTypeId, outDistr) << endl;
//		cout << "zipfianCdf.size: " << zipfianCdf.size() << endl;
//		cout << "distr.arg1: " << distr.arg1 << endl;
		newInterfaceConnections = cumDistrUtils.findPositionInCdf(zipfianCdf, node->getPosition()) + distr.arg1;

//		cout << "newInterfaceConnections: " << newInterfaceConnections << endl;

		difference = newInterfaceConnections - node->getNumberOfInterfaceConnections();
		node->incrementOpenInterfaceConnectionsByN(difference);
		node->setNumberOfInterfaceConnections(newInterfaceConnections);

//		cout << "difference: " << difference << endl;
//		cout << "New NmOfInterfaceConnections: " << node->getNumberOfInterfaceConnections(outDistr) << endl;
//		cout << "New NmOfOpenInterfaceConnections: " << node->getNumberOfOpenInterfaceConnections() << endl;
	}
}


void incrementalDeterministicGraphGenerator::updateICsForNonScalableType(vector<graphNode> & nodes, int iterationNumber, double meanUpdateDistr, double meanNonUpdateDistr, distribution & distr) {
//	cout << "Updating ICs for non scalable type" << endl;
	if (nodes.size() == 0) {
		cout << "A fixed amount of nodes of one of the types is equal to 0" << endl;
		return;
	}

//	cout << "iterationNumber: " << iterationNumber << endl;
	int increment = floor((((iterationNumber + 1) * meanNonUpdateDistr) / nodes.size()) - meanUpdateDistr);
	if (increment > 0) {
		if (distr.type == DISTRIBUTION::NORMAL || distr.type == DISTRIBUTION::ZIPFIAN) {
//			cout << "Before distr.arg1: " << distr.arg1 << endl;
			distr.arg1 += increment;
//			cout << "After distr.arg1: " << distr.arg1 << endl;
		} else if (distr.type == DISTRIBUTION::UNIFORM) {
//			cout << "Before distr.arg1: " << distr.arg1 << endl;
//			cout << "Before distr.arg2: " << distr.arg2 << endl;
			double diff = distr.arg2 - distr.arg1;
			double newMean = ((distr.arg2 + distr.arg1) / 2) + (double) increment;
			if (diff > 0.0) {
				distr.arg1 = max(round(newMean - (diff/2)), 0.0);
				distr.arg2 = round(newMean + (diff/2));
			} else {
				distr.arg1 = newMean;
				distr.arg2 = newMean;
			}
//				cout << "After distr.arg1: " << distr.arg1 << endl;
//				cout << "After distr.arg2: " << distr.arg2 << endl;
		}

//		cout << "Update nodes with " << increment << endl;
		for (int i=0; i<nodes.size(); i++) {
			nodes[i].incrementOpenInterfaceConnectionsByN(increment);
			nodes[i].incrementInterfaceConnectionsByN(increment);

		}
	}
}

void incrementalDeterministicGraphGenerator::updateICsForNonScalableType(config::edge & edgeType, int iterationNumber) {
	if (!conf.types.at(edgeType.subject_type).scalable) {
		// Subject is not scalable so update the ICs of all the subject nodes
		if (iterationNumber >= conf.types.at(edgeType.subject_type).size) {
			double meanOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, nodes.first.size());
			double meanInDistr = getMeanICsPerNode(edgeType.incoming_distrib, nodes.second.size());
			updateICsForNonScalableType(nodes.first, iterationNumber, meanOutDistr, meanInDistr, edgeType.outgoing_distrib);
		}
	} else {
		// object is not scalable so update the ICs of all the object nodes
		if (iterationNumber >= conf.types.at(edgeType.object_type).size) {
			double meanOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, nodes.first.size());
			double meanInDistr = getMeanICsPerNode(edgeType.incoming_distrib, nodes.second.size());
			updateICsForNonScalableType(nodes.second, iterationNumber, meanInDistr, meanOutDistr, edgeType.incoming_distrib);
		}
	}
}
// ####### Update interface-connections #######

double incrementalDeterministicGraphGenerator::getMeanICsPerNode(distribution & distr, int zipfMax) {
	double meanEdgesPerNode;
	if (distr.type == DISTRIBUTION::NORMAL) {
		meanEdgesPerNode = distr.arg1;
	} else if (distr.type == DISTRIBUTION::UNIFORM) {
		meanEdgesPerNode = (distr.arg2 + distr.arg1) / 2.0;
	} else if (distr.type == DISTRIBUTION::ZIPFIAN) {
		if (distr.arg2 > 3.5) {
			// This mean hold for alpha > 2, but I will use it only when alpha > 3.5 to keep the error between the theoretical mean and the practical mean as low as possible
			// TODO: maybe instead of the magic number 3.5: use a value dependent on alpha and the total number of nodes in the graph, n. The higher the n, the lower the alpha can be. But always alpha > 2.
			meanEdgesPerNode = (1.0/(distr.arg2-2.0)) + distr.arg1;
		} else {
			double temp = 0.0;
			for (int i=1; i<=zipfMax; i++) {
				temp += i*pow((i), -1*distr.arg2);
			}
			meanEdgesPerNode = temp - 1 + distr.arg1;
		}
//		cout << "Zipfian mean=" << temp << endl;
	} else {
		meanEdgesPerNode = 1;
	}
	return meanEdgesPerNode;
}

void incrementalDeterministicGraphGenerator::changeDistributionParams(distribution & distr, double meanICsPerNodeForOtherDistr, double probOrSizeOther, double probOrSize) {
	if (distr.type == DISTRIBUTION::NORMAL) {
		distr.arg1 = (probOrSizeOther * meanICsPerNodeForOtherDistr) / probOrSize;
			cout << "new normal mean: " << distr.arg1 << endl;
	} else if (distr.type == DISTRIBUTION::UNIFORM) {
		double diff = distr.arg2 - distr.arg1;
		double newMean = (probOrSizeOther * meanICsPerNodeForOtherDistr) / probOrSize;
		if (diff > 0.0) {
			distr.arg1 = max(round(newMean - (diff/2)), 0.0);
			distr.arg2 = round(newMean + (diff/2));
		} else {
			distr.arg1 = round(newMean);
			distr.arg2 = round(newMean);
		}
		cout << "New uniform min: " << distr.arg1 << endl;
		cout << "New uniform max: " << distr.arg2 << endl;
	}
}

void incrementalDeterministicGraphGenerator::changeDistributionParams(config::edge & edgeType) {
//	cout << "Changing edge-type " << edgeType.edge_type_id << endl;

	if (conf.types.at(edgeType.subject_type).scalable ^ conf.types.at(edgeType.object_type).scalable) {
		if (conf.types.at(edgeType.subject_type).scalable) {
			// In-distr is not scalable, so change the in-distribution
			double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 10000);
			double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, conf.types.at(edgeType.object_type).size);
			if (edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
				edgeType.incoming_distrib.arg1 = max(1.0, round(meanICsPerNodeForOutDistr - meanICsPerNodeForInDistr));
			} else {
				changeDistributionParams(edgeType.incoming_distrib, meanICsPerNodeForOutDistr, conf.types.at(edgeType.object_type).size, conf.types.at(edgeType.object_type).size);
			}
		} else {
			// Out-distr is not scalable, so change the out-distribution
			double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, 10000);
			double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, conf.types.at(edgeType.subject_type).size);
			if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN) {
//				cout << "zipfianStartValueOut" << endl;
				edgeType.outgoing_distrib.arg1 = max(1.0, round(meanICsPerNodeForInDistr - meanICsPerNodeForOutDistr));
			} else {
				changeDistributionParams(edgeType.outgoing_distrib, meanICsPerNodeForInDistr, conf.types.at(edgeType.subject_type).size, conf.types.at(edgeType.subject_type).size);
			}
		}


		if (edgeType.incoming_distrib.type == DISTRIBUTION::UNDEFINED) {
			edgeType.incoming_distrib = distribution(DISTRIBUTION::UNIFORM, 1, 1);
		}
		return;
	}

	double subjectProbOrSize;
	double objectProbOrSize;
	if (conf.types.at(edgeType.subject_type).scalable) {
		subjectProbOrSize = conf.types.at(edgeType.subject_type).proportion;
		objectProbOrSize = conf.types.at(edgeType.object_type).proportion;
	} else {
		subjectProbOrSize = conf.types.at(edgeType.subject_type).size;
		objectProbOrSize = conf.types.at(edgeType.object_type).size;
	}

	// In-distr is undefined, so change this in-distr
	if (edgeType.incoming_distrib.type == DISTRIBUTION::UNDEFINED) {
		double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 10000);
		changeDistributionParams(edgeType.incoming_distrib, meanICsPerNodeForOutDistr, subjectProbOrSize, objectProbOrSize);
	}

	// Out-distr is Zipfian, so change the params of the in-distr (only if )
	if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN && edgeType.incoming_distrib.type != DISTRIBUTION::ZIPFIAN) {
		double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 10000);
		double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, 0);

//		cout << "meanICsPerNodeForInDistr: " << meanICsPerNodeForInDistr << endl;
//		cout << "objectProbOrSize: " << objectProbOrSize << endl;
//		cout << "meanICsPerNodeForOutDistr: " << meanICsPerNodeForOutDistr << endl;
//		cout << "subjectProbOrSize: " << subjectProbOrSize << endl;
		if ((objectProbOrSize * meanICsPerNodeForInDistr) > (subjectProbOrSize * meanICsPerNodeForOutDistr)) {
			// Mean of ICsPerIteration for non-Zipfian distr is higher that the mean of the Zipfian distr
//			changeDistributionParams(edgeType, meanICsPerNodeForOutDistr, false, subjectProbOrSize, objectProbOrSize);
			double newMean = (objectProbOrSize * meanICsPerNodeForInDistr) / subjectProbOrSize;
			edgeType.outgoing_distrib.arg1 = ceil(newMean - meanICsPerNodeForOutDistr);
//			cout << "Change the Zipfian start value to: " << zipfianStartValueOut << endl;
		}
		return;
	}
	// In-distr is Zipfian, so change the params of the out-distr
	if (edgeType.outgoing_distrib.type != DISTRIBUTION::ZIPFIAN && edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
		double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, 10000);
		double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 0);

		if ((subjectProbOrSize * meanICsPerNodeForOutDistr) > (objectProbOrSize * meanICsPerNodeForInDistr)) {
			// Mean of ICsPerIteration for non-Zipfian distr is higher that the mean of the Zipfian distr
//			changeDistributionParams(edgeType, meanICsPerNodeForInDistr, true, subjectProbOrSize, objectProbOrSize);
			double newMean = (subjectProbOrSize * meanICsPerNodeForOutDistr) / objectProbOrSize;
			edgeType.incoming_distrib.arg1 = ceil(newMean - meanICsPerNodeForInDistr);
//			cout << "Change the Zipfian start value to: " << zipfianStartValueIn << endl;
		}
		return;
	}

	// Both are non-Zipfian
	if (edgeType.outgoing_distrib.type != DISTRIBUTION::ZIPFIAN && edgeType.incoming_distrib.type != DISTRIBUTION::ZIPFIAN) {
		double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 0);
		double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, 0);

		if (meanICsPerNodeForOutDistr * subjectProbOrSize >
			meanICsPerNodeForInDistr * objectProbOrSize) {
			// Change in-distr
			cout << "meanICsPerNodeForOutDistr: " << meanICsPerNodeForOutDistr << endl;
			cout << "subjectProbOrSize: " << subjectProbOrSize << endl;
			cout << "objectProbOrSize: " << objectProbOrSize << endl;
			changeDistributionParams(edgeType.incoming_distrib, meanICsPerNodeForOutDistr, subjectProbOrSize, objectProbOrSize);
			return;
		} else {
			// Change out-distr
			changeDistributionParams(edgeType.outgoing_distrib, meanICsPerNodeForInDistr, objectProbOrSize, subjectProbOrSize);
			return;
		}
	}

	if (edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN && edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
		double meanICsPerNodeForOutDistr = getMeanICsPerNode(edgeType.outgoing_distrib, 10000);
		double meanICsPerNodeForInDistr = getMeanICsPerNode(edgeType.incoming_distrib, 10000);
		if (meanICsPerNodeForOutDistr * subjectProbOrSize >
			meanICsPerNodeForInDistr * objectProbOrSize) {
			// Change in-distr
			double newMean = (subjectProbOrSize * meanICsPerNodeForOutDistr) / objectProbOrSize;
			edgeType.incoming_distrib.arg1 = ceil(newMean - meanICsPerNodeForInDistr);
//			cout << "Change the ZipfianIn start value to: " << zipfianStartValueIn << endl;
			return;
		} else {
			// Change out-distr
			double newMean = (objectProbOrSize * meanICsPerNodeForInDistr) / subjectProbOrSize;
			edgeType.outgoing_distrib.arg1 = ceil(newMean - meanICsPerNodeForOutDistr);
//			cout << "Change the ZipfianOut start value to: " << zipfianStartValueOut << endl;
			return;
		}
	}
}

int incrementalDeterministicGraphGenerator::getNumberOfOpenICs(vector<graphNode*> nodes) {
	int openICs = 0;
	for(int i=0; i<nodes.size(); i++) {
		openICs += (*nodes.at(i)).getNumberOfOpenInterfaceConnections();
	}
	return openICs;
}

//int incrementalDeterministicGraphGenerator::getNumberOfEdgesPerIteration(config::edge edgeType, pair<int, int> zipfOpenInterfaceConnections) {
//	int c = round(max(getMeanICsPerNode(edgeType.outgoing_distrib, 10000), getMeanICsPerNode(edgeType.incoming_distrib, 10000)));
//	if (!conf.types.at(edgeType.subject_type).scalable && conf.types.at(edgeType.object_type).scalable) {
//		c = round(getMeanICsPerNode(edgeType.incoming_distrib, 10000));
//	} else if (conf.types.at(edgeType.subject_type).scalable && !conf.types.at(edgeType.object_type).scalable) {
//		c = round(getMeanICsPerNode(edgeType.outgoing_distrib, 10000));
//	}
//
//	int numberOfEdgesPerIteration;
//	int sf = 2;
//	if (zipfOpenInterfaceConnections.first != -1 && zipfOpenInterfaceConnections.second != -1) {
//		// Zipf in- and out-distr case
//		numberOfEdgesPerIteration = min(zipfOpenInterfaceConnections.first, zipfOpenInterfaceConnections.second);
//	} else if (zipfOpenInterfaceConnections.first != -1){
//		// Zipf out-distr case
//		int openICsForInDistr = getNumberOfOpenICs(nodes.second);
//		numberOfEdgesPerIteration = min(zipfOpenInterfaceConnections.first, openICsForInDistr);
//	} else if (zipfOpenInterfaceConnections.second != -1) {
//		// Zipf in-distr case
//		int openICsForOutDistr = getNumberOfOpenICs(nodes.first);
//		numberOfEdgesPerIteration = min(openICsForOutDistr, zipfOpenInterfaceConnections.second);
//	} else {
//		// Non Zipfian case
//		int openICsForInDistr = getNumberOfOpenICs(nodes.second);
//		int openICsForOutDistr = getNumberOfOpenICs(nodes.first);
//		numberOfEdgesPerIteration = min(openICsForOutDistr, openICsForInDistr);
//	}
//	numberOfEdgesPerIteration -= c*sf;
//	return numberOfEdgesPerIteration;
//}

int incrementalDeterministicGraphGenerator::getNumberOfEdgesPerIteration(config::edge edgeType, vector<graphNode*> subjectNodesWithOpenICs, vector<graphNode*> objectNodesWithOpenICs, int iterationNumber) {
	int scale = 5;

	int subjects;
	if (conf.types.at(edgeType.subject_type).scalable) {
		subjects = round(max(1.0, conf.types.at(edgeType.subject_type).proportion / conf.types.at(edgeType.object_type).proportion)) * iterationNumber;
	} else {
		subjects = conf.types.at(edgeType.subject_type).size;
	}

	int objects;
	if (conf.types.at(edgeType.object_type).scalable) {
		objects = round(max(1.0, conf.types.at(edgeType.object_type).proportion / conf.types.at(edgeType.subject_type).proportion)) * iterationNumber;
	} else {
		objects = conf.types.at(edgeType.object_type).size;
	}

	int c = round(max(getMeanICsPerNode(edgeType.outgoing_distrib, subjects), getMeanICsPerNode(edgeType.incoming_distrib, objects)));

	if (conf.types.at(edgeType.subject_type).scalable ^ conf.types.at(edgeType.object_type).scalable) {
		c = 1;
	}
	int openICs = min(getNumberOfOpenICs(subjectNodesWithOpenICs), getNumberOfOpenICs(objectNodesWithOpenICs));
//	cout << "OpenConnections: "  << openICs << endl;
//	cout << "c: "  << c << endl;
	return openICs - (c*scale);
}

vector<graphNode*> getNodesWithAtLeastOneOpenIC(vector<graphNode> & nodes) {
	vector<graphNode*> nodesWithOpenICs;
	for (graphNode & node: nodes) {
		if (node.getNumberOfOpenInterfaceConnections() > 0) {
			nodesWithOpenICs.push_back(&node);
		}
	}
	return nodesWithOpenICs;
}

void incrementalDeterministicGraphGenerator::processIteration(int iterationNumber, config::edge & edgeType, ofstream*  outputFile) {
//	if (iterationNumber % 1000 == 0) {
//		cout << endl<< "---Process interationNumber " << to_string(iterationNumber) << " of edgeType " << to_string(edgeType.edge_type_id) << "---" << endl;
//	}
	nodeGen.addSubjectNodes(edgeType);
	nodeGen.addObjectNodes(edgeType);

//	cout << "Subjects: " << nodes.first.size() << endl;
//	cout << "Objects: " << nodes.second.size() << endl;

//	int subjectNodes = graph.nodes.first.size();
//	int objectNodes = graph.nodes.second.size();


	if (conf.types.at(edgeType.subject_type).scalable ^ conf.types.at(edgeType.object_type).scalable) {
		updateICsForNonScalableType(edgeType, iterationNumber);
	}

	if(edgeType.outgoing_distrib.type == DISTRIBUTION::ZIPFIAN) {
		updateInterfaceConnectionsForZipfianDistributions(&nodes.first, edgeType.outgoing_distrib);
	}
	if(edgeType.incoming_distrib.type == DISTRIBUTION::ZIPFIAN) {
		updateInterfaceConnectionsForZipfianDistributions(&nodes.second, edgeType.incoming_distrib);
	}

//	cout << "Subjects: " << nodes.first.size() << endl;
//	cout << "Objects: " << nodes.second.size() << endl;
	vector<graphNode*> subjectNodesWithOpenICs = getNodesWithAtLeastOneOpenIC(nodes.first);
	vector<graphNode*> objectNodesWithOpenICs = getNodesWithAtLeastOneOpenIC(nodes.second);
//	cout << "SubjectsWithOpenICs: " << subjectNodesWithOpenICs.size() << endl;
//	cout << "ObjectsWithOpenICs: " << objectNodesWithOpenICs.size() << endl;

//	int numberOfEdgesPerIteration = getNumberOfEdgesPerIteration(edgeType, zipfOpenInterfaceConnections);
	int numberOfEdgesPerIteration = getNumberOfEdgesPerIteration(edgeType, subjectNodesWithOpenICs, objectNodesWithOpenICs, iterationNumber);


//	cout << "numberOfEdgesPerIteration: " << numberOfEdgesPerIteration << endl;

	for (int i=0; i<numberOfEdgesPerIteration; i++) {
		int sourceNodeLocalId;
		int targetNodeLocalId;


		sourceNodeLocalId = findSourceNode(edgeType, subjectNodesWithOpenICs);
		if (sourceNodeLocalId == -1) {
//			cout << "Edge is not added because of no available ICs in the Subject nodes, so go to next iteration" << endl;
			break;
		} else {
			targetNodeLocalId = findTargetNode(edgeType, sourceNodeLocalId, objectNodesWithOpenICs);
		}

		if(targetNodeLocalId == -1) {
//			cout << "Target iterationId = -1" << endl;
		} else {
			addEdge(nodes.first[sourceNodeLocalId], nodes.second[targetNodeLocalId], edgeType.predicate, outputFile);
//			cout << "Edge added: [" << sourceNode->iterationId << " - " << targetNode->iterationId << "]" << endl;
		}
	}
}

void incrementalDeterministicGraphGenerator::processEdgeType(config::edge & edgeType, ofstream*  outputFile, int seed) {
//	cout << endl << endl << "-----Processing edge-type " << to_string(edgeType.edge_type_id) << "-----" << endl;

	randomGenerator.seed(seed);

	changeDistributionParams(edgeType);

	nodeGen = nodeGenerator(edgeType, &randomGenerator, &nodes, &conf);

	int nmOfIterations;
	if ((conf.types.at(edgeType.subject_type).scalable && conf.types.at(edgeType.object_type).scalable)
			|| (!conf.types.at(edgeType.subject_type).scalable && !conf.types.at(edgeType.object_type).scalable)) {
		nmOfIterations = min(conf.types.at(edgeType.object_type).size, conf.types.at(edgeType.subject_type).size);
	} else {
		nmOfIterations = max(conf.types.at(edgeType.object_type).size, conf.types.at(edgeType.subject_type).size);
	}
	int sf = 100;
//	cout << "Total number of iterations: " << nmOfIterations << endl;


//	int numberOfEdgesPerIteration = getNumberOfEdgesPerIteration(edgeType);
//	cout << "numberOfEdgesPerIteration: " << numberOfEdgesPerIteration << endl;

	for(int i=0; i<nmOfIterations; i+=sf) {
//		cout << "Number of maxNodes: " << numberOfNodesOfMax << endl;
		processIteration(i, edgeType, outputFile);
	}
}

//void incrementalDeterministicGraphGenerator::generateIncDetGraph(ofstream*  outputFile, int* seeds, int edgeTypeIdLow, int edgeTypeIdHigh) {
////	cout << "Generate a incremental deterministic graph (given a seed)" << endl;
//
//
//	for (int i=edgeTypeIdLow; i<edgeTypeIdHigh; i++) {
//		processEdgeType(conf.schema.edges.at(i), outputFile, seeds[i]);
//	}
//}

} /* namespace std */
