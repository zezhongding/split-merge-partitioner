#include "hep_graph.hpp"
#include "conversions.hpp"

// // // returns number of h2h edges
// size_t mem_graph_t::stream_build(std::ifstream &fin, size_t num_edges, dense_bitset &is_high_degree, dense_bitset &has_high_degree_neighbor, std::vector<size_t> &count, bool write_low_degree_edgelist){
// 	size_t num_all_edges = num_edges;

// 	fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);
// 	neighbors = (vid_eid_t *)realloc(neighbors, sizeof(vid_eid_t) * num_edges * 2); // store 2 vids for each edge
// 	CHECK(neighbors) << "allocation failed!";


// 	LOG(INFO) << "stream builder starts...";
// 	std::vector<vid_t> offsets(num_vertices, 0); // to put the in-neighbors at the right position when building the column array

// 	nedges = num_edges; // num_edges, num_vertices
// 	double average_degree = (num_edges * 2.0)  / (double)num_vertices; // non-rounded average degree
// 	high_degree_threshold = average_degree * high_degree_factor; // this is the th, if exceeded, the node is ignored for csr

// 	LOG(INFO) << "Average degree: " << average_degree << std::endl;
// 	LOG(INFO) << "High degree threshold: " << high_degree_threshold << std::endl;

// 	std::vector<edge_t> tmp_edges; // temporary buffer to read edges from file
// 	size_t chunk_size;

// 	if (num_edges >= 100000){
// 		chunk_size = 100000; // batch read of so many edges
// 	}
// 	else {
// 		chunk_size = num_edges;
// 	}
// 	tmp_edges.resize(chunk_size);

// 	while (num_edges > 0){ // edges to be read
// 		fin.read((char *)&tmp_edges[0], sizeof(edge_t) * chunk_size);
// 	    for (size_t i = 0; i < chunk_size; i++){
// 	    	count[tmp_edges[i].first]++;
// 	    	count[tmp_edges[i].second]++;
// 	    	offsets[tmp_edges[i].first]++;
// 	    }
// 	    num_edges -= chunk_size;
// 	    if (num_edges < chunk_size){ // adapt chunk size for last batch read
// 	    	chunk_size = num_edges;
// 	    }
// 	}
// 	// now, counts are complete. counts are the degrees of the vertices.

// 	/************************
// 	 * build the index array
// 	 * **********************
// 	 */
// 	vid_t h_count = 0; // how many high degree vertices are found
// 	vdata[0] = mem_adjlist_t(neighbors);
// 	if (count[0] > high_degree_threshold){
// 		is_high_degree.set_bit_unsync(0);
// 		h_count++;
// 	}

// 	std::vector<size_t> index(num_vertices, 0); // for index array

// 	for (vid_t v = 1; v < num_vertices; v++) {

// 		// we ignore the counts of vertices that have a high degree; we also ignore them when building the column array
// 		if (count[v-1] <= high_degree_threshold){
// 			index[v] = index[v-1] + count[v-1];
// 		}
// 		else{
// 			index[v] = index[v-1]; // ignoring v-1, will not use it in CSR
// 		}

// 		// set the index array
// 		vdata[v] = mem_adjlist_t( neighbors + index[v]);

// 		if  (count[v] > high_degree_threshold){
// 			is_high_degree.set_bit_unsync(v);
// 			h_count++;
// 		}
// 	}

// 	LOG(INFO) << "Number of vertices with high degree " << h_count << std::endl;
// 	std::streampos pos(0);
// 	h2h_file.seekp(pos);

// 	std::streampos pos_low(0);
// 	low_degree_file.seekp(pos_low);
// 	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
// 	low_degree_file.write((char *)&num_edges, sizeof(num_edges));

// 	/****************************
// 	 * build the column array
// 	 * **************************
// 	 */
//     num_edges = nedges;
// 	//resizing the chunk size
// 	if (num_edges >= 100000){
// 	   	chunk_size = 100000; // batch read of so many edges
// 	}
// 	else {
// 	   	chunk_size = num_edges;
// 	}

// 	fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg); // start read from beginning

// 	size_t savings = 0;

