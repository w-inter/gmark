MATCH (x0)<-[:p2]-()-[:p3]->(x1), (x1)-[:p6]->()<-[:p6]-()<-[:p3]-()-[:p2]->(x2), (x0)<-[:p2]-()-[:p2]->(x3), (x2)<-[:p2]-()-[:p2]->(x4) RETURN DISTINCT x0;
