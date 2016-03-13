/* perf_stat.h
 * copyrights by beyondy, 2007-2010
 * all rights reserved.
 *
 *
**/
#ifndef __PERF_STAT__H
#define __PERF_STAT__H

#include <map>			// for std::map
#include <string>
#include <iostream>
#include <sys/time.h>		// for gettimeofday
#include <pthread.h>

namespace beyondy {
	struct perf_entry {
		perf_entry() : pe_called_total(0ULL), 
			       pe_cost_total(0ULL), 
			       pe_cost_min(-1UL),
			       pe_cost_max(0UL)
		{
		}

		unsigned long long pe_called_total;
		unsigned long long pe_cost_total;		// in us???
		unsigned long pe_cost_min;
		unsigned long pe_cost_max;
	};

	class perf_stat {
	public:
		perf_stat()
		{
			pthread_mutex_init(&ps_lock, NULL);
			
		}

		~perf_stat()
		{
			pthread_mutex_destroy(&ps_lock);
		}

		static perf_stat& instance()
		{
			static perf_stat static_stat;
			return static_stat;
		}
		
		void add(const char *name, unsigned long cost)
		{
			pthread_mutex_lock(&ps_lock);
			{
				perf_entry &entry = ps_stat_map[name];

				++entry.pe_called_total;
				if (cost < entry.pe_cost_min)
					entry.pe_cost_min = cost;
				if (cost > entry.pe_cost_max)
					entry.pe_cost_max = cost;
				entry.pe_cost_total += cost;
			}
			pthread_mutex_unlock(&ps_lock);
		}

		template <typename _oStream>
		_oStream& print_header(_oStream& out) const
		{
			out << "Name\tTotal-Called\tTotal-Cost\tAvg-Cost\tMin-Cost\tMax-Cost" << std::endl;
			return out;
				
		}

		template <typename _oStream>
		_oStream& print(_oStream& out)
		{
			pthread_mutex_lock(&ps_lock);
			for (const_iterator iter = ps_stat_map.begin(); iter != ps_stat_map.end(); ++iter) {
				unsigned long average = iter->second.pe_cost_total;
				if (iter->second.pe_called_total == 0)
					average = 0;
				else
					average /= iter->second.pe_called_total;

				out << iter->first << "\t" << iter->second.pe_called_total
				    << "\t" << iter->second.pe_cost_total << "\t"
				    << average << "\t" << iter->second.pe_cost_min << "\t"
				    << iter->second.pe_cost_max << std::endl;
			}

			out << std::endl;
			pthread_mutex_unlock(&ps_lock);
			return out;
		}

	private:
		typedef std::map<std::string, perf_entry> map_type;
		typedef map_type::iterator iterator;
		typedef map_type::const_iterator const_iterator;

		// TODO: sync-lock???
		map_type ps_stat_map;
		pthread_mutex_t ps_lock;
	};

	class perf_guard {
	public:
		perf_guard(const char *name, perf_stat& state = perf_stat::instance()) : pg_name(name), pg_state(state)
		{
			gettimeofday(&pg_start_tv, NULL);
		}

		~perf_guard()
		{
			struct timeval end_tv;

			gettimeofday(&end_tv, NULL);
			unsigned long cost = (end_tv.tv_sec  - pg_start_tv.tv_sec ) * 1000000 + \
					     (end_tv.tv_usec - pg_start_tv.tv_usec);
			pg_state.add(pg_name.c_str(), cost);
		}

		void reset()
		{
			gettimeofday(&pg_start_tv, NULL);
		}

		void reset(const char *name)
		{
			if (name) pg_name = name;	
			gettimeofday(&pg_start_tv, NULL);
		}

		void set_name(const char *name)
		{
			if (name) pg_name = name;
		}

		const char *get_name() const
		{
			return pg_name.c_str();
		}

	private:
		perf_guard(const perf_guard&);
		perf_guard& operator=(const perf_guard&);

	private:
		std::string pg_name;
		perf_stat& pg_state;
 
		struct timeval pg_start_tv;
	};

	template <typename _oStream>
	_oStream& perf_print(_oStream& out, perf_stat *pfs = 0)
	{
		if (pfs == 0) pfs = &perf_stat::instance();

		pfs->print_header(out);
		pfs->print(out);

		return out;
	}
};

#ifdef __TRACE_PROF__
#define TRACE_PROF() beyondy::perf_guard __trace_prof(__PRETTY_FUNCTION__ )
#define TRACE_PROF_NAME(name) beyondy::perf_guard  __trace_prof_name(name)
#else
#define TRACE_PROF()	do {} while (0) /* nothing */
#define TRACE_PROF_NAME(name) do {} while(0) /* nothing */
#endif /* __TRACE_PROF__ */

#endif	/*! __PERF_STATE__H */

