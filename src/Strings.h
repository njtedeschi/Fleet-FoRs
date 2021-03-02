#pragma once

#include <sstream>
#include <queue>
#include <string.h>
#include <array>

#include "Numerics.h"
#include "Random.h"

template<typename T>
std::string str(T x){
	/**
	 * @brief A pythonesque string function
	 * @param x
	 * @return 
	 */
	return std::to_string(x);
}

std::string str(const std::string& x){
	// Need to specialize this, otherwise std::to_string fails
	return x;
}

template<typename... T, size_t... I >
std::string str(const std::tuple<T...>& x, std::index_sequence<I...> idx){
	/**
	 * @brief A pythonesque string function
	 * @param x
	 * @return 
	 */
	return "<" + ( (std::to_string(std::get<I>(x)) + ",") + ...) + ">";
}
template<typename... T>
std::string str(const std::tuple<T...>& x ){
	/**
	 * @brief A pythonesque string function
	 * @param x
	 * @return 
	 */
	return str(x, std::make_index_sequence<sizeof...(T)>() );
}


template<typename T, size_t N>
std::string str(const std::array<T, N>& a ){
	/**
	 * @brief A pythonesque string function
	 * @param x
	 * @return 
	 */
	std::string out = "<";
	for(auto& x : a) {
		out += str(x) + ",";
	}
	return out+">";
}



template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 14) {
	// https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

/* If x is a prefix of y -- works for strings and vectors */
template<typename T>
bool is_prefix(const T& prefix, const T& x) {
	/**
	 * @brief For any number of iterable types, is prefix a prefix of x
	 * @param prefix
	 * @param x
	 * @return 
	 */
		
	if(prefix.size() > x.size()) return false;
	if(prefix.size() == 0) return true;
	
	return std::equal(prefix.begin(), prefix.end(), x.begin());
}

bool contains(const std::string& s, const std::string& x) {
	return s.find(x) != std::string::npos;
}

/**
 * @brief Probability of converting x into y by deleting some number (each with del_p, then stopping with prob 1-del_p), adding with 
 * 		  probability add_p, and then when we add selecting from an alphabet of size alpha_n
 * @param x
 * @param y
 * @param del_p - probability of deleting the next character (geometric)
 * @param add_p - probability of adding (geometric)
 * @param alpha_n - size of alphabet
 * @return The probability of converting x to y by deleting characters with probability del_p and then adding with probability add_p
 */
 template<const float& add_p, const float& del_p>
inline double p_delete_append(const std::string& x, const std::string& y, const float log_alphabet) {
	/**
	 * @brief This function computes the probability that x would be converted into y, when we insert with probability add_p and delete with probabiltiy del_p
	 * 		  and when we add we add from an alphabet of size log_alphabet. Note that this is a template function because otherwise
	 *        we end up computing log(add_p) and log(del_p) a lot, and these are in fact constant. 
	 * @param x
	 * @param y
	 * @param log_alphabet
	 * @return 
	 */
	
	// all of these get precomputed at compile time 
	static const float log_add_p   = log(add_p);
	static const float log_del_p   = log(del_p);
	static const float log_1madd_p = log(1.0-add_p);
	static const float log_1mdel_p = log(1.0-del_p);	
	
	
	// Well we can always delete the whole thing and add on the remainder
	float lp = log_del_p*x.length()                 + // we don't add log_1mdel_p again here since we can't delete past the beginning
				(log_add_p-log_alphabet)*y.length() + log_1madd_p;
	
	// now as long as they are equal, we can take only down that far if we want
	// here we index over mi, the length of the string that so far is equal
	for(size_t mi=1;mi<=std::min(x.length(),y.length());mi++){
		if(x[mi-1] == y[mi-1]) {
			lp = logplusexp(lp, log_del_p*(x.length()-mi)                + log_1mdel_p + 
							    (log_add_p-log_alphabet)*(y.length()-mi) + log_1madd_p);
		}
		else {
			break;
		}
	}
	
	return lp;
}


/**
 * @brief Split is returns a deque of s split up at the character delimiter. 
 * It handles these special cases:
 * split("a:", ':') -> ["a", ""]
 * split(":", ':')  -> [""]
 * split(":a", ':') -> ["", "a"]
 * @param s
 * @param delimiter
 * @return 
 */
std::deque<std::string> split(const std::string& s, const char delimiter) {
	std::deque<std::string> tokens;
	
	if(s.length() == 0) {
		return tokens;
	}
	
	size_t i = 0;
	while(i < s.size()) {
		size_t k = s.find(delimiter, i);
		
		if(k == std::string::npos) {
			tokens.push_back(s.substr(i,std::string::npos));
			return tokens;
		}
		else {		
			tokens.push_back(s.substr(i,k-i));
			i=k+1;
		}
	}	

	// if we get here, that means that the last foudn k was the last
	// character, which means we need to append ""
	tokens.push_back("");
	return tokens;
}

/**
 * @brief Split iwith a fixed return size, useful in parsing csv
 * @param s
 * @param delimiter
 * @return 
 */
template<size_t N>
std::array<std::string, N> split(const std::string& s, const char delimiter) {
	std::array<std::string, N> out;
	
	auto q = split(s, delimiter);
	
	// must be hte right size 
	if(q.size() != N) {
		std::cerr << "*** String in split<N> has " << q.size() << " not " << N << " elements: " << s << std::endl;
		assert(false);
	}
	
	// convert to an array now that we know it's the right size
	size_t i = 0;
	for(auto& x : q) {
		out[i++] = x;
	}

	return out;
}

std::pair<std::string, std::string> divide(const std::string& s, const char delimiter) {
	// divide this string into two pieces at the first occurance of delimiter
	auto k = s.find(delimiter);
	assert(k != std::string::npos && "*** Cannot divide a string without delimiter");
	return std::make_pair(s.substr(0,k), s.substr(k+1));
}



unsigned int levenshtein_distance(const std::string& s1, const std::string& s2) {
	/**
	 * @brief Compute levenshtein distiance between two strings (NOTE: Or O(N^2))
	 * @param s1
	 * @param s2
	 * @return 
	 */
	
	// From https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++

	const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
			  d[i][j] = std::min(d[i - 1][j] + 1, std::min(d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) ));
			  
	return d[len1][len2];
}


