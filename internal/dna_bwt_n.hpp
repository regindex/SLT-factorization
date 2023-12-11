#include "include.hpp"
#include "dna_string_n.hpp"

#ifndef INTERNAL_DNA_BWT_N_HPP_
#define INTERNAL_DNA_BWT_N_HPP_

template<class str_type>
class dna_bwt_n{

public:

	typedef sa_node_n sa_node_t;

	dna_bwt_n(){};

	/*
	 * constructor path of a BWT file containing the BWT in ASCII format
	 */
	dna_bwt_n(string path, char TERM = '#') : TERM(TERM){

		n = uint64_t(filesize(path));

		BWT = str_type(path, TERM);

		uint64_t n_term = 0;

		//build F column
		for(uint64_t i=0;i<n;++i){

			assert(BWT[i] < 256);

			F_A += (BWT[i]==TERM);
			F_C += (BWT[i]=='A');
			F_G += (BWT[i]=='C');
			F_N += (BWT[i]=='G');
			F_T += (BWT[i]=='N');

		}

		F_C += F_A;
		F_G += F_C;
		F_N += F_G;
		F_T += F_N;

	}

	/*
	 * get full BWT range
	 */
	range_t full_range(){

		//right-exclusive range
		return {0,size()};

	}

	/*
	 * left-extend range by all letters
	 */
	p_range_n LF(range_t rn){

		assert(rn.second >= rn.first);

		//number of A,C,T,G,N before start of interval
		p_rank_n start = BWT.parallel_rank(rn.first);

		//number of A,C,T,G,N before end of interval (last position of interval included)
		p_rank_n end;

		if(rn.second>rn.first)
			end	= BWT.parallel_rank(rn.second);
		else
			end = start;

		assert(start <= end);

		p_rank_n f = {F_A,F_C,F_G,F_N,F_T};
		p_rank_n l = f + start;
		p_rank_n r = f + end;

		assert(r.A <= l.C);
		assert(r.C <= l.G);
		assert(r.G <= l.N);
		assert(r.N <= l.T);
		assert(l.T <= n);

		return fold_ranks(l,r);

	}

	char operator[](uint64_t i){

		return BWT[i];

	}

	/*
	 * number of c before position i excluded
	 */
	uint64_t rank(uint64_t i, uint8_t c){

		assert(i<=n);

		return BWT.rank(i,c);

	}

	/*
	 * return number of occurrences of A,C,T,G,N in the prefix of length i of the text. At most 1 cache miss!
	 */
	p_rank_n parallel_rank(uint64_t i){

		return BWT.parallel_rank(i);

	}

	uint64_t size(){

		assert(n == BWT.size());
		return n;

	}

	uint64_t serialize(std::ostream& out){

		uint64_t w_bytes = 0;

		out.write((char*)&n,sizeof(n));
		out.write((char*)&F_A,sizeof(uint64_t));
		out.write((char*)&F_C,sizeof(uint64_t));
		out.write((char*)&F_G,sizeof(uint64_t));
		out.write((char*)&F_N,sizeof(uint64_t));
		out.write((char*)&F_T,sizeof(uint64_t));

		w_bytes += sizeof(n) + sizeof(uint64_t)*5;

		w_bytes += BWT.serialize(out);

		return w_bytes;

	}

	/* load the structure from the istream
	 * \param in the istream
	 */
	void load(std::istream& in) {

		in.read((char*)&n,sizeof(n));
		in.read((char*)F_A,sizeof(uint64_t));
		in.read((char*)F_C,sizeof(uint64_t));
		in.read((char*)F_G,sizeof(uint64_t));
		in.read((char*)F_N,sizeof(uint64_t));
		in.read((char*)F_T,sizeof(uint64_t));

		BWT.load(in);

	}

	void save_to_file(string path){

		std::ofstream out(path);
		serialize(out);
		out.close();

	}

	/*
	 * path = path of an index file
	 */
	void load_from_file(string path){

		std::ifstream in(path);
		load(in);
		in.close();

	}


	/*
	 * functions for suffix tree navigation
	 */

	sa_node_n root(){

		return {
			0,
			F_A,
			F_C,
			F_G,
			F_N,
			F_T,
			n,
			0
		};

	}

	/*
	 * depth = LCP inside the leaf.
	 */
	sa_leaf first_leaf(){

		return {{0, F_A}, 0};

	}

