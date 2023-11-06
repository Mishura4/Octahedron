#ifndef OCTAHEDRON_VISITOR_H_
#define OCTAHEDRON_VISITOR_H_

namespace octahedron
{
template <typename... Ts>
struct visitor : Ts... {
	using Ts::operator()...;
};

template <typename... Ts>
visitor(Ts...) -> visitor<Ts...>;
}

#endif /* SHION_VISITOR_H_ */