// 	while (num_edges > 0){ // edges to be read
// 	   	fin.read((char *)&tmp_edges[0], sizeof(edge_t) * chunk_size);
// 	   	for (size_t i = 0; i < chunk_size; i++){
// 	   		vid_t u = tmp_edges[i].first;
// 	   		vid_t v = tmp_edges[i].second;
// 	   		// we do not build column array for high degree vertices
// 	   		bool low_degree = false; // needed in case we write a low_degree edge list out to file
// 	   		if (count[u] <= high_degree_threshold){
// 	   			vdata[u].push_back_out(vid_eid_t(v, i));
// 	   			low_degree = true;
// 	   		}
// 	   		else{
// 	   			has_high_degree_neighbor.set_bit_unsync(v);
// 	   			savings++;
// 	   		}
// 	   		if (count[v] <= high_degree_threshold){
// 	   			vdata[v].push_back_in(vid_eid_t(v, i), offsets[v]);
// 	   			low_degree = true;
// 	   		}
// 	   		else{
// 	   			has_high_degree_neighbor.set_bit_unsync(u);
// 	   			savings++;
// 	   		  	if (count[u] > high_degree_threshold){ // u AND v are both high degree vertices, treat the edge specially
// 	   		  		edge_t edge = edge_t(u,v);
// 	   		  		h2h_file.write((char*)&edge, sizeof(edge_t));
// 	   		  		num_h2h_edges++;
// 	   			}
// 	   		}
// 	   		if (write_low_degree_edgelist && low_degree){
// 	   			edge_t edge = edge_t(u,v);
// 	   			low_degree_file.write((char*)&edge, sizeof(edge_t));
// 	   		}

// 	   	}
// 	   	num_edges -= chunk_size;
// 	   	if (num_edges < chunk_size){ // adapt chunk size for last batch read
// 	   		chunk_size = num_edges;
// 	  	}
// 	}
// 	LOG(INFO) << "Edges to a high-degree vertex: " << savings << std::endl;
// 	LOG(INFO) << "Edges between two high-degree vertices: " << num_h2h_edges << std::endl;

// 	// write the number of vertices and number of low-degree edges to the low-file
// 	size_t num_low_edges = num_all_edges - num_h2h_edges;

// 	low_degree_file.seekp(pos_low);
// 	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
// 	low_degree_file.write((char *)&num_low_edges, sizeof(num_edges));


// 	return num_h2h_edges;

// }

