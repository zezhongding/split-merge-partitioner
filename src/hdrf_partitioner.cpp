#include "hdrf_partitioner.hpp"
#include "conversions.hpp"

HdrfPartitioner::HdrfPartitioner(std::string basefilename, bool need_k_split)
    : basefilename(basefilename), writer(basefilename, !need_k_split && FLAGS_write)
{
    Timer convert_timer;
    convert_timer.start();
    Converter *converter = new Converter(basefilename);
    convert(basefilename, converter);
    delete converter;
    convert_timer.stop();
    LOG(INFO) << "convert time: " << convert_timer.get_time();

    total_time.start();
    LOG(INFO) << "initializing partitioner";

    std::ifstream fin(binedgelist_name(basefilename),
                      std::ios::binary | std::ios::ate);
    auto filesize = fin.tellg();
    LOG(INFO) << "file size: " << filesize;
    fin.seekg(0, std::ios::beg);

    fin.read((char *)&num_vertices, sizeof(num_vertices));
    fin.read((char *)&num_edges, sizeof(num_edges));

    LOG(INFO) << "num_vertices: " << num_vertices
              << ", num_edges: " << num_edges;
    CHECK_EQ(sizeof(vid_t) + sizeof(eid_t) + num_edges * sizeof(edge_t), filesize);

    p = FLAGS_p;
    if (need_k_split) {
        p *= FLAGS_k;
    }

    edges.resize(num_edges);
    fin.read((char *)&edges[0], sizeof(edge_t) * num_edges);

    LOG(INFO) << "constructing...";

    is_boundarys.assign(p, dense_bitset(num_vertices));
    occupied.assign(p, 0);
    capacity = (double)num_edges * 1.0 / p + 1; //will be used to as stopping criterion later
    // edgelist2bucket.assign(num_edges, kInvalidBid);

    degrees.resize(num_vertices);
    std::ifstream degree_file(degree_name(basefilename), std::ios::binary);
    degree_file.read((char *)&degrees[0], num_vertices * sizeof(vid_t));
    degree_file.close();
    max_degree = *std::max_element(degrees.begin(), degrees.end());
}

void HdrfPartitioner::split()
{
    // std::shuffle(edges.begin(), edges.end(), rd);
    for (eid_t eid = 0; eid < num_edges; ++eid) {
        edge_t e = edges[eid];
        if (eid % 50000000 == 0) {
            LOG(INFO) << "Processing edges " << eid;
        }
        vid_t u = e.first, v = e.second;
        bid_t bucket = best_scored_partition(u, v); // according to ebv scoring
        assign_edge(bucket, u, v, eid);
        if (occupied[bucket] > max_size) {
            max_size = occupied[bucket];
        }
        if (occupied[bucket] == min_size) {
            int min_sized_bucket_count = 0;
            for (bid_t b = 0; b < p; ++b) {
                if (occupied[b] == min_size) {
                    ++min_sized_bucket_count;
                }
            }
            if (min_sized_bucket_count == 1) {
                ++min_size;
            }
        }
    }

    total_time.stop();
    LOG(INFO) << "total partition time: " << total_time.get_time();
    for (bid_t b = 0; b < p; ++b) {
        LOG(INFO) << b << ' ' << is_boundarys[b].popcount() << ' ' << occupied[b];
    }
    calculate_stats();
}

bid_t HdrfPartitioner::best_scored_partition(vid_t u, vid_t v) {
	double best_score = -1.0;
	bid_t best_partition = 0;
	for (bid_t b = 0; b < p; ++b) {
		double score = compute_partition_score(u, v, b);
		if (score > best_score) {
			best_score = score;
			best_partition = b;
		}
	}
	return best_partition;
}