	/*
	 * Input: suffix tree node N.
	 * Output: suffix tree nodes (explicit, implicit, or empty) reached applying LF for A,C,G,T,N from node N
	 */
	p_node_n LF(sa_node_n N){

		p_rank_n before_TERM;
		p_rank_n before_A;
		p_rank_n before_C;
		p_rank_n before_G;
		p_rank_n before_N;
		p_rank_n before_T;
		p_rank_n before_end;

		before_TERM = parallel_rank(N.first_TERM);

		if(N.first_A == N.first_TERM) before_A = before_TERM;
		else before_A = parallel_rank(N.first_A);

		if(N.first_C == N.first_A) before_C = before_A;
		else before_C = parallel_rank(N.first_C);

		if(N.first_G == N.first_C) before_G = before_C;
		else before_G = parallel_rank(N.first_G);

		if(N.first_N == N.first_G) before_N = before_G;
		else before_N = parallel_rank(N.first_N);

		if(N.first_T == N.first_N) before_T = before_N;
		else before_T = parallel_rank(N.first_T);

		if(N.last == N.first_T) before_end = before_T;
		else before_end = parallel_rank(N.last);



		return {
			{F_A + before_TERM.A, F_A + before_A.A, F_A + before_C.A, F_A + before_G.A, F_A + before_N.A, F_A + before_T.A, F_A + before_end.A, N.depth+1},
			{F_C + before_TERM.C, F_C + before_A.C, F_C + before_C.C, F_C + before_G.C, F_C + before_N.C, F_C + before_T.C, F_C + before_end.C, N.depth+1},
			{F_G + before_TERM.G, F_G + before_A.G, F_G + before_C.G, F_G + before_G.G, F_G + before_N.G, F_G + before_T.G, F_G + before_end.G, N.depth+1},
			{F_N + before_TERM.N, F_N + before_A.N, F_N + before_C.N, F_N + before_G.N, F_N + before_N.N, F_N + before_T.N, F_N + before_end.N, N.depth+1},
			{F_T + before_TERM.T, F_T + before_A.T, F_T + before_C.T, F_T + before_G.T, F_T + before_N.T, F_T + before_T.T, F_T + before_end.T, N.depth+1}
		};

	}

	//follow Weiner links from node x and push on the stack the resulting right-maximal nodes
	void next_nodes(sa_node_n & x, vector<sa_node_n> & TMP_NODES, int & t){

		p_node_n left_exts = LF(x);

		sa_node_n A = left_exts.A;
		sa_node_n C = left_exts.C;
		sa_node_n G = left_exts.G;
		sa_node_n N = left_exts.N;
		sa_node_n T = left_exts.T;

		t = 0;

		if(number_of_children(A) >= 2) TMP_NODES[t++] = A;
		if(number_of_children(C) >= 2) TMP_NODES[t++] = C;
		if(number_of_children(G) >= 2) TMP_NODES[t++] = G;
		if(number_of_children(N) >= 2) TMP_NODES[t++] = N;
		if(number_of_children(T) >= 2) TMP_NODES[t++] = T;

		//is x a leaf in the suffix link tree?
		if(
			number_of_children(A) < 2 and 
			number_of_children(C) < 2 and 
			number_of_children(G) < 2 and 
			number_of_children(N) < 2 and 
			number_of_children(T) < 2
		){
			number_sl_leaves++;
			number_sl_leaves_ext += number_of_children(x);
		}

		//push right-maximal nodes on stack in decreasing size (i.e. interval length) order

		std::sort( TMP_NODES.begin(), TMP_NODES.begin()+t, [ ]( const sa_node_n& lhs, const sa_node_n& rhs )
		{
			return node_size(lhs) < node_size(rhs);
		});

	}

	uint64_t get_number_sl_leaves(){
		return number_sl_leaves;
	}

		uint64_t get_number_sl_leaves_ext(){
		return number_sl_leaves_ext;
	}

private:

	char TERM = '#';

	uint64_t n = 0;//BWT length

	uint64_t F_A=0; //F array
	uint64_t F_C=0; //F array
	uint64_t F_G=0; //F array
	uint64_t F_N=0; //F array
	uint64_t F_T=0; //F array

	uint64_t number_sl_leaves=0; 	//number of leaves in the suffix link tree
	uint64_t number_sl_leaves_ext=0;	//number of right extensions of suffix link tree leaves

	//vector<uint64_t> F;
	str_type BWT;

};

typedef dna_bwt_n<dna_string_n> dna_bwt_n_t;

#endif /* INTERNAL_DNA_BWT_N_HPP_ */