// returns number of h2h edges
size_t mem_graph_t::inmemory_build(std::ifstream &fin, size_t num_edges, dense_bitset &is_high_degree, dense_bitset &has_high_degree_neighbor, std::vector<size_t> &count, bool write_low_degree_edgelist, std::vector<edge_t> &edges){
	size_t num_all_edges = num_edges;

	fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);
	neighbors = (vid_eid_t *)realloc(neighbors, sizeof(vid_eid_t) * num_edges * 2); // store 2 vids for each edge
	CHECK(neighbors) << "allocation failed!";

	LOG(INFO) << "builder starts...";
	std::vector<vid_t> offsets(num_vertices, 0); // to put the in-neighbors at the right position when building the column array

	nedges = num_edges; // num_edges, num_vertices
	double average_degree = (num_edges * 2.0)  / (double)num_vertices; // non-rounded average degree
	high_degree_threshold = average_degree * high_degree_factor; // this is the th, if exceeded, the node is ignored for csr

	LOG(INFO) << "Average degree: " << average_degree << std::endl;
	LOG(INFO) << "High degree threshold: " << high_degree_threshold << std::endl;

    edges.resize(nedges);
    fin.read((char *)&edges[0], sizeof(edge_t) * nedges);

    for (size_t i = 0; i < nedges; i++){
        count[edges[i].first]++;
        count[edges[i].second]++;
        offsets[edges[i].first]++;
    }

	// now, counts are complete. counts are the degrees of the vertices.

	/************************
	 * build the index array
	 * **********************
	 */
	vid_t h_count = 0; // how many high degree vertices are found
	vdata[0] = mem_adjlist_t(neighbors);
	if (count[0] > high_degree_threshold){
		is_high_degree.set_bit_unsync(0);
		h_count++;
	}

	std::vector<size_t> index(num_vertices, 0); // for index array

	for (vid_t v = 1; v < num_vertices; v++) {

		// we ignore the counts of vertices that have a high degree; we also ignore them when building the column array
		if (count[v-1] <= high_degree_threshold){
			index[v] = index[v-1] + count[v-1];
		}
		else{
			index[v] = index[v-1]; // ignoring v-1, will not use it in CSR
		}

		// set the index array
		vdata[v] = mem_adjlist_t( neighbors + index[v]);

		if  (count[v] > high_degree_threshold){
			is_high_degree.set_bit_unsync(v);
			h_count++;
		}
	}

	LOG(INFO) << "Number of vertices with high degree " << h_count << std::endl;
	std::streampos pos(0);
	h2h_file.seekp(pos);

	std::streampos pos_low(0);
	low_degree_file.seekp(pos_low);
	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
	low_degree_file.write((char *)&num_edges, sizeof(num_edges));

	/****************************
	 * build the column array
	 * **************************
	 */

	size_t savings = 0;

    for (size_t i = 0; i < nedges; i++){
        vid_t u = edges[i].first;
        vid_t v = edges[i].second;
        // we do not build column array for high degree vertices
        bool low_degree = false; // needed in case we write a low_degree edge list out to file
        if (count[u] <= high_degree_threshold){
            vdata[u].push_back_out(vid_eid_t(v, i));
            low_degree = true;
        }
        else{
            has_high_degree_neighbor.set_bit_unsync(v);
            savings++;
        }
        if (count[v] <= high_degree_threshold){
            vdata[v].push_back_in(vid_eid_t(u, i), offsets[v]);
            low_degree = true;
        }
        else{
            has_high_degree_neighbor.set_bit_unsync(u);
            savings++;
            if (count[u] > high_degree_threshold){ // u AND v are both high degree vertices, treat the edge specially
                edge_with_id_t edge = edge_with_id_t(u,v,i);
                h2h_file.write((char*)&edge, sizeof(edge_with_id_t));
                num_h2h_edges++;
            }
        }
        if (write_low_degree_edgelist && low_degree){
            edge_with_id_t edge = edge_with_id_t(u,v,i);
            low_degree_file.write((char*)&edge, sizeof(edge_with_id_t));
        }

    }

	LOG(INFO) << "Edges to a high-degree vertex: " << savings << std::endl;
	LOG(INFO) << "Edges between two high-degree vertices: " << num_h2h_edges << std::endl;

	// write the number of vertices and number of low-degree edges to the low-file
	size_t num_low_edges = num_all_edges - num_h2h_edges;

	low_degree_file.seekp(pos_low);
	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
	low_degree_file.write((char *)&num_low_edges, sizeof(num_edges));


	return num_h2h_edges;

}