double HdrfPartitioner::compute_partition_score(vid_t u, vid_t v, bid_t bucket_id) {
	if (occupied[bucket_id] >= capacity) {
		return -1.0; // partition is full, do not choose it
	}
	vid_t degree_u = degrees[u];
	vid_t degree_v = degrees[v];
	vid_t sum = degree_u + degree_v;
	double gu = 0.0, gv = 0.0;
	if (is_boundarys[bucket_id].get(u)) {
		gu = degree_u;
		gu /= sum;
		gu = 1 + (1 - gu);
	}
	if (is_boundarys[bucket_id].get(v)) {
        gv = degree_v;
        gv /= sum;
        gv = 1 + (1 - gv);
	}

	double bal = (max_size - occupied[bucket_id]) / (1.0 + max_size - min_size);

	double score = gu + gv + lambda * bal;
	return score;
}

void HdrfPartitioner::calculate_stats()
{
    std::cerr << std::string(25, '#') << " Calculating Statistics " << std::string(25, '#') << '\n';
    std::vector<vid_t> num_bucket_vertices(p, 0);
    for (bid_t b = 0; b < p; ++b) {
        num_bucket_vertices[b] = is_boundarys[b].popcount();
    }
    vid_t max_part_vertice_cnt = *std::max_element(num_bucket_vertices.begin(), num_bucket_vertices.end());
    vid_t all_part_vertice_cnt = accumulate(num_bucket_vertices.begin(), num_bucket_vertices.end(), (vid_t)0);
    eid_t max_part_edge_cnt = *std::max_element(occupied.begin(), occupied.end());
    eid_t all_part_edge_cnt = accumulate(occupied.begin(), occupied.end(), (eid_t)0);

    for (bid_t b = 0; b < p; ++b) {
        LOG(INFO) << "Bucket_info: " << b
                << ", vertices: " << num_bucket_vertices[b]
                << ", edges: " << occupied[b];
    }
    
    double avg_vertice_cnt = static_cast<double>(all_part_vertice_cnt) / p;
    double avg_edge_cnt = static_cast<double>(all_part_edge_cnt) / p;

    double std_vertice_deviation = 0.0;
    double std_edge_deviation = 0.0;
    for (bid_t b = 0; b < p; ++b) {
        std_vertice_deviation += pow(num_bucket_vertices[b] - avg_vertice_cnt, 2);
        std_edge_deviation += pow(occupied[b] - avg_edge_cnt, 2);
    }
    std_vertice_deviation = sqrt(static_cast<double>(std_vertice_deviation) / p);
    std_edge_deviation = sqrt(static_cast<double>(std_edge_deviation) / p);

    
    LOG(INFO) << std::string(20, '#') << "\tVertice    balance\t" << std::string(20, '#');
    LOG(INFO) << "Max vertice count / avg vertice count: "
              << static_cast<double>(max_part_vertice_cnt) / (num_vertices / p);
    LOG(INFO) << "Max Vertice count: "
              << max_part_vertice_cnt;
    LOG(INFO) << "Avg Vertice count(No replicate): "
              << num_vertices / p;
    LOG(INFO) << "Vertice std_vertice_deviation / avg: "
              << std_vertice_deviation / avg_vertice_cnt;

    LOG(INFO) << std::string(20, '#') << "\tEdge       balance\t" << std::string(20, '#');
    LOG(INFO) << "Max edge count / avg edge count: "
              << static_cast<double>(max_part_edge_cnt) / avg_edge_cnt;
    LOG(INFO) << "Max Edge count: "
              << max_part_edge_cnt;
    LOG(INFO) << "Avg Edge count: "
              << num_edges / p;
    LOG(INFO) << "Edge std_edge_deviation / avg: "
              << std_edge_deviation / avg_edge_cnt;

    CHECK_EQ(all_part_edge_cnt, num_edges);
    LOG(INFO) << std::string(20, '#') << "\tReplicate    factor\t" << std::string(20, '#');
    LOG(INFO) << "replication factor (final): " << static_cast<double>(all_part_vertice_cnt) / num_vertices;
}