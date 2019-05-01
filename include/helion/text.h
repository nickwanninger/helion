// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#include <string>

namespace helion {

	using rune = uint32_t;

	class text {
		public:

		  // using container = std::u32string;
		  using container = std::u32string;

		protected:

			friend std::hash<helion::text>;


			void ingest_utf8(std::string const&);

		public:
			container buf;

			typedef container::iterator iterator;
			typedef container::const_iterator const_iterator;

			typedef rune value_type;
			typedef rune& reference;
			typedef const rune& const_reference;

			text(void);
			text(char*);
			text(const char*);
			text(const char32_t*);
			text(std::string const&);
			text(std::u32string const&);
			text(const helion::text&);

			~text(void);


			iterator begin(void);
			iterator end(void);

			const_iterator cbegin(void) const;
			const_iterator cend(void) const;

			uint32_t size(void);
			uint32_t length(void);
			uint32_t max_size(void);

			void resize(uint32_t, rune c = 0);

			uint32_t capacity(void);

			void clear(void);

			bool empty(void);


			text& operator+=(const text&);
			text& operator+=(const char *);
			text& operator+=(std::string const&);
			text& operator+=(char);
			text& operator+=(int);
			text& operator+=(rune);

			text& operator=(const text&);

			bool operator==(const text &other) const;

			bool operator==(const text&);


			operator std::string() const;
			operator std::u32string() const;

			rune operator[](size_t) const;
			rune operator[](size_t);


			void push_back(rune&);
			void push_back(rune&&);
	};


	inline std::ostream& operator<<(std::ostream& os, text& r) {
		std::string s = r;
		return os << s;
	}

	inline std::ostream& operator<<(std::ostream& os, const text& r) {
		std::string s = r;
		return os << s;
	}

};


template <>
struct std::hash<helion::text> {
	std::size_t operator()(const helion::text& k) const
	{
		std::hash<helion::text::container> hash_func;
		return hash_func(k.buf);
	}
};