// returns number of h2h edges
size_t mem_graph_t::stream_build(std::ifstream &fin, size_t num_edges, dense_bitset &is_high_degree, dense_bitset &has_high_degree_neighbor, std::vector<size_t> &count, bool write_low_degree_edgelist){
	size_t num_all_edges = num_edges;

	fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);
	neighbors = (vid_eid_t *)realloc(neighbors, sizeof(vid_eid_t) * num_edges * 2); // store 2 vids for each edge
	CHECK(neighbors) << "allocation failed!";

	LOG(INFO) << "builder starts...";
	std::vector<vid_t> offsets(num_vertices, 0); // to put the in-neighbors at the right position when building the column array

	nedges = num_edges; // num_edges, num_vertices
	double average_degree = (num_edges * 2.0)  / (double)num_vertices; // non-rounded average degree
	high_degree_threshold = average_degree * high_degree_factor; // this is the th, if exceeded, the node is ignored for csr

	LOG(INFO) << "Average degree: " << average_degree << std::endl;
	LOG(INFO) << "High degree threshold: " << high_degree_threshold << std::endl;

    std::vector<edge_t> tmp_edges; // temporary buffer to read edges from file
	size_t chunk_size;

	if (num_edges >= 100000){
		chunk_size = 100000; // batch read of so many edges
	}
	else {
		chunk_size = num_edges;
	}
	tmp_edges.resize(chunk_size);
    fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);

    while (num_edges > 0){ // edges to be read
		fin.read((char *)&tmp_edges[0], sizeof(edge_t) * chunk_size);
	    for (size_t i = 0; i < chunk_size; i++){
	    	count[tmp_edges[i].first]++;
	    	count[tmp_edges[i].second]++;
	    	offsets[tmp_edges[i].first]++;
	    }
	    num_edges -= chunk_size;
	    if (num_edges < chunk_size){ // adapt chunk size for last batch read
	    	chunk_size = num_edges;
	    }
	}
	// now, counts are complete. counts are the degrees of the vertices.

	/************************
	 * build the index array
	 * **********************
	 */
	vid_t h_count = 0; // how many high degree vertices are found
	vdata[0] = mem_adjlist_t(neighbors);
	if (count[0] > high_degree_threshold){
		is_high_degree.set_bit_unsync(0);
		h_count++;
	}

	std::vector<size_t> index(num_vertices, 0); // for index array

	for (vid_t v = 1; v < num_vertices; v++) {

		// we ignore the counts of vertices that have a high degree; we also ignore them when building the column array
		if (count[v-1] <= high_degree_threshold){
			index[v] = index[v-1] + count[v-1];
		}
		else{
			index[v] = index[v-1]; // ignoring v-1, will not use it in CSR
		}

		// set the index array
		vdata[v] = mem_adjlist_t( neighbors + index[v]);

		if  (count[v] > high_degree_threshold){
			is_high_degree.set_bit_unsync(v);
			h_count++;
		}
	}

	LOG(INFO) << "Number of vertices with high degree " << h_count << std::endl;
	std::streampos pos(0);
	h2h_file.seekp(pos);

	std::streampos pos_low(0);
	low_degree_file.seekp(pos_low);
	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
	low_degree_file.write((char *)&num_edges, sizeof(num_edges));

	/****************************
	 * build the column array
	 * **************************
	 */
    num_edges = nedges;
	//resizing the chunk size
	if (num_edges >= 100000){
	   	chunk_size = 100000; // batch read of so many edges
	}
	else {
	   	chunk_size = num_edges;
	}

	fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg); // start read from beginning

	size_t savings = 0;

    vid_t edge_id = 0;
    while (num_edges > 0){ // edges to be read
	   	fin.read((char *)&tmp_edges[0], sizeof(edge_t) * chunk_size);
	    for (size_t i = 0; i < chunk_size; i++, edge_id ++){
	   		vid_t u = tmp_edges[i].first;
	   		vid_t v = tmp_edges[i].second;
	   		// we do not build column array for high degree vertices
	   		bool low_degree = false; // needed in case we write a low_degree edge list out to file
	   		if (count[u] <= high_degree_threshold){
	   			vdata[u].push_back_out(vid_eid_t(v, edge_id));
	   			low_degree = true;
	   		}
	   		else{
	   			has_high_degree_neighbor.set_bit_unsync(v);
	   			savings++;
	   		}
	   		if (count[v] <= high_degree_threshold){
	   			vdata[v].push_back_in(vid_eid_t(u, edge_id), offsets[v]);
	   			low_degree = true;
	   		}
	   		else{
	   			has_high_degree_neighbor.set_bit_unsync(u);
	   			savings++;
	   		  	if (count[u] > high_degree_threshold){ // u AND v are both high degree vertices, treat the edge specially
	   		  		edge_with_id_t edge = edge_with_id_t(u,v, edge_id);
	   		  		h2h_file.write((char*)&edge, sizeof(edge_with_id_t));
	   		  		num_h2h_edges++;
	   			}
	   		}
	   		if (write_low_degree_edgelist && low_degree){
	   			edge_with_id_t edge = edge_with_id_t(u,v, edge_id);
	   			low_degree_file.write((char*)&edge, sizeof(edge_with_id_t));
	   		}
	   	}
	   	num_edges -= chunk_size;
	    if (num_edges < chunk_size){ // adapt chunk size for last batch read
	    	chunk_size = num_edges;
	    }
	}

	LOG(INFO) << "Edges to a high-degree vertex: " << savings << std::endl;
	LOG(INFO) << "Edges between two high-degree vertices: " << num_h2h_edges << std::endl;

	// write the number of vertices and number of low-degree edges to the low-file
	size_t num_low_edges = num_all_edges - num_h2h_edges;

	low_degree_file.seekp(pos_low);
	low_degree_file.write((char *)&num_vertices, sizeof(num_vertices));
	low_degree_file.write((char *)&num_low_edges, sizeof(num_edges));


	return num_h2h_edges;

}