size_t count(const std::string& str, const std::string& sub) {
	/**
	 * @brief How many times does sub occur in str? Does not count overlapping substrings
	 * @param str
	 * @param sub
	 * @return 
	 */
	
	// https://www.rosettacode.org/wiki/Count_occurrences_of_a_substring#C.2B.2B
	
    if (sub.length() == 0) return 0;
    
	size_t count = 0;
    for (size_t offset = str.find(sub); 
		 offset != std::string::npos;
		 offset = str.find(sub, offset + sub.length())) {
        ++count;
    }
    return count;
}

std::string reverse(std::string x) {
	return std::string(x.rbegin(),x.rend()); 
}


std::string QQ(std::string x) {
	/**
	 * @brief Handy adding double quotes to a string
	 * @param x - input string
	 * @return 
	 */
	
	return std::string("\"") + x + std::string("\"");
}
std::string Q(std::string x) {
	/**
	 * @brief Handy adding single quotes to a string 
	 * @param x - input string
	 * @return 
	 */
		
	return std::string("\'") + x + std::string("\'");
}


// Some functions for converting to and from strings:
//namespace Fleet {
//	
//	
//	
//	const char NodeDelimiter = ';';
//	const char FactorDelimiter = '|';
//	
//	
//	
//}


/**
 * @brief The string probability model from Kashyap & Oommen, 1983, basically giving a string
 *        edit distance that is a probability model. This could really use some unit tests,
 *        but it is hard to find implementations. 
 * 			
 * 		  This assumes that the deletions, insertions, and changes all happen with a constant, equal
 *        probability of perr, but note that swaps and insertions also have to choose the character
 *        out of nalphabet. This could probably be optimized to not have to compute these logs
 * @param x
 * @param y
 * @param perr
 * @param nalphabet
 * @return 
 */
double p_KashyapOommen1983_edit(const std::string x, const std::string y, const double perr, const size_t nalphabet) {
	// From Kashyap & Oommen, 1983
	// perr gets used here to define the probability of each of these operations
	// BUT Note here that we use the logs of these 
	
	const int m = x.length(); // these must be ints or some negatives below go bad
	const int n = y.length();
	const int T = std::max(m,n); // max insertions allowed
	const int E = std::max(m,n); // max deletions allowed
	const int R = std::min(m,n);
	
	const double lp_insert = -log(nalphabet); // P(y_t|lambda) - probability of generating a single character from nothing
	const double lp_delete = log(perr);           // P(phi|d_x) -- probability of deleting
	
	#pragma GCC diagnostic ignored "-Wvla" // my god, just kill me
	double W[T+1][E+1][R+1]; 	// unlike in Kashyap & Oommen, this will store the LOG of the probability
	#pragma GCC diagnostic warning "-Wvla"
	
	// set to 1 in log space
	W[0][0][0] = 0.0; 
	
	// indexes into y and x, and returns the swap probability
	// when they are different. Also corrects so i,j are 0-indexed
	// instead of 1-indexed, as they are below
	const auto Sab = [&](int i, int j) -> double {	
		return (y[i-1]==x[j-1] ? log(1-perr) : log(perr)-log(nalphabet) );
	};
	
	for(int t=1;t<=T;t++) 
		W[t][0][0] = W[t-1][0][0] + lp_insert;
		
	for(int d=1;d<=E;d++)
		W[0][d][0] = W[0][d-1][0] + lp_delete;
	
	for(int s=1;s<=R;s++) 
		W[0][0][s] = W[0][0][s-1] + Sab(s-1,s-1);
		
	for(int t=1;t<=T;t++) 
		for(int d=1;d<=E;d++) 
			W[t][d][0] = logplusexp(W[t-1][d][0]+lp_insert, 
							        W[t][d-1][0]+lp_delete);
			
	for(int t=1;t<=T;t++)
		for(int s=1;s<=n-t;s++)
			W[t][0][s] = logplusexp(W[t-1][0][s]+lp_insert,
			                        W[t][0][s-1]+Sab(s+t-1,s-1));
	
	for(int d=1;d<=E;d++)
		for(int s=1;s<=m-d;s++) 
			W[0][d][s] = logplusexp(W[0][d-1][s]+lp_delete, 
			                        W[0][d][s-1]+Sab(s-1,s+d-1));

	for(int t=1;t<=T;t++)
		for(int d=1;d<=E;d++)
			for(int s=1;s<=std::min(n-t,m-d);s++)
				W[t][d][s] = logplusexp(W[t-1][d][s]+lp_insert,
				             logplusexp(W[t][d-1][s]+lp_delete,
								        W[t][d][s-1]+Sab(t+s-1,d+s-1))); // shoot me in the face	
	
	
	// now we sum up
	double lp_yGx = -infinity; // p(Y|X)
	for(int t=std::max(0,n-m); t<=std::min(T,E+n-m);t++){
		double qt = lpmf_geometric(t+1,1.0-perr); // probability of t insertions -- but must use t+1 to allow t=0 (geometric is defined on 1,2,3,...)
		lp_yGx = logplusexp(lp_yGx, W[t][m-n+t][n-t] + qt + lfactorial(m)+lfactorial(t)-lfactorial(m+t));
	}
	return lp_yGx;
	
}
